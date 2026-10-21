#include "../mainwindow.cpp"
#include "../usagewidget.h"
#include <QtTest/QtTest>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryFile>
#include <QDir>
#include <QCalendarWidget>
#include <QDialog>
#include <QTimer>

class UsageGuiTest : public QObject {
    Q_OBJECT
    bool inspected;
    QString expectedDate, expectedReason, expectedPath;
public slots:
    void inspectAndCloseIssueDialog() {
        QDialog *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        QTableWidget *table = dialog->findChild<QTableWidget *>("usageIssueTable");
        if (!table) { dialog->reject(); QFAIL("Issue table not found"); }
        const bool correct = table->rowCount() == 1 && table->item(0,0)->text() == expectedDate
            && table->item(0,1)->text().contains(expectedReason) && table->item(0,2)->text() == expectedPath;
        inspected = correct;
        if (!qgetenv("FILAMENT_ISSUE_SCREENSHOT").isEmpty()) {
            QApplication::processEvents();
            QVERIFY(QPixmap::grabWidget(dialog).save(QString::fromLocal8Bit(qgetenv("FILAMENT_ISSUE_SCREENSHOT"))));
        }
        dialog->accept();
        QVERIFY(correct);
    }
private slots:
    void clickingCoverageExplainsAllPeriodErrorsInEnglish() {
        QTemporaryFile temp; QVERIFY(temp.open());
        const QString root = temp.fileName() + "-issues";
        QVERIFY(QDir().mkpath(root));
        Archive::Paths paths;
        for (int i=0;i<4;++i) paths.directories[i] = QStringList() << root;
        QFile snapshot(root + "/tube-2026-10-06.csv"); QVERIFY(snapshot.open(QIODevice::WriteOnly));
        snapshot.write(QString::fromUtf8("Cihaz,Tüp,Sökülen Tüp,Sökülen Tarih,Takilan Tüp,Takilan Tarih\ned1,1-3,OLD,2026-01-01,NEW,2026-01-04\n").toUtf8()); snapshot.close();
        QFile oldFile(root + "/xrayontime-2026-01-02.csv"); QVERIFY(oldFile.open(QIODevice::WriteOnly));
        oldFile.write("ED_01\nTotal working time: broken\n"); oldFile.close();
        QFile newFile(root + "/xrayontime-2026-10-05.csv"); QVERIFY(newFile.open(QIODevice::WriteOnly));
        newFile.write("ED_01\nTotal working time: 01:00:00\nTotal working time: 02:00:00\n"); newFile.close();
        UsageWidget widget(paths); widget.resize(1100,700); widget.show();
        widget.findChild<QDateEdit *>("usageDate")->setDate(QDate(2026,10,6));
        QApplication::processEvents();
        QTableWidget *summary = widget.findChild<QTableWidget *>("usageSummary"); QVERIFY(summary);
        for (int row=0;row<2;++row) {
            expectedDate = row ? "05.10.2026" : "02.01.2026";
            expectedReason = row ? "Duplicate" : "HH:MM:SS";
            expectedPath = row ? newFile.fileName() : oldFile.fileName();
            inspected = false;
            QTimer::singleShot(0,this,SLOT(inspectAndCloseIssueDialog()));
            QTest::mouseClick(summary->viewport(),Qt::LeftButton,Qt::NoModifier,
                              summary->visualItemRect(summary->item(row,5)).center());
            QVERIFY(inspected);
        }
        QCOMPARE(widget.findChild<QPushButton *>("usageRefresh")->text(),QString("Refresh"));
        QCOMPARE(summary->item(0,0)->text(),QString("Removed"));
        QCOMPARE(summary->item(1,0)->text(),QString("Installed"));
        QVERIFY(snapshot.remove()); QVERIFY(oldFile.remove()); QVERIFY(newFile.remove()); QVERIFY(QDir().rmdir(root));
    }
    void dropdownsAndCalendarHaveComfortableClickTargets() {
        UsageWidget widget((Archive::Paths()));
        widget.resize(1000,700); widget.show();
        QApplication::processEvents();
        QComboBox *device = widget.findChild<QComboBox *>("usageDevice"); QVERIFY(device);
        const QImage pixels = QPixmap::grabWidget(device).toImage();
        int brightArrowPixels = 0;
        for (int y=device->height()/2-6; y<=device->height()/2+6; ++y)
            for (int x=device->width()-24; x<device->width()-8; ++x)
                if (qRed(pixels.pixel(x,y)) > 160 && qGreen(pixels.pixel(x,y)) > 170) ++brightArrowPixels;
        QVERIFY(brightArrowPixels > 5);
        QTest::mouseClick(device,Qt::LeftButton,Qt::NoModifier,device->rect().center());
        QApplication::processEvents(); QVERIFY(device->view()->isVisible());
        if (!qgetenv("FILAMENT_POPUP_SCREENSHOT").isEmpty())
            QVERIFY(QPixmap::grabWidget(device->view()->window()).save(QString::fromLocal8Bit(qgetenv("FILAMENT_POPUP_SCREENSHOT"))));
        QTest::keyClick(device,Qt::Key_Down);
        QTest::keyClick(device,Qt::Key_Return);
        QCOMPARE(device->currentText(),QString("ED02"));
        QDateEdit *date = widget.findChild<QDateEdit *>("usageDate"); QVERIFY(date);
        // Click inside the wider calendar target, not just its tiny native arrow.
        QTest::mouseClick(date,Qt::LeftButton,Qt::NoModifier,QPoint(date->width()-26,date->height()/2));
        QApplication::processEvents(); QVERIFY(date->calendarWidget()->isVisible());
        if (!qgetenv("FILAMENT_CALENDAR_SCREENSHOT").isEmpty())
            QVERIFY(QPixmap::grabWidget(date->calendarWidget()->window()).save(QString::fromLocal8Bit(qgetenv("FILAMENT_CALENDAR_SCREENSHOT"))));
        QTest::keyClick(date->calendarWidget(),Qt::Key_Escape);
        QApplication::processEvents(); QVERIFY(!date->calendarWidget()->isVisible());
    }
    void validLegacyReadersStillShowMeasurements() {
        QTemporaryFile temp; QVERIFY(temp.open());
        temp.write(QString::fromUtf8("Cihaz,Tüp,Sökülen Tüp,Sökülen Tarih,Takilan Tüp,Takilan Tarih\ned1,1-3,OLD,2026-10-01,NEW,2026-10-04\n").toUtf8());
        temp.flush();
        QVERIFY(readTubeCsv(temp.fileName()).value("ed1").contains("OLD"));
        QVERIFY(readTubeCsv(temp.fileName()).value("ed1").contains("NEW"));
        temp.resize(0); temp.seek(0);
        temp.write("ED_02\nMax Downtime: 03:00:00\n------\nED_01\nMax Downtime: 02:30:00\n------\n");
        temp.flush();
        QCOMPARE(readSectionValues(temp.fileName(),"Max",true).value("ED_01"),QString("02:30:00"));
        temp.resize(0); temp.seek(0);
        temp.write("ED01(192.0.2.1) 2026-10-06\n1-3 4.010\n"); temp.flush();
        QMap<QString,MachineFilament> machines;
        QVERIFY(parseDailyFile(temp.fileName(), &machines));
        QCOMPARE(machines.value("ED01").readings.size(),1);
        QCOMPARE(machines.value("ED01").readings.at(0).valueText,QString("4.010"));
    }
    void missingAndMalformedFilesHaveVisibleErrors() {
        QTemporaryFile temp; QVERIFY(temp.open());
        const QString missing = temp.fileName() + "-missing";
        QVERIFY(!readTubeCsv(missing).value("ed1").isEmpty());
        QVERIFY(!readSectionValues(missing,"Max",true).value("ED_01").isEmpty());
        temp.write("broken archive\n"); temp.flush();
        QVERIFY(!readTubeCsv(temp.fileName()).value("ed1").isEmpty());
        QVERIFY(!readSectionValues(temp.fileName(),"Max",true).value("ED_01").isEmpty());
        temp.resize(0); temp.seek(0);
        temp.write("ED01(192.0.2.1) 2026-10-06\n"); temp.flush();
        QMap<QString,MachineFilament> machines;
        QVERIFY(!parseDailyFile(temp.fileName(), &machines));
        QVERIFY(machines.isEmpty());
    }
    void mainWindowIncludesUsageTab() {
        MainWindow window;
        QTabWidget *tabs = window.findChild<QTabWidget *>("tabWidget");
        QVERIFY(tabs);
        QCOMPARE(tabs->count(), 3);
        QVERIFY(window.findChild<QWidget *>("tubeUsagePage"));
    }
    void refreshShowsBoundariesMissingDaysAndSources() {
        QTemporaryFile temp; QVERIFY(temp.open());
        const QString root = temp.fileName() + "-gui";
        QVERIFY(QDir().mkpath(root));
        Archive::Paths paths;
        for (int i=0; i<4; ++i) paths.directories[i] = QStringList() << root;
        const QString snapshot = root + "/tube-2026-10-06.csv";
        QFile file(snapshot); QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QString::fromUtf8("Cihaz,Tüp,Sökülen Tüp,Sökülen Tarih,Takilan Tüp,Takilan Tarih\ned1,1-3,OLD,2026-10-01,NEW,2026-10-04\n").toUtf8());
        file.close();
        QFile on(root + "/xrayontime-2026-10-05.csv"); QVERIFY(on.open(QIODevice::WriteOnly));
        on.write("ED_01\nTotal working time: 02:30:00\n------\n"); on.close();
        UsageWidget widget(paths);
        widget.resize(1000,650); widget.show();
        QDateEdit *date = widget.findChild<QDateEdit *>("usageDate"); QVERIFY(date);
        date->setDate(QDate(2026,10,6));
        QTest::mouseClick(widget.findChild<QPushButton *>("usageRefresh"), Qt::LeftButton);
        QTableWidget *summary = widget.findChild<QTableWidget *>("usageSummary"); QVERIFY(summary);
        QCOMPARE(summary->rowCount(), 2);
        QCOMPARE(summary->item(0,1)->text(), QString("OLD"));
        QCOMPARE(summary->item(1,1)->text(), QString("NEW"));
        QCOMPARE(summary->item(1,4)->text(), QString("2:30:00"));
        QVERIFY(summary->item(1,5)->text().contains("1 / 2"));
        QLabel *status = widget.findChild<QLabel *>("usageStatus"); QVERIFY(status);
        QCOMPARE(status->toolTip(),snapshot);
        QTableWidget *days = widget.findChild<QTableWidget *>("usageDays"); QVERIFY(days);
        QCOMPARE(days->rowCount(), 6);
        QVERIFY(days->item(3,1)->text().contains("Replacement"));
        QProgressBar *bar = qobject_cast<QProgressBar *>(days->cellWidget(4,2)); QVERIFY(bar);
        QCOMPARE(bar->value(), 9000);
        QVERIFY(days->item(4,3)->text().endsWith("xrayontime-2026-10-05.csv"));
        QCOMPARE(days->item(4,3)->toolTip(),root + "/xrayontime-2026-10-05.csv");
        QVERIFY(days->item(5,2)->text().contains("No data"));
        // Changing device refreshes the view; previous totals must disappear.
        widget.findChild<QComboBox *>("usageDevice")->setCurrentIndex(1);
        QTest::mouseClick(widget.findChild<QPushButton *>("usageRefresh"), Qt::LeftButton);
        QCOMPARE(summary->rowCount(), 0);
        QCOMPARE(days->rowCount(), 0);
        QVERIFY(!status->text().isEmpty());
        widget.findChild<QComboBox *>("usageDevice")->setCurrentIndex(0);
        QTest::mouseClick(widget.findChild<QPushButton *>("usageRefresh"), Qt::LeftButton);
        QApplication::processEvents();
        if (qgetenv("FILAMENT_SCREENSHOT").size())
            QVERIFY(QPixmap::grabWidget(&widget).save(QString::fromLocal8Bit(qgetenv("FILAMENT_SCREENSHOT"))));
        QVERIFY(file.remove()); QVERIFY(on.remove()); QVERIFY(QDir().rmdir(root));
    }
};
QTEST_MAIN(UsageGuiTest)
#include "usage_gui_test.moc"
