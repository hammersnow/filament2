#ifndef TUBE_USAGE_WIDGET_H
#define TUBE_USAGE_WIDGET_H
#include "archive.h"
#include <QWidget>
class QComboBox;
class QDateEdit;
class QLabel;
class QTableWidget;

class UsageWidget : public QWidget {
    Q_OBJECT
public:
    explicit UsageWidget(const Archive::Paths &paths, QWidget *parent = 0);
private slots:
    void reload();
    void showIssues(int row, int column);
private:
    Archive::Paths paths;
    Archive::Report report;
    QComboBox *device, *position;
    QDateEdit *date;
    QLabel *status;
    QTableWidget *summary, *days;
};
#endif
