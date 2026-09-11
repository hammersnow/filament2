#include <QtGui/QApplication>
#include "mainwindow.h"
#include <QDateEdit>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QRect>
#include <QStyleFactory>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>

static void styleDateEdit(QDateEdit *edit)
{
    if (!edit)
        return;

    edit->setMinimumSize(0, 0);
    edit->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    edit->setCalendarPopup(true);
    edit->setDisplayFormat("d/M/yy");
    edit->setStyleSheet(
        "QDateEdit{background:#20262e;color:#eef2f6;border:1px solid #46515d;"
        "border-radius:3px;padding:0px 18px 0px 6px;font-size:9pt;}"
        "QDateEdit::drop-down{subcontrol-origin:border;subcontrol-position:top right;"
        "width:14px;border-left:1px solid #46515d;background:#2b333d;"
        "border-top-right-radius:3px;border-bottom-right-radius:3px;}"
        "QDateEdit::drop-down:hover{background:#36414d;}"
    );
}

static void styleButton(QPushButton *button)
{
    if (!button)
        return;

    button->setMinimumSize(0, 0);
    button->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    button->setStyleSheet(
        "QPushButton{background:#2b333d;color:#eef2f6;border:1px solid #46515d;"
        "border-radius:3px;padding:0px 5px;font-weight:600;font-size:9pt;}"
        "QPushButton:hover{background:#36414d;}"
        "QPushButton:pressed{background:#222a32;}"
    );
}

static void moveToPage(QWidget *widget, QWidget *page)
{
    if (!widget || !page)
        return;

    QWidget *oldParent = widget->parentWidget();
    if (oldParent && oldParent->layout())
        oldParent->layout()->removeWidget(widget);

    widget->setParent(page);
    widget->show();
    widget->raise();
}

static void hideTopDateLabel(QWidget *page)
{
    if (!page)
        return;

    const QList<QLabel *> labels = page->findChildren<QLabel *>();
    for (int i = 0; i < labels.size(); ++i)
    {
        QLabel *label = labels.at(i);
        if (!label)
            continue;

        const QRect g = label->geometry();
        const QString text = label->text().toUpper();
        if (g.x() > 850 && g.y() < 90 && text.contains("DATE"))
            label->hide();
    }
}

static void placeControlRow(QWidget *page,
                            QDateEdit *dateEdit,
                            QPushButton *okButton,
                            QPushButton *currentButton,
                            int referenceWidth)
{
    if (!page || !dateEdit || !okButton || !currentButton)
        return;

    const int rightMargin = 10;
    const int y = 0;
    const int h = 19;
    const int gap = 5;
    const int dateW = 100;
    const int okW = 65;
    const int currentW = 110;
    const int totalW = dateW + gap + okW + gap + currentW;

    int usableWidth = referenceWidth;
    if (usableWidth <= 0)
        usableWidth = page->width();

    int x = usableWidth - rightMargin - totalW;
    if (x < 8)
        x = 8;

    dateEdit->setGeometry(x, y, dateW, h);
    okButton->setGeometry(x + dateW + gap, y, okW, h);
    currentButton->setGeometry(x + dateW + gap + okW + gap, y, currentW, h);

    dateEdit->show();
    okButton->show();
    currentButton->show();

    dateEdit->raise();
    okButton->raise();
    currentButton->raise();
}

static void fixTopRightControls(MainWindow &w)
{
    QWidget *tab1 = w.findChild<QWidget *>("tab");
    QWidget *tab2 = w.findChild<QWidget *>("tab_2");
    QTabWidget *tabs = w.findChild<QTabWidget *>("tabWidget");

    QDateEdit *date1 = w.findChild<QDateEdit *>("dateEdit");
    QDateEdit *date2 = w.findChild<QDateEdit *>("dateEdit_2");

    QPushButton *ok1 = w.findChild<QPushButton *>("pushButton");
    QPushButton *current1 = w.findChild<QPushButton *>("pushButton_2");
    QPushButton *ok2 = w.findChild<QPushButton *>("pushButton_3");
    QPushButton *current2 = w.findChild<QPushButton *>("pushButton_4");

    styleDateEdit(date1);
    styleDateEdit(date2);
    styleButton(ok1);
    styleButton(current1);
    styleButton(ok2);
    styleButton(current2);

    if (ok1)
        ok1->setText("Ok");
    if (current1)
        current1->setText("Current Date");
    if (ok2)
        ok2->setText("Ok");
    if (current2)
        current2->setText("Current Date");

    QWidget *oldDatePanel1 = date1 ? date1->parentWidget() : 0;
    moveToPage(date1, tab1);
    moveToPage(ok1, tab1);
    moveToPage(current1, tab1);
    if (oldDatePanel1 && oldDatePanel1 != tab1)
        oldDatePanel1->hide();

    moveToPage(date2, tab2);
    moveToPage(ok2, tab2);
    moveToPage(current2, tab2);

    // Filament sekmesindeki eski DATE basligini gizle.
    // Tube Serial sekmesindeki dort Date basligi korunur.
    hideTopDateLabel(tab1);

    int rightReference = 0;
    if (tabs)
        rightReference = tabs->width() - 4;
    if (rightReference <= 0)
        rightReference = w.width() - 8;

    placeControlRow(tab1, date1, ok1, current1, rightReference);
    placeControlRow(tab2, date2, ok2, current2, rightReference);
}

static void restoreTubeSerialReadability(MainWindow &w)
{
    QWidget *tab2 = w.findChild<QWidget *>("tab_2");
    if (!tab2)
        return;

    const QList<QLabel *> labels = tab2->findChildren<QLabel *>();
    for (int i = 0; i < labels.size(); ++i)
    {
        QLabel *label = labels.at(i);
        if (!label)
            continue;

        const QString name = label->objectName();
        if (name.endsWith("tub"))
            label->setProperty("tubeSerialValue", true);
        else if (name.startsWith("ED_"))
            label->setProperty("tubeSerialStatus", true);
    }

    QString style = tab2->styleSheet();
    style += "\nQLabel[tubeSerialValue=\"true\"]{font-family:Monospace;}";
    style += "\nQLabel[tubeSerialStatus=\"true\"]{font-family:Monospace;font-size:10px;}";
    tab2->setStyleSheet(style);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Plastique"));

    MainWindow w;
    w.show();
    a.processEvents();

    QTabWidget *tabs = w.findChild<QTabWidget *>("tabWidget");
    int oldIndex = 0;

    if (tabs)
    {
        oldIndex = tabs->currentIndex();
        if (tabs->count() > 1)
        {
            tabs->setCurrentIndex(1);
            a.processEvents();
        }
    }

    fixTopRightControls(w);
    restoreTubeSerialReadability(w);

    if (tabs)
    {
        tabs->setCurrentIndex(oldIndex);
        a.processEvents();
    }

    // Ayni fonksiyon hem acilista hem Ok butonunda kullanilir.
    // on_pushButton_3_released artik QProcess acmadan loglari dogrudan okur.
    QTimer::singleShot(0, &w, SLOT(on_pushButton_3_released()));

    return a.exec();
}
