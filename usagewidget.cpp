#include "usagewidget.h"
#include "controlstyle.h"
#include <QApplication>
#include <QComboBox>
#include <QDateEdit>
#include <QHeaderView>
#include <QLabel>
#include <QMap>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLocale>

UsageWidget::UsageWidget(const Archive::Paths &archivePaths, QWidget *parent)
    : QWidget(parent), paths(archivePaths)
{
    setObjectName("tubeUsagePage");
    QFont textFont = font(); textFont.setPointSize(10); setFont(textFont);
    setStyleSheet(selectorStyle() + "UsageWidget{background:#151a20;}"
        "QLabel{color:#d8dee9;}"
        "QPushButton{background:#345573;color:#fff;border:1px solid #6386a5;border-radius:6px;padding:6px 16px;min-height:26px;}"
        "QPushButton:hover{background:#426d92;}QPushButton:pressed{background:#29465f;}"
        "QPushButton:focus{border:1px solid #acd7f5;}"
        "QTableWidget{background:#1b2430;alternate-background-color:#202c39;color:#eef2f6;"
        "gridline-color:#344353;selection-background-color:#365674;}"
        "QTableWidget::item{padding:5px 8px;}"
        "QHeaderView::section{background:#2b3b4b;color:#eef2f6;border:0;padding:9px 8px;font-weight:bold;}"
        "QProgressBar{background:#111e2b;color:#fff;border:1px solid #42596f;"
        "border-radius:4px;margin:5px;text-align:center;}"
        "QProgressBar::chunk{background:#39779f;border-radius:3px;}");
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16,16,16,16); layout->setSpacing(12);
    QLabel *title = new QLabel(QString::fromUtf8("Tube Usage"), this);
    title->setStyleSheet("font-size:20px;font-weight:bold;"); layout->addWidget(title);
    QLabel *note = new QLabel(QString::fromUtf8(
        "Recorded device X-ray On during each tube installation period.\n"
        "Installation/replacement dates and today are excluded. Missing files are not zero."), this);
    note->setWordWrap(true); layout->addWidget(note);
    QHBoxLayout *controls = new QHBoxLayout;
    device = new QComboBox(this); device->setObjectName("usageDevice");
    device->setMinimumWidth(112);
    device->addItems(QString("ED01 ED02 ED03 ED04 ED05 ED06 ED09 ED10 ED11 ED12 ED13 ED14 ED15 ED16 ED17 ED18 ED19 ED20 ED23 ED24 ED25 ED26 ED27 ED28").split(' '));
    position = new QComboBox(this); position->setObjectName("usagePosition");
    position->setMinimumWidth(100);
    position->addItems(QString("1-3 1-1 1-2 2-2 2-3 2-1 3-1 3-2 3-3").split(' '));
    date = new QDateEdit(QDate::currentDate(), this); date->setObjectName("usageDate");
    date->setCalendarPopup(true); date->setDisplayFormat("dd.MM.yyyy"); date->setMaximumDate(QDate::currentDate());
    prepareCalendar(date);
    date->setLocale(QLocale(QLocale::English,QLocale::UnitedKingdom));
    date->setMinimumWidth(160);
    QLabel *deviceLabel = new QLabel(QString::fromUtf8("Device"), this); deviceLabel->setBuddy(device);
    QLabel *positionLabel = new QLabel(QString::fromUtf8("Tube"), this); positionLabel->setBuddy(position);
    QLabel *dateLabel = new QLabel(QString::fromUtf8("Archive date"), this); dateLabel->setBuddy(date);
    controls->addWidget(deviceLabel); controls->addWidget(device);
    controls->addSpacing(12); controls->addWidget(positionLabel); controls->addWidget(position);
    controls->addSpacing(12); controls->addWidget(dateLabel); controls->addWidget(date);
    QPushButton *refresh = new QPushButton(QString::fromUtf8("Refresh"), this);
    refresh->setObjectName("usageRefresh"); controls->addWidget(refresh); controls->addStretch();
    layout->addLayout(controls);
    status = new QLabel(this); status->setObjectName("usageStatus");
    status->setTextFormat(Qt::PlainText); status->setWordWrap(true);
    status->setTextInteractionFlags(Qt::TextSelectableByMouse); layout->addWidget(status);
    summary = new QTableWidget(0,6,this); summary->setObjectName("usageSummary");
    summary->setHorizontalHeaderLabels(QString::fromUtf8("Status|Serial number|Installed on|Period end|X-ray On|Day coverage").split('|'));
    summary->setEditTriggers(QAbstractItemView::NoEditTriggers);
    summary->setAlternatingRowColors(true); summary->setSelectionBehavior(QAbstractItemView::SelectRows);
    summary->verticalHeader()->hide(); summary->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    summary->horizontalHeader()->setResizeMode(0,QHeaderView::ResizeToContents);
    summary->horizontalHeader()->setResizeMode(2,QHeaderView::ResizeToContents);
    summary->horizontalHeader()->setResizeMode(3,QHeaderView::ResizeToContents);
    summary->horizontalHeader()->setResizeMode(4,QHeaderView::ResizeToContents);
    summary->setFixedHeight(124); layout->addWidget(summary);
    QLabel *legend = new QLabel(QString::fromUtf8(
        "Daily view • last 90 calendar days • bars scaled to 24 hours • totals cover the full period.\n"
        "Duration: hours:minutes:seconds. Click invalid coverage or a daily issue for details."), this);
    legend->setWordWrap(true); layout->addWidget(legend);
    days = new QTableWidget(0,4,this); days->setObjectName("usageDays");
    days->setHorizontalHeaderLabels(QString::fromUtf8("Date|Period|X-ray On / 24 hours|Source file").split('|'));
    days->setEditTriggers(QAbstractItemView::NoEditTriggers); days->verticalHeader()->hide();
    days->setAlternatingRowColors(true); days->setSelectionBehavior(QAbstractItemView::SelectRows);
    days->horizontalHeader()->setResizeMode(0,QHeaderView::ResizeToContents);
    days->horizontalHeader()->setResizeMode(1,QHeaderView::ResizeToContents);
    days->horizontalHeader()->setResizeMode(2,QHeaderView::Stretch);
    days->horizontalHeader()->setResizeMode(3,QHeaderView::Stretch);
    days->setTextElideMode(Qt::ElideMiddle); layout->addWidget(days,1);
    connect(refresh,SIGNAL(clicked()),this,SLOT(reload()));
    connect(device,SIGNAL(currentIndexChanged(int)),this,SLOT(reload()));
    connect(position,SIGNAL(currentIndexChanged(int)),this,SLOT(reload()));
    connect(date,SIGNAL(dateChanged(QDate)),this,SLOT(reload()));
    connect(summary,SIGNAL(cellClicked(int,int)),this,SLOT(showIssues(int,int)));
    connect(summary,SIGNAL(cellActivated(int,int)),this,SLOT(showIssues(int,int)));
    connect(days,SIGNAL(cellClicked(int,int)),this,SLOT(showIssues(int,int)));
    connect(days,SIGNAL(cellActivated(int,int)),this,SLOT(showIssues(int,int)));
    reload();
}

void UsageWidget::reload()
{
    summary->setRowCount(0); days->setRowCount(0);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    report = Archive::load(device->currentText(),position->currentText(),date->date(),paths);
    QApplication::restoreOverrideCursor();
    if (!report.error.isEmpty()) {
        status->setText(report.error.section(": ",0,0)); status->setToolTip(report.error);
        status->setStyleSheet("color:#ffb56b;");
        return;
    }
    status->setStyleSheet("color:#b5c6d6;");
    status->setText(QString::fromUtf8("%1 / %2 • Archive: %3 • Day coverage: valid / expected")
        .arg(device->currentText()).arg(position->currentText()).arg(date->date().toString("dd.MM.yyyy")));
    status->setToolTip(report.snapshotPath);
    summary->setRowCount(2);
    for (int row=0; row<2; ++row) {
        const Archive::Period &p = row == 0 ? report.removed : report.installed;
        const QString serial = row == 0 ? report.record.removedSerial : report.record.installedSerial;
        const QDate installed = row == 0 ? report.record.removedSince : report.record.installedSince;
        const QDate end = row == 0 ? report.record.installedSince : report.installed.end;
        QString coverage = !p.known ? QString::fromUtf8("Installation date unknown") : p.expectedDays == 0
            ? QString::fromUtf8("No complete days")
            : QString::fromUtf8("%1 / %2 • missing %3 • invalid %4").arg(p.validDays).arg(p.expectedDays).arg(p.missingDays).arg(p.invalidDays);
        QStringList cells;
        cells << (row == 0 ? QString::fromUtf8("Removed") : QString::fromUtf8("Installed"))
              << (serial.isEmpty() || serial == "Bos" ? "Unknown" : serial)
              << (installed.isValid() ? installed.toString("dd.MM.yyyy") : "--")
              << (installed > end ? "--" : end.toString("dd.MM.yyyy"))
              << (p.validDays ? Archive::duration(p.seconds) : "--") << coverage;
        for (int col=0; col<cells.size(); ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(cells.at(col));
            item->setToolTip(cells.at(col)); summary->setItem(row,col,item);
        }
        if (p.invalidDays) {
            QTableWidgetItem *item = summary->item(row,5);
            QFont linkFont = item->font(); linkFont.setUnderline(true); item->setFont(linkFont);
            item->setTextColor(QColor("#91cdf8"));
            item->setToolTip("Click or press Enter to see invalid dates, reasons and source files for the full period.");
        }
        summary->setRowHeight(row,42);
    }
    QMap<QDate,Archive::Day> byDate;
    foreach (const Archive::Day &day, report.days) byDate.insert(day.date,day);
    const QDate first = report.record.removedSince.isValid() ? report.record.removedSince : report.record.installedSince;
    const QDate chartEnd = report.installed.end;
    const QDate chartStart = qMax(first,chartEnd.addDays(-89));
    days->setRowCount(qMax(0,chartStart.daysTo(chartEnd)+1));
    for (int row=0; row<days->rowCount(); ++row) {
        const QDate dayDate = chartStart.addDays(row);
        const bool boundary = dayDate == report.record.installedSince || dayDate == report.record.removedSince;
        const QString period = boundary ? QString::fromUtf8("Replacement / installation *")
            : dayDate < report.record.installedSince ? QString::fromUtf8("Removed tube period") : QString::fromUtf8("Installed tube period");
        days->setItem(row,0,new QTableWidgetItem(dayDate.toString("dd.MM.yyyy")));
        days->item(row,0)->setData(Qt::UserRole,dayDate);
        days->setItem(row,1,new QTableWidgetItem(period));
        if (boundary) days->item(row,1)->setTextColor(QColor("#ffd278"));
        const bool found = byDate.contains(dayDate);
        const Archive::Day day = byDate.value(dayDate);
        const QString path = found ? day.path : Archive::resolve(Archive::On,dayDate,paths);
        QTableWidgetItem *source = new QTableWidgetItem(QFileInfo(path).fileName());
        source->setTextColor(QColor("#b5c6d6")); source->setToolTip(path); days->setItem(row,3,source);
        if (found && day.error.isEmpty()) {
            QProgressBar *bar = new QProgressBar(days);
            bar->setRange(0,86400); bar->setValue(int(day.seconds)); bar->setFormat(Archive::duration(day.seconds));
            bar->setAccessibleName(dayDate.toString("yyyy-MM-dd") + " " + Archive::duration(day.seconds));
            bar->setToolTip(boundary ? QString::fromUtf8("Installation time unknown; excluded from totals.") : period);
            days->setCellWidget(row,2,bar);
        } else {
            const QString message = found ? day.error : QString::fromUtf8("No data");
            QTableWidgetItem *item = new QTableWidgetItem(message);
            item->setToolTip(message + "\nClick or press Enter for details."); days->setItem(row,2,item);
        }
        days->setRowHeight(row,36);
    }
}

void UsageWidget::showIssues(int row, int column)
{
    QList<Archive::Day> issues;
    if (sender() == summary) {
        if (column != 5 || row < 0 || row > 1) return;
        const Archive::Period &period = row == 0 ? report.removed : report.installed;
        foreach (const Archive::Day &day, report.days)
            if (period.known && day.date >= period.start && day.date <= period.end && !day.error.isEmpty())
                issues.append(day);
    } else {
        if (column != 2 || row < 0 || !days->item(row,0)) return;
        const QDate selected = days->item(row,0)->data(Qt::UserRole).toDate();
        bool found = false;
        foreach (const Archive::Day &day, report.days) {
            if (day.date != selected) continue;
            found = true;
            if (!day.error.isEmpty()) issues.append(day);
        }
        if (!found) {
            Archive::Day day; day.date = selected;
            day.path = Archive::resolve(Archive::On,selected,paths);
            day.error = "No daily archive file found."; issues.append(day);
        }
    }
    if (issues.isEmpty()) return;
    QDialog dialog(this);
    dialog.setObjectName("usageIssueDialog"); dialog.setWindowTitle("X-ray On Record Details");
    dialog.setStyleSheet(styleSheet() + "QDialog{background:#151a20;}");
    dialog.resize(1000,420);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QLabel *note = new QLabel(device->currentText() + " / " + position->currentText()
        + " - These records are excluded from the total. Missing files are counted separately.",&dialog);
    note->setWordWrap(true); layout->addWidget(note);
    QTableWidget *table = new QTableWidget(issues.size(),3,&dialog);
    table->setObjectName("usageIssueTable");
    table->setHorizontalHeaderLabels(QString("Date|Reason|Source file").split('|'));
    table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows); table->verticalHeader()->hide();
    table->horizontalHeader()->setResizeMode(0,QHeaderView::ResizeToContents);
    table->horizontalHeader()->setResizeMode(1,QHeaderView::Stretch);
    table->horizontalHeader()->setResizeMode(2,QHeaderView::Stretch);
    for (int i=0;i<issues.size();++i) {
        const Archive::Day &day = issues.at(i);
        QStringList cells; cells << day.date.toString("dd.MM.yyyy") << day.error << day.path;
        for (int col=0;col<cells.size();++col) {
            QTableWidgetItem *item = new QTableWidgetItem(cells.at(col));
            item->setToolTip(cells.at(col)); table->setItem(i,col,item);
        }
        table->setRowHeight(i,72);
    }
    layout->addWidget(table);
    QDialogButtonBox *buttons = new QDialogButtonBox(&dialog);
    buttons->addButton("Close",QDialogButtonBox::RejectRole);
    connect(buttons,SIGNAL(rejected()),&dialog,SLOT(reject())); layout->addWidget(buttons);
    dialog.exec();
}
