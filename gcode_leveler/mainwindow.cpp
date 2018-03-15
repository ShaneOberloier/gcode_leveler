#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "global.h"
#include <QDebug>
#include <QFileDialog>
#include <QtSerialPort/QSerialPort>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QCloseEvent>
#include <QThread>
#include <QTimer>


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

    //Add the common Baud Rates
    ui->cbxBaud->addItem(QStringLiteral("9600"), QSerialPort::Baud9600);
    ui->cbxBaud->addItem(QStringLiteral("19200"), QSerialPort::Baud19200);
    ui->cbxBaud->addItem(QStringLiteral("38400"), QSerialPort::Baud38400);
    ui->cbxBaud->addItem(QStringLiteral("115200"), QSerialPort::Baud115200);
    ui->cbxBaud->setCurrentIndex(3);//Set to the default baud rate for this mill

    // create a stream timer
    StreamTimer = new QTimer(this);

    // setup signal and slot
    connect(StreamTimer, SIGNAL(timeout()),
          this, SLOT(MyTimerSlot()));

    // create a stream timer
    ReadTimer = new QTimer(this);

    // setup signal and slot
    connect(ReadTimer, SIGNAL(timeout()),
          this, SLOT(ReceiveData()));

    // create a stream timer
    ProbeTimer = new QTimer(this);

    // setup signal and slot
    connect(ProbeTimer, SIGNAL(timeout()),
          this, SLOT(ProbeStream()));

    // create a stream timer
    BacklashTimer = new QTimer(this);

    // setup signal and slot
    connect(BacklashTimer, SIGNAL(timeout()),
          this, SLOT(BacklashSlot()));
}

void MainWindow::ReceiveData()
{   QString data="";
    if(serial.canReadLine()){
        data = serial.readLine();
        data.remove("\n");
        if (data!="")
        {
            ui->tbxCommandHistory->append("R: " + data);

            if (data=="ok"){
                MachineReady=true;
                return;
            }
            Response=data;//We don't want to save "ok" in the response - to keep meaningful data.
            if (Response.contains("endstops hit")){
                QStringList ResponseParts = Response.split(":");
                QString Meas = ResponseParts.at(3);
                Measurement = Meas.toFloat();
                qDebug() << Measurement;
                MeasRecorded=false;
            }
        }
    }
}

void MainWindow::MyTimerSlot()
{
    QString Response = "";


    int LastLine = GCode.size();
    if(MachineReady & CompensationState==0){
        if(lineNumber<=LastLine-1){
            SendGCode(GCode[lineNumber]);
            lineNumber++;
        }
        else
            StreamTimer->stop();
    }
}

bool Reverse=0; //0 is forward, 1 is Reverse
void MainWindow::ProbeStream()
{
    float IncX = SizeX/(PointsX-1);
    float IncY = SizeY/(PointsY-1);
    //We have 3 states to handle - movement, probing, and recording data.
    //Assume that the probe is at (0,0,0)
    if (ProbeState==0){//Initialize values
        ProbeState = 2; //0 = XY Movement, 1 = Downward Movement (probe), 2 = Record Data, 3 = Upward Movement
        ProbeX=0;
        ProbeY=0;
        MeasRecorded=false;
    }
    //Before we can do anything make sure the machine is ready
    if(MachineReady & CompensationState==0){
        switch(ProbeState){
            case 1://XY Movement
            {
                //Check if X Motion is necissary
                //Detect if we're done
                if((((ProbeX == PointsX-1) & ~Reverse)|((ProbeX == 0) & Reverse)) & (ProbeY == PointsY-1))
                {
                    //Leave this timer - We're all done probing!
                    ProbeState=0;
                    //Spit out Matrix data for debug
                    ProbeTimer->stop();
                    break;
                }
                //Detect if we've reached an edge
                if(((Reverse==0) & (ProbeX == PointsX-1))|((Reverse==1) & (ProbeX==0)))
                {
                    //Let's move in the Y Direction
                    RelativeSendGCode(Movement + " Y"+ QByteArray::number(IncY) +" F" + QByteArray::number(MovementSpeed));
                    ProbeState=2;
                    ProbeY++;
                    Reverse=!Reverse;
                    break;
                }
                //If we get to this point then we either need to go left or right on the X axis
                if(Reverse)
                {
                    RelativeSendGCode(Movement + " X-"+ QByteArray::number(IncX) +" F" + QByteArray::number(MovementSpeed));
                    ProbeX--;
                    ProbeState=2;
                    break;
                }
                if(!Reverse)
                {
                    RelativeSendGCode(Movement + " X"+ QByteArray::number(IncX) +" F" + QByteArray::number(MovementSpeed));
                    ProbeX++;
                    ProbeState=2;
                    break;
                }
            }
            case 2://Probe
            {
                RelativeSendGCode(Movement + " Z-10 F" + QByteArray::number(ProbingSpeed));
                ProbeState=3;
            }
            break;
            case 3://Record Data
            {
                if (!MeasRecorded)
                {
                    qDebug() << Measurement;
                    MeasRecorded=true;
                    MeasurementArray[ProbeX][ProbeY] = Measurement;
                    ProbeState=4;
                }
                break;
            }
            case 4://Upward Movement
            {
                RelativeSendGCode(Movement + " Z5 F" + QByteArray::number(ProbingSpeed));
                ProbeState=1;
                break;
            }
        default: ProbeState=2;
        }
    }
}

void MainWindow::BacklashSlot()
{
    ReceiveData();
    //Must wait for Machine Ready at every step
    if (MachineReady)
    {
        switch(CompensationState){
            case 0:
            {
                BacklashTimer->stop();
                CommandMemory="";
                return;
            }
            case 1: //Compensate for backlash in each relevant direction (1)
            {
                if((RelativeEnabled==0)&(RelativeOverride==0)){
                    serial.write("G91\n");
                    MachineReady=false;
                    ui->tbxCommandHistory->append("S: G91"); //Add the command to the history
                    while (!MachineReady)//We HAVE to wait for the measurement
                    {
                        serial.waitForReadyRead(10);
                        ReceiveData();
                    }
                }
                Response="";
                serial.write("M114\n");
                MachineReady=false;
                ui->tbxCommandHistory->append("S: M114"); //Add the command to the history
                while (Response=="")//We HAVE to wait for the measurement
                {
                    serial.waitForReadyRead(10);
                    ReceiveData();
                }
                    qDebug() << Response;
                    QString BacklashCommand="G0";
                    //X+
                    if (BacklashDirections[0])
                    {
                        BacklashCommand+=" X" + QString::number(BacklashX);;
                        BacklashDirections[0]=0;
                    }
                    //X-
                    if (BacklashDirections[1])
                    {
                            BacklashCommand+=" X-" + QString::number(BacklashX);;
                            BacklashDirections[1]=0;
                    }
                    //Y+
                    if (BacklashDirections[2])
                    {
                            BacklashCommand+=" Y" + QString::number(BacklashY);
                            BacklashDirections[2]=0;
                    }
                    //Y-
                    if (BacklashDirections[3])
                    {
                            BacklashCommand+=" Y-" + QString::number(BacklashY);
                            BacklashDirections[3]=0;
                    }
                    //Y+
                    if (BacklashDirections[4])
                    {
                            BacklashCommand+=" Z" + QString::number(BacklashZ);;
                            BacklashDirections[4]=0;
                    }
                    //Y-
                    if (BacklashDirections[5])
                    {
                            BacklashCommand+=" Z-" + QString::number(BacklashZ);;
                            BacklashDirections[5]=0;
                    }
                    qDebug()<<BacklashCommand;
                    CompensationState=2;

                    if((RelativeEnabled==0)&(RelativeOverride==0)){
                        serial.write("G90\n");
                        MachineReady=false;
                        ui->tbxCommandHistory->append("S: G90"); //Add the command to the history
                        while (!MachineReady)//We HAVE to wait for the measurement
                        {
                            serial.waitForReadyRead(10);
                            ReceiveData();
                        }
                    }
                return;
            }
            case 2: //Set position to measured location. (2)
            {
                //MachineReady = false;
                CompensationState =3;
                return;
            }
            case 3://Send original movement command (3)
            {
                MachineReady = false;
                serial.write(CommandMemory.toLocal8Bit() + "\n"); //Send command & append new line characer
                ui->tbxCommandHistory->append("S: " + CommandMemory); //Add the command to the history
                UpdateStatus();
                CompensationState = 0;
                return;
            }
        }
    }
}

MainWindow::~MainWindow()
{
    //serial.close();
    //delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    for(int i=0; i < PointsY; ++i)
    {
        delete [] MeasurementArray[i];
    }
    delete [] MeasurementArray;
    on_btnRest_released();//This is the command to implement the rest procedure
    serial.close();//Close the serial communication so it may be picked up later
    delete ui;
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

void MainWindow::RelativeSendGCode(QString Command){
    if(RelativeEnabled==0){SendGCode("G91"); RelativeOverride=1;}
    SendGCode(Command);
    if(RelativeEnabled==0){SendGCode("G90"); RelativeOverride=0;}
}

QString MainWindow::SendGCode(QString Command){
    //QString Response; //What will be read back from the machine
    QByteArray CommandBytes = Command.toLocal8Bit(); //Make it so the string will play with serial
    if(serial.isOpen()){
        //Make sure we didn't skip a backlash compensation
        while(CompensationState != 0)
        {
            BacklashSlot();
        }
        MachineReady =false;
        if (Command=="")
        {
            MachineReady =true;
            return "";//A blank space will cause problems because there is no response
        }
        if (Command.contains(";"))
        {   MachineReady =true;
            return "";//Don't send comments
        }
        if (Command.contains("G90")&(RelativeOverride==1)){
            //We are in absoloute movement
            RelativeEnabled = false;
        }
        if (Command.contains("G91")&(RelativeOverride==1)){
            //We are in relative movement
            RelativeEnabled = true;
        }
        if (Command.contains("G92")){
            QStringList CommandParts = Command.split(" ");
            if (Command.contains("X"))
            {
                  QString Xpos = CommandParts.filter("X").at(0);
                  PosX = Xpos.mid(1).toFloat();
            }
            if (Command.contains("Y"))
            {
                  QString Ypos = CommandParts.filter("Y").at(0);
                  PosY = Ypos.mid(1).toFloat();
            }
            if (Command.contains("Z"))
            {
                  QString Zpos = CommandParts.filter("Z").at(0);
                  PosZ = Zpos.mid(1).toFloat();
            }
        }
        if(CompensateBacklash){
            //Check and see if this is a motion command. If it is, then add backlash error.
            if(Command.contains("G1")|Command.contains("G0")){
                //Extract positional values
                QStringList CommandParts = Command.split(" ");
                if (Command.contains("X")){
                      QString Xpos = CommandParts.filter("X").at(0);
                      Xpos = Xpos.mid(1);
                      float XposVal = Xpos.toFloat();
                      qDebug()<<"X Destination: "<<XposVal;
                      if (RelativeEnabled|RelativeOverride){
                          XposVal = XposVal + PosX;
                      }
                      if ((XposVal > PosX) && (DeltaX < 0)){
                        //Compensate with positive backlash
                        qDebug()<<"Compensating for X Backlash";
                        BacklashDirections[0]=true;
                        CompensationState=1;
                        DeltaX = XposVal - PosX;
                        PosX = XposVal;
                      }
                       else if ((XposVal < PosX) && (DeltaX >= 0)){
                        //Compensate with positive backlash
                        qDebug()<<"Compensating for X Backlash";
                        BacklashDirections[1]=true;
                        CompensationState=1;
                        DeltaX = XposVal - PosX;
                        PosX = XposVal;
                      }
                      else if (XposVal != PosX){
                          DeltaX = XposVal - PosX;
                          PosX = XposVal;
                      }
                }
                if (Command.contains("Y")){
                    QString Ypos = CommandParts.filter("Y").at(0);
                    Ypos = Ypos.mid(1);
                    float YposVal = Ypos.toFloat();
                    qDebug()<<"Y Destination: "<<YposVal;
                    if (RelativeEnabled|RelativeOverride){
                        YposVal = YposVal + PosY;
                    }
                    if ((YposVal > PosY) && (DeltaY < 0)){
                      //Compensate with positive backlash
                      qDebug()<<"Compensating for Y Backlash";
                      BacklashDirections[2]=true;
                      CompensationState=1;
                      DeltaY = YposVal - PosY;
                      PosY = YposVal;
                    }
                     else if ((YposVal < PosY) && (DeltaY >= 0)){
                      //Compensate with positive backlash
                      qDebug()<<"Compensating for Y Backlash";
                      BacklashDirections[3]=true;
                      CompensationState=1;
                      DeltaY = YposVal - PosY;
                      PosY = YposVal;
                    }
                    else if (YposVal != PosY){
                        DeltaY = YposVal - PosY;
                        PosY = YposVal;
                    }
                }
                if (Command.contains("Z")){
                    QString Zpos = CommandParts.filter("Z").at(0);
                    Zpos = Zpos.mid(1);
                    float ZposVal = Zpos.toFloat();
                    qDebug()<<"Z Destination: "<<ZposVal;
                    if (RelativeEnabled|RelativeOverride){
                        ZposVal = ZposVal + PosZ;
                    }
                    if ((ZposVal > PosZ) && (DeltaZ < 0)){
                      //Compensate with positive backlash
                      qDebug()<<"Compensating for Z Backlash";
                      BacklashDirections[4]=true;
                      CompensationState=1;
                      DeltaZ = ZposVal - PosZ;
                      PosZ = ZposVal;
                    }
                     else if ((ZposVal < PosZ) && (DeltaZ >= 0)){
                      //Compensate with positive backlash
                      qDebug()<<"Compensating for Z Backlash";
                      BacklashDirections[5]=true;
                      CompensationState=1;
                      DeltaZ = ZposVal - PosZ;
                      PosZ = ZposVal;
                    }
                    else if (ZposVal != PosZ){
                        DeltaZ = ZposVal - PosZ;
                        PosZ = ZposVal;
                    }
                }
            }
            //Call for backlash compensation if necissary.
            if (CompensationState != 0)
            {
                //Turn on the backlash compensator.
                BacklashTimer->start(1);
                CommandMemory=Command;
                MachineReady =true;
                return "";
            }
        }
        serial.write(CommandBytes + "\n"); //Send command & append new line characer
        ui->tbxCommandHistory->append("S: " + Command); //Add the command to the history
    } else {
        qDebug() << "Not connected";
        ui->tbxCommandHistory->append("ERROR: Failure to send command - not connected");
    }
    //ReadTimer->stop();
    //serial.waitForReadyRead(50);
    //Response = serial.readAll(); //Read the response from the machine
    //ui->tbxCommandHistory->append("R: " + Response);
    //ReadTimer->start(100);
    UpdateStatus();
    return Response;
}

void MainWindow::on_pbFileBrowse_released()
{
    FilePath = QFileDialog::getOpenFileName();//Open the system file browser, load selected path to FilePath
    ui->txtFileName->setText(FilePath);//Update the text in txtFileName
    ui->txtFileName_2->setText(FilePath);
    PromptProbeDataClear();

    QFile textFile(FilePath);
    textFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream textStream(&textFile);
    ui->txtGCode->clear();
    GCode.clear();
    while(true)
    {
        QString line = textStream.readLine();
        if (line.isNull())
            break;
        else{
            GCode.append(line);
            ui->txtGCode->append(line);
           }
    }

}

void MainWindow::on_txtFileName_returnPressed()
{
    PromptProbeDataClear();
}
void MainWindow::UpdateStatus()
{
    ui->txtStatus->setText("X:" + QString::number(PosX) + " Y:" + QString::number(PosY) + " Z:" + QString::number(PosZ) );
}

void MainWindow::on_btnSendCommand_released()
{
    SendGCode(ui->txtCommand->text());
    ui->txtCommand->clear();
}

void MainWindow::on_btnConnect_released()
{
    if(serial.isOpen()){
        serial.close();
        ui->btnConnect->setText("Connect");
    }
    else{
        serial.setPortName(ui->cbxCom->currentText());
        serial.setBaudRate(ui->cbxBaud->currentText().toInt());
        serial.setDataBits(QSerialPort::Data8);
        serial.setParity(QSerialPort::NoParity);
        serial.setStopBits(QSerialPort::OneStop);
        serial.setFlowControl(QSerialPort::NoFlowControl);
        if(serial.open(QIODevice::ReadWrite)){
                qDebug() << "Connection Successful";
                ReadTimer->start(10);
                MachineReady=true;
                /*serial.waitForReadyRead(1000);
                QByteArray data;
                while(serial.canReadLine())
                {
                    data = serial.readLine();
                    ui->tbxCommandHistory->append("R: " + data);
                    serial.waitForReadyRead(1000);
                }*/
                ui->btnConnect->setText("Disconnect");
        }else {
                qDebug() << "Connection Failed";
        }
    }
}

void MainWindow::on_btnIncY_released()
{
    RelativeSendGCode(Movement + " Y" + QByteArray::number(Increment) + " F" + QByteArray::number(Feedrate));
}

void MainWindow::on_btnIncZ_released()
{
    RelativeSendGCode(Movement + " Z" + QByteArray::number(Increment) + " F" + QByteArray::number(Feedrate));
}

void MainWindow::on_btnDecX_released()
{
    RelativeSendGCode(Movement + " X-" + QByteArray::number(Increment) + " F" + QByteArray::number(Feedrate));
}

void MainWindow::on_btnIncX_released()
{
    RelativeSendGCode(Movement + " X" + QByteArray::number(Increment) + " F" + QByteArray::number(Feedrate));
}
void MainWindow::on_btnDecY_released()
{
    RelativeSendGCode(Movement + " Y-" + QByteArray::number(Increment) + " F" + QByteArray::number(Feedrate));
}

void MainWindow::on_btnDecZ_released()
{
    RelativeSendGCode(Movement + " Z-" + QByteArray::number(Increment) + " F" + QByteArray::number(Feedrate));
}

void MainWindow::on_txtIncrement_textChanged(const QString &arg1)
{
    //When the Increment text is changed, update the global variable
    Increment = ui->txtIncrement->text().toFloat();
}

void MainWindow::on_txtFeedrate_textChanged(const QString &arg1)
{
    //When the Feedrate text is changed, update the global variable
    Feedrate = ui->txtFeedrate->text().toFloat();
}

void MainWindow::on_btnSetZero_released()
{
    //Set the current location to (0,0,0)
    SendGCode(SetPosition + " X0 Y0 Z0");
    //Reset All backlash tracking variables.
    PosX = 0; //Tracking position of X motors
    PosY = 0; //Tracking position of Y motors
    PosZ = 0; //Tracking position of Z motors
    DeltaX = 0;
    DeltaY = 0;
    DeltaZ = 0;
}

void MainWindow::on_btnRest_released()
{
    //Here is a resting procedure that is customized for the OSE Circuit Mill
    SendGCode("G28"); //Home all axes
    SendGCode(RelativeMovement);
    SendGCode(Movement + " Z-12 F100");
    SendGCode(AbsoluteMovement);
    SendGCode("M84");//Disable motors This only works in Marlin
}

void MainWindow::on_btnHomeX_released()
{
    //Home the X Axis
    SendGCode("G28 X");
}

void MainWindow::on_btnHomeY_released()
{
    //Home the Y Axis
    SendGCode("G28 Y");
}

void MainWindow::on_btnHomeZ_released()
{
    //Home the Z Axis
    SendGCode("G28 Z");
}

void MainWindow::on_btnHomeall_released()
{
    //Home all axes
    ui->tbxCommandHistory->append(SendGCode("G28 X Y Z"));
}

void MainWindow::on_cbxCompensateBacklash_stateChanged(int arg1)
{
    CompensateBacklash = ui->cbxCompensateBacklash->isChecked();
}

void MainWindow::on_txtFileName_textEdited(const QString &arg1)
{
    ui->txtFileName_2->setText(arg1);
}

void MainWindow::on_btnRun_released()
{
    lineNumber=0;
    StreamTimer->start(1);
}

void MainWindow::on_pbTest_released()
{
    ui->tbxCommandHistory->clear();
}

void MainWindow::on_btnTest2_released()
{
    Response="";
    serial.write("M114\n");
    while (Response=="")
    {
        serial.waitForReadyRead(1000);
        ReceiveData();
    }
}

void MainWindow::on_pbFileBrowse_2_released()
{
    MainWindow::on_pbFileBrowse_released();
}

void MainWindow::on_btnPause_released()
{
    StreamTimer->stop();
}

void MainWindow::on_btnResume_released()
{
    StreamTimer->start(1);
}

void MainWindow::on_btnSpindleOn_released()
{
    SendGCode("M106");
}

void MainWindow::on_btnStartProbe_released()
{
    ProbeTimer->start(1);
    MeasurementArray=new float*[PointsY];
    for (int i=0; i < PointsY; ++i)
    {
        MeasurementArray[i]=new float[PointsX];
    }
    //Prompt to append G-Code
    //If Yes - Append G-Code
    //If No - do not append, but keep probe data
}

void MainWindow::ProbeSequence()
{
    float IncX = SizeX/PointsX;
    float IncY = SizeY/PointsY;
    float Measurement;
    //Create Array of XxY
    int Heights[PointsX][PointsY];
    //Before Movement - enter relative movement
    if(RelativeEnabled==0){SendGCode("G91"); RelativeOverride=1;}
    //For all Y elements
    for(int i=0; i<PointsY; i++)
    {
        //For all X elements
        for(int j=0; j<PointsX; j++)
        {
            //Probe Height
            SendGCode(Movement + " Z-10 F" + ProbingSpeed);
            //Record Value at XY
            while(Response==""|Response=="ok\n")
            {
                //echo:endstops hit:  Z:
                //Do Nothing till a response is received
                //sleep(5);
            }
            QStringList ResponseParts = Response.split(" ");
            QString Meas = ResponseParts.filter(":").at(0);
            Meas = Meas.mid(1);
            Measurement = Meas.toFloat();
            Response="";
            qDebug()<<Measurement;
            //Move Back Up
            SendGCode(Movement + " Z5 F" + ProbingSpeed);
            //Move Delta X
            SendGCode(Movement + " X"+ IncX +" F" + MovementSpeed);
        }
        //Move Delta Y
        SendGCode(Movement + " Y"+ IncY +" F" + MovementSpeed);
    }
    if(RelativeEnabled==0){SendGCode("G90"); RelativeOverride=0;}
}

void MainWindow::on_btnSpindleOff_released()
{
    SendGCode("M107");
}

void MainWindow::on_btnAbortProbe_released()
{
    ProbeTimer->stop();
    for(int i=0; i < PointsY; ++i)
    {
        delete [] MeasurementArray[i];
    }
    delete [] MeasurementArray;
}

void MainWindow::on_txtSizeX_textChanged(const QString &arg1)
{
    if(!(ProbeTimer->isActive()))
        SizeX = ui->txtSizeX->text().toFloat();
}

void MainWindow::on_txtSizeY_textChanged(const QString &arg1)
{
    if(!(ProbeTimer->isActive()))
        SizeY = ui->txtSizeY->text().toFloat();
}

void MainWindow::on_txtPointsX_textChanged(const QString &arg1)
{
    if(!(ProbeTimer->isActive()))
        PointsX = ui->txtPointsX->text().toInt();
}

void MainWindow::on_txtPointsY_textChanged(const QString &arg1)
{
    if(!(ProbeTimer->isActive()))
        PointsY = ui->txtPointsY->text().toInt();
}
