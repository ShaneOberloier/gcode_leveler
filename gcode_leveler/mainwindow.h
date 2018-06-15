#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtCore/QtGlobal>

#include <QtSerialPort/QSerialPort>
#include <QTimer>

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
    void RelativeSendGCode(QString Command);
    void ProbeSequence();
    void MyTimer();
    QTimer *StreamTimer;
    QTimer *ProbeTimer;
    QTimer *ReadTimer;
    QTimer *BacklashTimer;


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

    void on_cbxCompensateBacklash_stateChanged(int arg1);

    void on_txtFileName_textEdited(const QString &arg1);

    void on_btnRun_released();

    void on_pbTest_released();

    void on_btnTest2_released();

    void on_pbFileBrowse_2_released();

    void MyTimerSlot();

    void ReceiveData();

    void ProbeStream();

    void BacklashSlot();

    void on_btnPause_released();

    void on_btnResume_released();

    void on_btnSpindleOn_released();

    void on_btnStartProbe_released();

    void on_btnSpindleOff_released();

    void on_btnAbortProbe_released();

    void on_txtSizeX_textChanged(const QString &arg1);

    void on_txtSizeY_textChanged(const QString &arg1);

    void on_txtPointsX_textChanged(const QString &arg1);

    void on_txtPointsY_textChanged(const QString &arg1);

    void UpdateStatus();

    void on_txtBacklashX_textChanged(const QString &arg1);

    void on_txtBacklashY_textChanged(const QString &arg1);

    void on_txtBacklashNegX_textChanged(const QString &arg1);

    void on_txtBacklashNegY_textChanged(const QString &arg1);

    void on_txtBacklashZ_textChanged(const QString &arg1);

    void on_txtBacklashNegZ_textChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
