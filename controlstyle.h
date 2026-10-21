#ifndef FILAMENT_CONTROL_STYLE_H
#define FILAMENT_CONTROL_STYLE_H

#include <QString>
#include <QCalendarWidget>
#include <QDateEdit>
#include <QTextCharFormat>

inline void prepareCalendar(QDateEdit *edit)
{
    QCalendarWidget *calendar = edit->calendarWidget();
    calendar->setMinimumSize(308,250);
    QTextCharFormat header;
    header.setBackground(QColor("#2b3b4b")); header.setForeground(QColor("#eef4fa"));
    calendar->setHeaderTextFormat(header);
    QTextCharFormat weekend;
    weekend.setForeground(QColor("#ffb18c"));
    calendar->setWeekdayTextFormat(Qt::Saturday,weekend);
    calendar->setWeekdayTextFormat(Qt::Sunday,weekend);
}

inline QString selectorStyle()
{
        return "QComboBox,QDateEdit{background:#202b38;color:#eef4fa;border:1px solid #526579;"
               "border-radius:6px;padding:6px 40px 6px 10px;min-height:26px;font-size:10pt;}"
               "QComboBox:hover,QDateEdit:hover{border-color:#91adc7;}"
               "QComboBox:focus,QDateEdit:focus{border:1px solid #79b8e6;background:#263648;}"
               "QComboBox::drop-down,QDateEdit::drop-down{subcontrol-origin:border;subcontrol-position:top right;"
               "width:32px;border:0;border-left:1px solid #526579;border-top-right-radius:6px;"
               "border-bottom-right-radius:6px;background:#2c3d50;}"
               "QComboBox::drop-down:hover,QDateEdit::drop-down:hover{background:#3c536b;}"
               "QComboBox::down-arrow,QDateEdit::down-arrow{image:url(:/foto/chevron-down.png);width:12px;height:10px;}"
               "QComboBox QAbstractItemView{background:#202b38;color:#eef4fa;border:1px solid #526579;"
               "selection-background-color:#365674;selection-color:#fff;outline:0;}"
               "QComboBox QAbstractItemView::item{min-height:30px;padding:3px 8px;}"
               "QCalendarWidget QWidget{background:#202b38;color:#eef4fa;}"
               "QCalendarWidget QToolButton{background:#2c3d50;color:#eef4fa;border:0;padding:6px;}"
               "QCalendarWidget QAbstractItemView{background:#202b38;alternate-background-color:#2b3b4b;color:#eef4fa;"
               "selection-background-color:#365674;selection-color:#fff;}";
}
#endif
