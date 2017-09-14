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
    QString SendGCode(QString Command);


private slots:
    void closeEvent(QCloseEvent *event);

    void on_pbFileBrowse_released();

    void on_txtFileName_returnPressed();

    void on_btnSendCommand_released();

    void on_btnConnect_released();

    void on_btnIncX_released();

    void on_btnHomeall_released();

    void on_btnIncY_released();

    void on_btnIncZ_released();

    void on_btnDecX_released();

    void on_btnDecY_released();

    void on_btnDecZ_released();

    void on_txtIncrement_textChanged(const QString &arg1);

    void on_txtFeedrate_textChanged(const QString &arg1);

    void on_btnSetZero_released();

    void on_btnRest_released();

    void on_btnHomeX_released();

    void on_btnHomeY_released();

    void on_btnHomeZ_released();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
