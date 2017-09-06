#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "global.h"
#include <QDebug>
#include <QFileDialog>
#include <QtSerialPort/QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //Fetch the available COM ports and place them in the combo box
    ui->cbxCom->clear(); //Clear the current contents (prevents duplicates)
    //This Loop finds each available COM port using the QSerialPortInfo Library
    //And adds it to the combo box
    Q_FOREACH(QSerialPortInfo port, QSerialPortInfo::availablePorts()){
                ui->cbxCom->addItem(port.portName());
    }

    serial.setPortName(ui->cbxCom->currentText());
    serial.setBaudRate(QSerialPort::Baud9600);
    serial.setDataBits(QSerialPort::Data8);
    serial.setParity(QSerialPort::NoParity);
    serial.setStopBits(QSerialPort::OneStop);
    serial.setFlowControl(QSerialPort::NoFlowControl);
    serial.open(QIODevice::ReadWrite);


}

MainWindow::~MainWindow()
{
    delete ui;
    serial.close();
}

void PromptProbeDataClear(){
    //Check to see if previous probing data exists, and if so, prompt the user for action
    //User may want to keep previous data if adjusting multiple files
    if(~ProbeDataExists){
        QMessageBox ClearData;
        ClearData.setWindowTitle("Clear Data");
        ClearData.setText("Do you want to clear the existing probing data?");
        ClearData.setStandardButtons(QMessageBox::Yes);
        ClearData.addButton(QMessageBox::No);
        ClearData.setDefaultButton(QMessageBox::No);
        if (ClearData.exec() == QMessageBox::Yes){
            //Clear the existing probing data
        }
        else{
            //Do not clear the existing probing data

        }
    }
}

void MainWindow::on_pbFileBrowse_released()
{
    FilePath = QFileDialog::getOpenFileName();//Open the system file browser, load selected path to FilePath
    ui->txtFileName->setText(FilePath);//Update the text in txtFileName
    PromptProbeDataClear();

}

void MainWindow::on_txtFileName_returnPressed()
{
    PromptProbeDataClear();
}

void MainWindow::on_btnSendCommand_released()
{
    QByteArray Command = ui->txtCommand->text().toLocal8Bit();

    serial.write(Command);

    ui->txtCommand->clear();

}
