#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

private slots:
    void on_pushButton_4_released();
    void on_pushButton_3_released();
    void on_pushButton_2_released();
    void on_pushButton_clicked();
    void updateNetworkVisibility();

};

#endif // MAINWINDOW_H
