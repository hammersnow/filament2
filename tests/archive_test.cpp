#include "../archive.h"
#include <QtTest/QtTest>
#include <QDir>
#include <QFile>
#include <QTemporaryFile>

class ArchiveTest : public QObject
{
    Q_OBJECT
    QString root;
    Archive::Paths paths;
    void put(const QString &path, const QByteArray &data) {
        QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(data), qint64(data.size()));
    }
    QByteArray ledger(const QByteArray &oldDate = "2026-10-01",
                      const QByteArray &newDate = "2026-10-04") {
        return QString::fromUtf8("Cihaz,Tüp,Sökülen Tüp,Sökülen Tarih,Takilan Tüp,Takilan Tarih\n").toUtf8()
            + "ed1,1-3,OLD," + oldDate + ",NEW," + newDate + "\n";
    }
    QByteArray on(const QByteArray &time) {
        return "ED_02\nTotal working time: 09:00:00\n------\nED_01\nTotal working time: " + time + "\n------\n";
    }
private slots:
    void todayExcludedWhileYesterdayStillCounts() {
        const QDate today = QDate::currentDate();
        const QByteArray data = ledger(today.addDays(-5).toString("yyyy-MM-dd").toLatin1(),
                                       today.addDays(-3).toString("yyyy-MM-dd").toLatin1());
        put(paths.directories[Archive::Tubes].first() + "/" + Archive::fileName(Archive::Tubes,today),data);
        put(paths.directories[Archive::On].first() + "/" + Archive::fileName(Archive::On,today),"broken current day\n");
        put(paths.directories[Archive::On].first() + "/" + Archive::fileName(Archive::On,today.addDays(-1)),on("01:00:00"));
        const Archive::Report r = Archive::load("ED01","1-3",today,paths);
        QVERIFY(r.error.isEmpty());
        QCOMPARE(r.installed.end,today.addDays(-1));
        QCOMPARE(r.installed.expectedDays,2);
        QCOMPARE(r.installed.validDays,1);
        QCOMPARE(r.installed.missingDays,1);
        QCOMPARE(r.installed.invalidDays,0);
        QCOMPARE(r.installed.seconds,qint64(3600));
        QCOMPARE(r.days.size(),1);
        put(paths.directories[Archive::Tubes].first() + "/" + Archive::fileName(Archive::Tubes,today.addDays(-1)),data);
        const Archive::Report historical = Archive::load("ED01","1-3",today.addDays(-1),paths);
        QCOMPARE(historical.installed.expectedDays,2);
        QCOMPARE(historical.installed.missingDays,1);
        put(paths.directories[Archive::Tubes].first() + "/" + Archive::fileName(Archive::Tubes,today),
            ledger(today.addDays(-5).toString("yyyy-MM-dd").toLatin1(),today.toString("yyyy-MM-dd").toLatin1()));
        const Archive::Report newlyInstalled = Archive::load("ED01","1-3",today,paths);
        QVERIFY(newlyInstalled.error.isEmpty());
        QCOMPARE(newlyInstalled.installed.expectedDays,0);
        QCOMPARE(newlyInstalled.installed.missingDays,0);
        QCOMPARE(newlyInstalled.installed.invalidDays,0);
    }
    void init() {
        QTemporaryFile temp;
        QVERIFY(temp.open());
        root = temp.fileName() + "-archive";
        for (int i = 0; i < 4; ++i) {
            paths.directories[i].clear();
            paths.directories[i] << root + "/new/" + QString::number(i)
                                 << root + "/old/" + QString::number(i);
        }
    }
    void cleanup() {
        // Only files created by this test, never operational archives.
        for (int i = 0; i < 4; ++i) {
            foreach (const QString &directory, paths.directories[i]) {
                QDir dir(directory);
                foreach (const QString &name, dir.entryList(QDir::Files)) QVERIFY(dir.remove(name));
                QDir().rmdir(directory);
            }
        }
        QDir().rmdir(root + "/new"); QDir().rmdir(root + "/old"); QDir().rmdir(root);
    }
    void perDateFallbackAndPreferredFile() {
        const QDate date(2026,10,6);
        for (int i = 0; i < 4; ++i) {
            const Archive::Kind kind = Archive::Kind(i);
            const QString name = Archive::fileName(kind, date);
            const QString oldPath = paths.directories[i].at(1) + "/" + name;
            const QString newPath = paths.directories[i].at(0) + "/" + name;
            put(oldPath, "old");
            QCOMPARE(Archive::resolve(kind, date, paths), oldPath);
            put(newPath, "new");
            QCOMPARE(Archive::resolve(kind, date, paths), newPath);
        }
        QFile::remove(paths.directories[0].at(0) + "/filament-2026-10-06");
        const QString txt = paths.directories[0].at(0) + "/filament-2026-10-06.txt";
        put(txt, "new text");
        QCOMPARE(Archive::resolve(Archive::Filament, date, paths), txt);
    }
    void independentDevicesZeroAndInvalidTotals() {
        const QString path = root + "/new/2/xrayontime-2026-10-06.csv";
        put(path, on("00:00:00"));
        qint64 seconds = -1; QString error;
        QVERIFY(Archive::readOn(path, "ED01", &seconds, &error));
        QCOMPARE(seconds, qint64(0));
        QVERIFY(Archive::readOn(path, "ed2", &seconds, &error));
        QCOMPARE(seconds, qint64(32400));
        QVERIFY(!Archive::readOn(path, "ED03", &seconds, &error));
        QVERIFY(!error.isEmpty());
        put(path, on("24:00:01"));
        QVERIFY(!Archive::readOn(path, "ED01", &seconds, &error));
        put(path, on("01:65:00"));
        QVERIFY(!Archive::readOn(path, "ED01", &seconds, &error));
        put(path, on("01:00:00") + "ED_01\nTotal working time: 02:00:00\n");
        QVERIFY(!Archive::readOn(path, "ED01", &seconds, &error));
    }
    void lifetimeBoundariesGapsAndNoDoubleCounting() {
        put(root + "/new/1/tube-2026-10-07.csv", ledger());
        put(root + "/old/2/xrayontime-2026-10-02.csv", on("01:00:00"));
        put(root + "/new/2/xrayontime-2026-10-02.csv", on("02:00:00"));
        put(root + "/old/2/xrayontime-2026-10-04.csv", on("08:00:00"));
        put(root + "/new/2/xrayontime-2026-10-05.csv", on("00:00:00"));
        put(root + "/old/2/xrayontime-2026-10-06.csv", on("09:00:00"));
        put(root + "/new/2/xrayontime-2026-10-06.csv", "broken preferred file\n");
        put(root + "/old/2/xrayontime-2026-10-07.csv", on("03:30:00"));
        const Archive::Report r = Archive::load("ED01", "1-3", QDate(2026,10,7), paths);
        QVERIFY2(r.error.isEmpty(), qPrintable(r.error));
        QCOMPARE(r.record.removedSerial, QString("OLD"));
        QCOMPARE(r.removed.seconds, qint64(7200));
        QCOMPARE(r.removed.expectedDays, 2);
        QCOMPARE(r.removed.validDays, 1);
        QCOMPARE(r.removed.missingDays, 1);
        QCOMPARE(r.installed.seconds, qint64(12600));
        QCOMPARE(r.installed.expectedDays, 3);
        QCOMPARE(r.installed.validDays, 2);
        QCOMPARE(r.installed.invalidDays, 1);
        QCOMPARE(r.installed.missingDays, 0);
        QCOMPARE(r.days.size(), 5); // Only existing files; gaps stay gaps.
    }
    void historicalSnapshotRequiredAndDatesValidated() {
        put(root + "/new/1/tube-2026-10-07.csv", ledger());
        QVERIFY(!Archive::load("ED01", "1-3", QDate(2026,10,6), paths).error.isEmpty());
        const QString path = root + "/new/1/tube-2026-10-06.csv";
        put(path, ledger("2026-10-05", "2026-10-04"));
        QVERIFY(!Archive::load("ED01", "1-3", QDate(2026,10,6), paths).error.isEmpty());
        put(path, ledger("2026-10-01", "2026-10-07"));
        QVERIFY(!Archive::load("ED01", "1-3", QDate(2026,10,6), paths).error.isEmpty());
        put(path, ledger("Bos", "2026-10-04"));
        Archive::Report r = Archive::load("ED01", "1-3", QDate(2026,10,6), paths);
        QVERIFY(r.error.isEmpty());
        QVERIFY(!r.removed.known);
        QVERIFY(r.installed.known);
        put(path, ledger("2026-10-04", "2026-10-04"));
        r = Archive::load("ED01", "1-3", QDate(2026,10,6), paths);
        QVERIFY(r.error.isEmpty());
        QCOMPARE(r.removed.expectedDays, 0);
        put(path, ledger() + "ed1,1-3,OTHER,2026-10-01,NEW,2026-10-04\n");
        QVERIFY(!Archive::load("ED01", "1-3", QDate(2026,10,6), paths).error.isEmpty());
    }
    void preferredNonFileMustNotExposeLegacyTotal() {
        put(root + "/new/1/tube-2026-10-06.csv", ledger());
        put(root + "/old/2/xrayontime-2026-10-05.csv", on("09:00:00"));
        const QString preferred = root + "/new/2/xrayontime-2026-10-05.csv";
        QVERIFY(QDir().mkpath(preferred));
        const Archive::Report r = Archive::load("ED01", "1-3", QDate(2026,10,6), paths);
        QVERIFY(QDir().rmdir(preferred));
        QCOMPARE(r.installed.seconds, qint64(0));
        QCOMPARE(r.installed.invalidDays, 1);
        QCOMPARE(r.installed.missingDays, 1);
    }
};
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    ArchiveTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "archive_test.moc"
