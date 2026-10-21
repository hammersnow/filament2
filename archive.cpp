#include "archive.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QRegExp>
#include <QSettings>
#include <QTextStream>

namespace Archive {
Paths::Paths()
{
    directories[Filament] << "/opt/Eko/logs/filament/Daily";
    directories[Tubes] << "/opt/Eko/logs/tubelog/log" << "/root/tubelog/log";
    directories[On] << "/opt/Eko/logs/xrayontime/log" << "/root/Eko/xrayontime/log";
    directories[Off] << "/opt/Eko/logs/xrayontime/offlog" << "/root/Eko/xrayontime/offlog";
    QStringList configs;
    configs << QCoreApplication::applicationDirPath() + "/filament2.ini"
            << QDir::currentPath() + "/filament2.ini"
            << QCoreApplication::applicationDirPath() + "/../filament2.ini";
    foreach (const QString &path, configs) {
        if (!QFileInfo(path).exists()) continue;
        QSettings settings(path, QSettings::IniFormat);
        const QString legacy = settings.value("filament/daily_directory").toString().trimmed();
        if (!legacy.isEmpty()) directories[Filament] << legacy;
        break;
    }
    directories[Filament] << "/home/eds_cm/eko/filament/Daily";
    directories[Filament].removeDuplicates();
}

QString fileName(Kind kind, const QDate &date)
{
    const char *prefix[] = { "filament-", "tube-", "xrayontime-", "xrayofftime-" };
    return QString::fromLatin1(prefix[kind]) + date.toString("yyyy-MM-dd")
        + (kind == Filament ? "" : ".csv");
}

QString resolve(Kind kind, const QDate &date, const Paths &paths)
{
    const QString name = fileName(kind, date);
    foreach (const QString &directory, paths.directories[kind]) {
        const QString path = QDir(directory).filePath(name);
        if (QFileInfo(path).exists() || QFileInfo(path).isSymLink()) return path;
        if (kind == Filament && (QFileInfo(path + ".txt").exists() || QFileInfo(path + ".txt").isSymLink()))
            return path + ".txt";
    }
    return paths.directories[kind].isEmpty() ? QString()
        : QDir(paths.directories[kind].first()).filePath(name);
}

static QString deviceKey(QString device)
{
    device.remove('_');
    QRegExp rx("ed0*([1-9][0-9]*)", Qt::CaseInsensitive);
    return rx.exactMatch(device.trimmed()) ? "ed" + QString::number(rx.cap(1).toInt()) : QString();
}

static bool fail(QString *error, const QString &message)
{
    if (error) *error = message;
    return false;
}

bool readOn(const QString &path, const QString &device, qint64 *seconds, QString *error)
{
    if (error) error->clear();
    *seconds = 0;
    QFile file(path);
    if (!QFileInfo(path).isFile()) return fail(error, "Not a readable regular file: " + path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return fail(error, "Cannot read file: " + path + " (" + file.errorString() + ")");
    const QString key = deviceKey(device);
    if (key.isEmpty()) return fail(error, "Invalid device ID.");
    QTextStream in(&file);
    in.setCodec("UTF-8");
    QRegExp idRx("\\bED_?[0-9]{1,2}\\b", Qt::CaseInsensitive);
    QRegExp timeRx("([0-9]+):([0-9]{2}):([0-9]{2})\\s*$");
    QString current;
    int totals = 0;
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (idRx.indexIn(line) >= 0) current = deviceKey(idRx.cap(0));
        if (current == key && line.contains("Total", Qt::CaseInsensitive)) {
            if (++totals > 1) return fail(error, "Duplicate daily Total entries for " + device + ".");
            if (timeRx.indexIn(line) < 0)
                return fail(error, "Malformed daily total; expected HH:MM:SS: " + line);
            bool ok;
            const qint64 hours = timeRx.cap(1).toLongLong(&ok);
            const int minutes = timeRx.cap(2).toInt(), secs = timeRx.cap(3).toInt();
            if (!ok) return fail(error, "Invalid hours value: " + line);
            if (hours > 24) return fail(error, "Daily total exceeds 24 hours: " + line);
            if (minutes > 59 || secs > 59)
                return fail(error, "Minutes and seconds must be between 00 and 59: " + line);
            *seconds = hours * 3600 + minutes * 60 + secs;
            if (*seconds > 86400) return fail(error, "Daily total exceeds 24 hours: " + line);
        }
        if (line.contains("------")) current.clear();
    }
    if (file.error() != QFile::NoError) return fail(error, "File read error: " + file.errorString());
    return totals == 1 || fail(error, "No daily Total entry for " + device + ".");
}

static bool record(const QString &path, const QString &device, const QString &position,
                   const QDate &asOf, TubeRecord *result, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return fail(error, "Cannot read the selected date's tube archive: " + path);
    QTextStream in(&file);
    in.setCodec("UTF-8");
    const QString header = QString::fromUtf8("Cihaz,Tüp,Sökülen Tüp,Sökülen Tarih,Takilan Tüp,Takilan Tarih");
    if (in.readLine().trimmed() != header) return fail(error, "Invalid tube archive header.");
    int matches = 0;
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;
        const QStringList row = line.split(',', QString::KeepEmptyParts);
        if (row.size() != 6) return fail(error, "Malformed row in the tube archive; expected six columns.");
        if (deviceKey(row.at(0)) != deviceKey(device) || row.at(1).trimmed() != position) continue;
        if (++matches > 1) return fail(error, "Duplicate device/tube record.");
        result->removedSerial = row.at(2).trimmed();
        result->installedSerial = row.at(4).trimmed();
        result->removedSince = QDate::fromString(row.at(3).trimmed(), "yyyy-MM-dd");
        result->installedSince = QDate::fromString(row.at(5).trimmed(), "yyyy-MM-dd");
        if (result->installedSerial.isEmpty() || result->installedSerial == "Bos" || !result->installedSince.isValid())
            return fail(error, "The installed tube serial number or installation date is unknown.");
        if (row.at(3).trimmed() != "Bos" && !result->removedSince.isValid())
            return fail(error, "Invalid previous installation date.");
        if (result->installedSince > asOf || (result->removedSince.isValid() && result->removedSince > result->installedSince))
            return fail(error, "Tube dates are out of order or later than the selected date.");
    }
    if (file.error() != QFile::NoError) return fail(error, "Tube archive read error: " + file.errorString());
    return matches == 1 || fail(error, "The selected device/tube is not in the archive.");
}

static Period period(const QDate &start, const QDate &end)
{
    Period p;
    p.start = start; p.end = end;
    p.known = start.isValid() && end.isValid();
    if (p.known && start <= end) p.expectedDays = start.daysTo(end) + 1;
    return p;
}

Report load(const QString &device, const QString &position, const QDate &asOf, const Paths &paths)
{
    Report r;
    if (!asOf.isValid() || deviceKey(device).isEmpty()) {
        r.error = "Invalid device or date.";
        return r;
    }
    r.snapshotPath = resolve(Tubes, asOf, paths);
    if (!record(r.snapshotPath, device, position, asOf, &r.record, &r.error)) return r;
    const QDate lastDay = qMin(asOf,QDate::currentDate().addDays(-1));
    // Qt4 addDays() can turn an invalid date into a valid Julian date.
    r.removed = period(r.record.removedSince.isValid() ? r.record.removedSince.addDays(1) : QDate(),
                       qMin(r.record.installedSince.addDays(-1),lastDay));
    r.installed = period(r.record.installedSince.addDays(1), lastDay);
    const QDate first = r.record.removedSince.isValid() ? r.record.removedSince : r.record.installedSince;
    QMap<QDate, QString> files;
    // Index available dates instead of probing every day in a potentially long lifetime.
    foreach (const QString &directory, paths.directories[On]) {
        const QDir dir(directory);
        const QStringList names = dir.entryList(QStringList() << "xrayontime-*.csv",
                                                QDir::Files | QDir::Dirs | QDir::System | QDir::NoDotAndDotDot);
        foreach (const QString &name, names) {
            const QDate date = QDate::fromString(name.mid(11, 10), "yyyy-MM-dd");
            if (date.isValid() && name == fileName(On, date) && date >= first && date <= lastDay && !files.contains(date))
                files.insert(date, dir.filePath(name));
        }
    }
    for (QMap<QDate, QString>::const_iterator it = files.constBegin(); it != files.constEnd(); ++it) {
        Day day; day.date = it.key(); day.path = resolve(On,day.date,paths);
        readOn(day.path, device, &day.seconds, &day.error);
        r.days.append(day);
        Period *periods[] = { &r.removed, &r.installed };
        for (int i = 0; i < 2; ++i) {
            Period &p = *periods[i];
            if (!p.known || day.date < p.start || day.date > p.end) continue;
            if (day.error.isEmpty()) { p.seconds += day.seconds; ++p.validDays; }
            else ++p.invalidDays;
        }
    }
    r.removed.missingDays = r.removed.expectedDays - r.removed.validDays - r.removed.invalidDays;
    r.installed.missingDays = r.installed.expectedDays - r.installed.validDays - r.installed.invalidDays;
    return r;
}

QString duration(qint64 seconds)
{
    return QString("%1:%2:%3").arg(seconds / 3600).arg((seconds / 60) % 60, 2, 10, QChar('0'))
        .arg(seconds % 60, 2, 10, QChar('0'));
}
}
