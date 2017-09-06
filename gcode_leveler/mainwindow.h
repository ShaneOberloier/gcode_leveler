#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore/QtGlobal>

#include <QtSerialPort/QSerialPort>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pbFileBrowse_released();

    void on_txtFileName_returnPressed();

    void on_btnSendCommand_released();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
