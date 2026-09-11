#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QDate>
#include <QDateEdit>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QList>
#include <QMap>
#include <QProcess>
#include <QPushButton>
#include <QRegExp>
#include <QScrollArea>
#include <QString>
#include <QStringList>
#include <QTabWidget>
#include <QTextBrowser>
#include <QTextStream>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>

namespace
{

struct FilamentReading
{
    QString tube;
    QString valueText;
    double value;
};

struct MachineFilament
{
    QString id;
    QString ip;
    QString date;
    QList<FilamentReading> readings;
};

QStringList allDevices()
{
    QStringList ids;
    ids << "ED01" << "ED02" << "ED03" << "ED04" << "ED05" << "ED06"
        << "ED09" << "ED10" << "ED11" << "ED12" << "ED13" << "ED14"
        << "ED15" << "ED16" << "ED17" << "ED18" << "ED19" << "ED20"
        << "ED23" << "ED24" << "ED25" << "ED26" << "ED27" << "ED28";
    return ids;
}

QStringList networkADevices()
{
    QStringList ids;
    ids << "ED01" << "ED03" << "ED05" << "ED09" << "ED11" << "ED13"
        << "ED15" << "ED17" << "ED19" << "ED23" << "ED25" << "ED27";
    return ids;
}

QStringList networkBDevices()
{
    QStringList ids;
    ids << "ED02" << "ED04" << "ED06" << "ED10" << "ED12" << "ED14"
        << "ED16" << "ED18" << "ED20" << "ED24" << "ED26" << "ED28";
    return ids;
}

QStringList tubeIds()
{
    QStringList ids;
    ids << "ed1" << "ed2" << "ed3" << "ed4" << "ed5" << "ed6"
        << "ed9" << "ed10" << "ed11" << "ed12" << "ed13" << "ed14"
        << "ed15" << "ed16" << "ed17" << "ed18" << "ed19" << "ed20"
        << "ed23" << "ed24" << "ed25" << "ed26" << "ed27" << "ed28";
    return ids;
}

QStringList xrayIds()
{
    QStringList ids;
    ids << "ED_01" << "ED_02" << "ED_03" << "ED_04" << "ED_05" << "ED_06"
        << "ED_09" << "ED_10" << "ED_11" << "ED_12" << "ED_13" << "ED_14"
        << "ED_15" << "ED_16" << "ED_17" << "ED_18" << "ED_19" << "ED_20"
        << "ED_23" << "ED_24" << "ED_25" << "ED_26" << "ED_27" << "ED_28";
    return ids;
}

bool isNetworkA(const QString &id)
{
    return networkADevices().contains(id);
}

bool isNetworkB(const QString &id)
{
    return networkBDevices().contains(id);
}

QString dailyFilePath(const QDate &date)
{
    QString directory;
    QStringList paths;
    paths << QCoreApplication::applicationDirPath() + "/filament2.ini";
    paths << QDir::currentPath() + "/filament2.ini";
    paths << QCoreApplication::applicationDirPath() + "/../filament2.ini";
    for (int i = 0; i < paths.size(); ++i) {
        if (!QFileInfo(paths.at(i)).exists()) continue;
        QSettings config(paths.at(i), QSettings::IniFormat);
        directory = config.value("filament/daily_directory").toString().trimmed();
        break;
    }
    if (directory.isEmpty()) return QString();
    const QString base = directory + "/filament-" +
                         date.toString("yyyy-MM-dd");

    if (QFileInfo(base).exists())
        return base;

    if (QFileInfo(base + ".txt").exists())
        return base + ".txt";

    return base;
}

QString colorStyleForValue(double value)
{
    if (value >= 3.250 && value <= 3.599)
        return "color:#ff5f56;font-weight:600;";
    if (value >= 3.600 && value <= 3.799)
        return "color:#ff9f43;font-weight:600;";
    if (value >= 3.800 && value <= 3.999)
        return "color:#ffd866;font-weight:600;";
    if (value >= 4.000 && value <= 4.600)
        return "color:#6bd968;font-weight:600;";

    return "color:#aeb8c2;";
}

bool parseDailyFile(const QString &path, QMap<QString, MachineFilament> *machines)
{
    if (machines == 0)
        return false;

    machines->clear();

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);
    QRegExp headerRx("^\\s*(ED[0-9]{2})\\(([^\\)]+)\\)\\s+([0-9]{4}-[0-9]{2}-[0-9]{2})\\s*$");
    QRegExp tubeRx("^[0-9]+-[0-9]+$");

    QString currentId;

    while (!in.atEnd())
    {
        const QString line = in.readLine().trimmed();

        if (line.isEmpty())
            continue;

        if (headerRx.exactMatch(line))
        {
            currentId = headerRx.cap(1).toUpper();

            MachineFilament machine;
            machine.id = currentId;
            machine.ip = headerRx.cap(2).trimmed();
            machine.date = headerRx.cap(3).trimmed();
            (*machines)[currentId] = machine;
            continue;
        }

        if (currentId.isEmpty() || !machines->contains(currentId))
            continue;

        const QStringList parts = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
        if (parts.size() < 2 || !tubeRx.exactMatch(parts.at(0)))
            continue;

        bool ok = false;
        const double value = parts.at(1).toDouble(&ok);
        if (!ok)
            continue;

        FilamentReading reading;
        reading.tube = parts.at(0);
        reading.valueText = parts.at(1);
        reading.value = value;

        MachineFilament machine = machines->value(currentId);
        machine.readings.append(reading);
        (*machines)[currentId] = machine;
    }

    file.close();
    return !machines->isEmpty();
}

int attentionCount(const MachineFilament &machine)
{
    int count = 0;
    for (int i = 0; i < machine.readings.size(); ++i)
    {
        const double value = machine.readings.at(i).value;
        if (value >= 3.000 && value < 4.000)
            ++count;
    }
    return count;
}

QString machineHtml(const MachineFilament &machine)
{
    QString html;
    html += "<div style=\"font-size:10pt;line-height:118%;\">";

    for (int i = 0; i < machine.readings.size(); ++i)
    {
        const FilamentReading &reading = machine.readings.at(i);
        html += "<span style=\"color:#d8dee9;\">" + reading.tube + "</span>";
        html += "&nbsp;&nbsp;";
        html += "<span style=\"" + colorStyleForValue(reading.value) + "\">" +
                reading.valueText + "</span><br>";
    }

    if (machine.readings.isEmpty())
        html += "<span style=\"color:#7f8c98;\">VERI YOK</span>";

    html += "</div>";
    return html;
}

void setIndicator(QLabel *label, int issueCount, bool hasData)
{
    if (label == 0)
        return;

    label->setText(QString());
    label->setAlignment(Qt::AlignCenter);
    label->setMinimumSize(10, 10);
    label->setMaximumSize(10, 10);
    label->setContentsMargins(0, 0, 0, 0);

    if (!hasData)
    {
        label->setStyleSheet("QLabel{background:transparent;border:0px;}");
        label->setToolTip("Veri yok");
    }
    else if (issueCount > 0)
    {
        label->setStyleSheet(
            "QLabel{background:#ff5f56;border:1px solid #ff8a85;border-radius:5px;}"
        );
        label->setToolTip(QString("%1 adet 3.xxx filament degeri").arg(issueCount));
    }
    else
    {
        label->setStyleSheet("QLabel{background:transparent;border:0px;}");
        label->setToolTip("3.xxx filament degeri yok");
    }
}

QMap<QString, QString> readTubeCsv(const QString &path)
{
    QMap<QString, QString> output;
    const QStringList ids = tubeIds();
    for (int i = 0; i < ids.size(); ++i)
        output.insert(ids.at(i), QString());

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return output;

    QTextStream in(&file);
    while (!in.atEnd())
    {
        const QString line = in.readLine();
        const QStringList parts = line.split(',', QString::KeepEmptyParts);
        if (parts.size() < 6)
            continue;

        const QString id = parts.at(0).trimmed().toLower();
        if (!output.contains(id))
            continue;

        const QString formatted =
            QString("%1 %2 %3 %4 %5")
                .arg(parts.at(1).trimmed(), -6)
                .arg(parts.at(2).trimmed(), -15)
                .arg(parts.at(3).trimmed(), -15)
                .arg(parts.at(4).trimmed(), -15)
                .arg(parts.at(5).trimmed(), -15);

        QString current = output.value(id);
        if (!current.isEmpty())
            current += "\n";
        current += formatted;
        output[id] = current;
    }

    file.close();
    return output;
}

QMap<QString, QString> readSectionValues(const QString &path,
                                         const QString &keyword,
                                         bool lastFieldOnly)
{
    QMap<QString, QString> output;
    const QStringList ids = xrayIds();
    for (int i = 0; i < ids.size(); ++i)
        output.insert(ids.at(i), QString());

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return output;

    QTextStream in(&file);
    QRegExp idRx("(ED_[0-9]{2})");
    QString currentId;

    while (!in.atEnd())
    {
        const QString line = in.readLine().trimmed();

        if (idRx.indexIn(line) >= 0)
        {
            const QString foundId = idRx.cap(1);
            if (output.contains(foundId))
                currentId = foundId;
        }

        if (!currentId.isEmpty() && line.contains(keyword))
        {
            QString value = line;
            if (lastFieldOnly)
            {
                const QStringList parts = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
                value = parts.isEmpty() ? QString() : parts.last();
            }

            if (!value.isEmpty())
            {
                QString current = output.value(currentId);
                if (!current.isEmpty())
                    current += "\n";
                current += value;
                output[currentId] = current;
            }
        }

        if (line.contains("------"))
            currentId.clear();
    }

    file.close();
    return output;
}

QFont normalContentFont()
{
    QFont font = QApplication::font();
    font.setPointSize(10);
    font.setBold(false);
    return font;
}

void styleTubeLabel(QLabel *label, const QString &text, int kind)
{
    if (!label)
        return;

    label->setFont(normalContentFont());
    label->setTextInteractionFlags(Qt::NoTextInteraction);
    label->setWordWrap(true);

    if (text.trimmed().isEmpty())
    {
        label->setText(QString());
        label->setStyleSheet(
            "QLabel{background:rgba(12,14,16,120);color:#aeb8c2;border:1px solid rgba(70,81,93,120);"
            "border-radius:5px;padding:4px;}"
        );
        return;
    }

    if (kind == 0)
    {
        label->setText(text);
        label->setStyleSheet(
            "QLabel{background:#20262e;color:#d8dee9;border:1px solid #3a4652;"
            "border-radius:5px;font-size:11px;padding:4px;}"
        );
    }
    else if (kind == 1)
    {
        label->setText(text);
        label->setStyleSheet(
            "QLabel{background:#18291d;color:#8be28b;border:1px solid #315b39;"
            "border-radius:5px;font-weight:bold;padding:4px;}"
        );
    }
    else
    {
        label->setText("Max Downtime: " + text);
        label->setStyleSheet(
            "QLabel{background:#301b1b;color:#ff8a85;border:1px solid #713333;"
            "border-radius:5px;font-weight:bold;padding:4px;}"
        );
    }

    label->adjustSize();
}

} // namespace


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->dateEdit->setDate(QDate::currentDate());
    ui->dateEdit_2->setDate(QDate::currentDate());

    this->setStyleSheet(
        "QMainWindow{"
        "background-image:url(:/foto/back);"
        "background-repeat:no-repeat;"
        "background-position:top right;"
        "background-color:#151a20;"
        "}"
        "QPushButton{background:#2b333d;color:#eef2f6;border:1px solid #46515d;"
        "border-radius:6px;padding:8px 16px;font-weight:600;min-height:34px;}"
        "QPushButton:hover{background:#36414d;}"
        "QPushButton:pressed{background:#222a32;}"
        "QDateEdit{background:#20262e;color:#eef2f6;border:1px solid #46515d;"
        "border-radius:5px;padding:4px 8px;min-height:32px;}"
        "QCheckBox{color:#d8dee9;font-weight:600;spacing:6px;}"
        "QCheckBox:disabled{color:#d8dee9;}"
        "QScrollArea{border:0;background:transparent;}"
    );

    ui->tabWidget->setStyleSheet(
        "QTabWidget::pane{"
        "border:1px solid #343d47;"
        "background-image:url(:/foto/back);"
        "background-repeat:no-repeat;"
        "background-position:top right;"
        "background-color:#151a20;"
        "}"
        "QTabBar::tab{background:#232a32;color:#c7d0d9;padding:8px 16px;"
        "border:1px solid #343d47;border-bottom:0;min-width:120px;}"
        "QTabBar::tab:selected{background:#313b46;color:white;}"
    );

    ui->yesil->setStyleSheet(
        "QLabel{background:#163b22;color:#7cff96;border:2px solid #4ad66d;"
        "border-radius:7px;font-size:16px;font-weight:800;padding:0px;}"
    );
    ui->sari->setStyleSheet(
        "QLabel{background:#40370d;color:#ffe75c;border:2px solid #e3c341;"
        "border-radius:7px;font-size:16px;font-weight:800;padding:0px;}"
    );
    ui->orange->setStyleSheet(
        "QLabel{background:#43270f;color:#ffad5c;border:2px solid #ff9f43;"
        "border-radius:7px;font-size:16px;font-weight:800;padding:0px;}"
    );
    ui->redco->setStyleSheet(
        "QLabel{background:#401818;color:#ff7770;border:2px solid #ff5f56;"
        "border-radius:7px;font-size:16px;font-weight:800;padding:0px;}"
    );

    ui->yesil->setAlignment(Qt::AlignCenter);
    ui->sari->setAlignment(Qt::AlignCenter);
    ui->orange->setAlignment(Qt::AlignCenter);
    ui->redco->setAlignment(Qt::AlignCenter);

    ui->yesil->setContentsMargins(0, 0, 0, 0);
    ui->sari->setContentsMargins(0, 0, 0, 0);
    ui->orange->setContentsMargins(0, 0, 0, 0);
    ui->redco->setContentsMargins(0, 0, 0, 0);

    ui->yesil->setMinimumHeight(34);
    ui->sari->setMinimumHeight(34);
    ui->orange->setMinimumHeight(34);
    ui->redco->setMinimumHeight(34);

    ui->checkBoxNetworkA->setText("Network A");
    ui->checkBoxNetworkB->setText("Network B");
    ui->checkBoxNetworkA->setEnabled(false);
    ui->checkBoxNetworkB->setEnabled(false);
    ui->checkBoxNetworkA->setMinimumWidth(110);
    ui->checkBoxNetworkB->setMinimumWidth(110);
    ui->checkBoxNetworkA->setToolTip("Daily dosyasi iceriginden otomatik belirlenir.");
    ui->checkBoxNetworkB->setToolTip("Daily dosyasi iceriginden otomatik belirlenir.");

    ui->pushButton->setMinimumWidth(110);
    ui->pushButton->setMinimumHeight(34);
    ui->pushButton_2->setMinimumWidth(130);
    ui->pushButton_2->setMinimumHeight(34);
    ui->pushButton_2->setText("Current Date");
    ui->dateEdit->setMinimumWidth(120);
    ui->dateEdit->setMinimumHeight(32);

    ui->pushButton_3->setMinimumWidth(130);
    ui->pushButton_3->setMinimumHeight(34);
    ui->pushButton_4->setMinimumWidth(130);
    ui->pushButton_4->setMinimumHeight(34);
    ui->dateEdit_2->setMinimumWidth(120);
    ui->dateEdit_2->setMinimumHeight(32);

    const QStringList devices = allDevices();
    for (int i = 0; i < devices.size(); ++i)
    {
        const QString id = devices.at(i);
        QTextBrowser *browser = findChild<QTextBrowser *>(id.toLower());
        if (browser)
        {
            browser->setStyleSheet(
                "QTextBrowser{background:#1b2128;color:#d8dee9;border:1px solid #36404a;"
                "border-radius:5px;padding:2px;selection-background-color:#40566f;}"
            );
            QFont font = QApplication::font();
            font.setPointSize(10);
            font.setBold(false);
            browser->setFont(font);
        }

        QFrame *frameA = findChild<QFrame *>(id.toLower() + "a");
        QFrame *frameB = findChild<QFrame *>(id.toLower() + "b");
        const QString frameStyle =
            "QFrame{background:transparent;border:0px;}";
        if (frameA)
            frameA->setStyleSheet(frameStyle);
        if (frameB)
            frameB->setStyleSheet(frameStyle);
    }

    ui->area1->setVisible(false);
    ui->area2->setVisible(false);
    ui->area3->setVisible(false);
    ui->area4->setVisible(false);
    ui->scrollArea_1->setVisible(false);
    ui->scrollArea_2->setVisible(false);
    ui->scrollArea_3->setVisible(false);
    ui->scrollArea_4->setVisible(false);

    on_pushButton_clicked();
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_2_released()
{
    ui->dateEdit->setDate(QDate::currentDate());
    on_pushButton_clicked();
}


void MainWindow::on_pushButton_clicked()
{
    const QDate selectedDate = ui->dateEdit->date();
    const QString path = dailyFilePath(selectedDate);

    QMap<QString, MachineFilament> machines;
    const bool loaded = parseDailyFile(path, &machines);

    const QStringList devices = allDevices();

    for (int i = 0; i < devices.size(); ++i)
    {
        const QString id = devices.at(i);
        QTextBrowser *browser = findChild<QTextBrowser *>(id.toLower());
        if (browser)
        {
            browser->clear();
            browser->setToolTip(QString());
        }

        const int deviceNo = id.mid(2).toInt();
        QLabel *indicator = findChild<QLabel *>(QString("orange%1").arg(deviceNo));
        setIndicator(indicator, 0, false);
    }

    int redCount = 0;
    int orangeCount = 0;
    int yellowCount = 0;
    int greenCount = 0;

    bool hasNetworkA = false;
    bool hasNetworkB = false;

    QMap<QString, MachineFilament>::const_iterator it = machines.constBegin();
    while (it != machines.constEnd())
    {
        const QString id = it.key();
        const MachineFilament machine = it.value();

        if (isNetworkA(id))
            hasNetworkA = true;
        if (isNetworkB(id))
            hasNetworkB = true;

        for (int i = 0; i < machine.readings.size(); ++i)
        {
            const double value = machine.readings.at(i).value;

            if (value >= 3.250 && value <= 3.599)
                ++redCount;
            else if (value >= 3.600 && value <= 3.799)
                ++orangeCount;
            else if (value >= 3.800 && value <= 3.999)
                ++yellowCount;
            else if (value >= 4.000 && value <= 4.600)
                ++greenCount;
        }

        QTextBrowser *browser = findChild<QTextBrowser *>(id.toLower());
        if (browser)
        {
            browser->setHtml(machineHtml(machine));
            browser->setToolTip(
                QString("%1 | %2 | %3 | %4 deger")
                    .arg(machine.id)
                    .arg(machine.ip)
                    .arg(machine.date)
                    .arg(machine.readings.size())
            );
        }

        const int deviceNo = id.mid(2).toInt();
        QLabel *indicator = findChild<QLabel *>(QString("orange%1").arg(deviceNo));
        setIndicator(indicator, attentionCount(machine), !machine.readings.isEmpty());

        ++it;
    }

    ui->redco->setText(QString::number(redCount));
    ui->orange->setText(QString::number(orangeCount));
    ui->sari->setText(QString::number(yellowCount));
    ui->yesil->setText(QString::number(greenCount));

    ui->checkBoxNetworkA->blockSignals(true);
    ui->checkBoxNetworkB->blockSignals(true);
    ui->checkBoxNetworkA->setChecked(hasNetworkA);
    ui->checkBoxNetworkB->setChecked(hasNetworkB);
    ui->checkBoxNetworkA->blockSignals(false);
    ui->checkBoxNetworkB->blockSignals(false);

    updateNetworkVisibility();

    QString networkText = "NETWORK ?";
    if (hasNetworkA && hasNetworkB)
        networkText = "NETWORK A+B";
    else if (hasNetworkA)
        networkText = "NETWORK A";
    else if (hasNetworkB)
        networkText = "NETWORK B";
    else if (!loaded)
        networkText = "VERI YOK";

    setWindowTitle(
        QString("Filament Monitor | %1 | %2")
            .arg(networkText)
            .arg(selectedDate.toString("yyyy-MM-dd"))
    );

    if (!loaded)
    {
        ui->redco->setText("0");
        ui->orange->setText("0");
        ui->sari->setText("0");
        ui->yesil->setText("0");
    }
}


void MainWindow::on_pushButton_3_released()
{
    const QDate selectedDate2 = ui->dateEdit_2->date();
    const QString tubedate = selectedDate2.toString("yyyy-MM-dd");

    const QMap<QString, QString> tubeData =
        readTubeCsv("/root/tubelog/log/tube-" + tubedate + ".csv");
    const QMap<QString, QString> onTimeData =
        readSectionValues("/root/Eko/xrayontime/log/xrayontime-" + tubedate + ".csv",
                          "Total", false);
    const QMap<QString, QString> downTimeData =
        readSectionValues("/root/Eko/xrayontime/offlog/xrayofftime-" + tubedate + ".csv",
                          "Max", true);

    const QStringList tubes = tubeIds();
    for (int i = 0; i < tubes.size(); ++i)
    {
        const QString id = tubes.at(i);
        styleTubeLabel(findChild<QLabel *>(id + "tub"), tubeData.value(id), 0);
    }

    const QStringList xrays = xrayIds();
    for (int i = 0; i < xrays.size(); ++i)
    {
        const QString id = xrays.at(i);
        styleTubeLabel(findChild<QLabel *>(id), onTimeData.value(id), 1);
        styleTubeLabel(findChild<QLabel *>(id + "x"), downTimeData.value(id), 2);
    }
}


void MainWindow::on_pushButton_4_released()
{
    QProcess process;
    process.start("/root/Eko/xrayontime/calculate_ontimecurrentsave.sh");
    process.waitForFinished(-1);

    QProcess process1;
    process1.start("/root/tubelog/tubesave.sh");
    process1.waitForFinished(-1);

    ui->dateEdit_2->setDate(QDate::currentDate());
    on_pushButton_3_released();
}


void MainWindow::updateNetworkVisibility()
{
    const bool showA = ui->checkBoxNetworkA->isChecked();
    const bool showB = ui->checkBoxNetworkB->isChecked();
    const bool showAny = showA || showB;

    ui->area1->setVisible(showAny);
    ui->area2->setVisible(showAny);
    ui->area3->setVisible(showAny);
    ui->area4->setVisible(showAny);
    ui->scrollArea_1->setVisible(showAny);
    ui->scrollArea_2->setVisible(showAny);
    ui->scrollArea_3->setVisible(showAny);
    ui->scrollArea_4->setVisible(showAny);

    const QStringList devices = allDevices();
    for (int i = 0; i < devices.size(); ++i)
    {
        const QString id = devices.at(i);
        const bool visible = (isNetworkA(id) && showA) || (isNetworkB(id) && showB);

        QFrame *frameA = findChild<QFrame *>(id.toLower() + "a");
        QFrame *frameB = findChild<QFrame *>(id.toLower() + "b");

        if (frameA)
            frameA->setVisible(visible);
        if (frameB)
            frameB->setVisible(visible);
    }

    ui->checkBoxNetworkA->setStyleSheet(
        showA
            ? "QCheckBox:disabled{color:#6bd968;font-weight:bold;spacing:6px;}"
            : "QCheckBox:disabled{color:#8a96a3;font-weight:bold;spacing:6px;}"
    );
    ui->checkBoxNetworkB->setStyleSheet(
        showB
            ? "QCheckBox:disabled{color:#6bd968;font-weight:bold;spacing:6px;}"
            : "QCheckBox:disabled{color:#8a96a3;font-weight:bold;spacing:6px;}"
    );
}
