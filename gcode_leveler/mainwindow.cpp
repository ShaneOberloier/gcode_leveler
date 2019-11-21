#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "global.h"
#include "gcodelib.h"
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

void InsertIntoGCode(int Index, QString Text)
{
    int GCodeSize = GCode.size();
    GCode << " ";
    for (int i = GCodeSize-1; i >= Index; --i)
    {
        GCode.replace(i+1,GCode.at(i));
    }
    GCode[Index]=Text;
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
            /*if (Response.contains("Count"))
            {
                QStringList PositionParts = Response.split(" ");
                MeasX=PositionParts[0];
                MeasX.remove("X:");
                MeasY=PositionParts[1];
                MeasY.remove("Y:");
                MeasZ=PositionParts[2];
                MeasZ.remove("Z:");
            }*/
            if (Response.contains("endstops hit")){
                QStringList ResponseParts = Response.split(":");
                QString Meas = ResponseParts.at(3);
                Measurement = Meas.toFloat();
                //qDebug() << Measurement;
                MeasRecorded=false;

                serial.write("G92 Z" + Meas.toLocal8Bit() + "\n");
                qDebug()<< "G92 Z" + Meas.toLocal8Bit();
                MachineReady=false;
                PosZ=Meas.toFloat();
                UpdateStatus();
                ui->tbxCommandHistory->append("G92 Z" + Meas); //Add the command to the history
                while (MachineReady==false)//We HAVE to wait for the measurement
                {
                    serial.waitForReadyRead(10);
                    ReceiveData();
                }
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
            ui->pbrRunProgress->setValue(lineNumber*100/LastLine);
        }
        else
        {
            StreamTimer->stop();
            GCode.clear();
        }
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
                    for (int j=0; j<PointsY;j++)
                    {
                        for (int i=0; i<PointsX;i++)
                        {
                            qDebug()<<MeasurementArray[j+i*(PointsX+1)];
                            ui->tbxProbeData->insertPlainText(QString::number(MeasurementArray[j+i*(PointsX+1)]));
                            if (~(j==PointsY-1))
                                ui->tbxProbeData->insertPlainText(",\t");
                        }
                        ui->tbxProbeData->append("");
                    }
                    AppendGCodeAndRun("G1 X0 Y0");
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
                    MeasurementArray[ProbeY+ProbeX*(PointsX+1)] = Measurement;
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

void MainWindow::FindGCodeFootPrint()
{
    QStringList GCodeTemp;

    QFile textFile(FilePath);
    textFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream textStream(&textFile);
    GCodeTemp.clear();
    while(true)
    {
        QString line = textStream.readLine();
        if (line.isNull())
            break;
        else{
            GCodeTemp.append(line);
        }
    }
    int LineCount = GCodeTemp.length();

    float xMin = 0;
    float xMax = 0;
    float yMin = 0;
    float yMax = 0;
    for (int i =0; i<LineCount; i++)
    {
        QString Command = GCodeTemp[i];
        //qDebug()<<Command;
        if(GCodeLib::IsMovementCommand(Command)){
            //Extract positional values
            QStringList CommandParts = Command.split(" ");
            if (Command.contains("X")){
                QString Xpos = CommandParts.filter("X").at(0);
                Xpos = Xpos.mid(1);
                float XPosVal = Xpos.toFloat();
                if (XPosVal > xMax)
                    xMax = XPosVal;
                if (XPosVal < xMin)
                    xMin = XPosVal;
            }
            if (Command.contains("Y")){
                QString Ypos = CommandParts.filter("Y").at(0);
                Ypos = Ypos.mid(1);
                float YPosVal = Ypos.toFloat();
                if (YPosVal > yMax)
                    yMax = YPosVal;
                if (YPosVal < yMin)
                    yMin = YPosVal;
            }
        }
    }
    qDebug()<<"XMin: "<<xMin;
    qDebug()<<"XMax: "<<xMax;
    qDebug()<<"YMin: "<<yMin;
    qDebug()<<"YMax: "<<yMax;
}


void MainWindow::BacklashSlot()
{

    if((RelativeEnabled==0)&(RelativeOverride==0)){
        InsertIntoGCode(lineNumber,"G90");}
    QString BacklashCommand="G0";
    QString PositionCommand="G92";
    //X+
    if (BacklashDirections[0])
    {
        BacklashCommand+=" X" + QString::number(BacklashX);
        BacklashDirections[0]=0;
        MeasX = QString::number(PosX-DeltaX);
        PositionCommand += " X" + MeasX;
    }
    //X-
    else if (BacklashDirections[1])
    {
        BacklashCommand+=" X-" + QString::number(BacklashNegX);
        BacklashDirections[1]=0;
        MeasX = QString::number(PosX-DeltaX);
        PositionCommand += " X" + MeasX;
    }
    //Y+
    if (BacklashDirections[2])
    {
        BacklashCommand+=" Y" + QString::number(BacklashY);
        BacklashDirections[2]=0;
        MeasY = QString::number(PosY-DeltaY);
        PositionCommand += " Y" + MeasY;
    }
    //Y-
    else if (BacklashDirections[3])
    {
        BacklashCommand+=" Y-" + QString::number(BacklashNegY);
        BacklashDirections[3]=0;
        MeasY = QString::number(PosY-DeltaY);
        PositionCommand += " Y" + MeasY;
    }
    //Y+
    if (BacklashDirections[4])
    {
        BacklashCommand+=" Z" + QString::number(BacklashZ);;
        BacklashDirections[4]=0;
        MeasZ = QString::number(PosZ-DeltaZ);
        PositionCommand += " Z" + MeasZ;
    }
    //Y-
    else if (BacklashDirections[5])
    {
        BacklashCommand+=" Z-" + QString::number(BacklashNegZ);;
        BacklashDirections[5]=0;
        MeasZ = QString::number(PosZ-DeltaZ);
        PositionCommand += " Z" + MeasZ;
    }




    //InsertIntoGCode(lineNumber,"G92 X" + MeasX.toLocal8Bit() +" Y" + MeasY.toLocal8Bit() + " Z" + MeasZ.toLocal8Bit());
    InsertIntoGCode(lineNumber,PositionCommand);
    InsertIntoGCode(lineNumber,BacklashCommand);
    if((RelativeEnabled==0)&(RelativeOverride==0)){

        InsertIntoGCode(lineNumber,"G91");
    }
    BacklashTimer->stop();
    lineNumber--;
    CompensationState = 0;

}

MainWindow::~MainWindow()
{
    //serial.close();
    //delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
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
    if(RelativeEnabled==0){RelativeOverride=1; GCode.append("G91");}
    GCode.append(Command);
    if(RelativeEnabled==0){GCode.append("G90"); RelativeOverride=0;}
    if (~(StreamTimer->isActive()))
    {
    lineNumber=0;
    StreamTimer->start(1);
    }
}

void MainWindow::AppendGCodeAndRun(QString Command){
    GCode.append(Command);
    if (~(StreamTimer->isActive()))
    {
    lineNumber=0;
    StreamTimer->start(1);
    }
}

QString MainWindow::SendGCode(QString Command){
    //QString Response; //What will be read back from the machine
    QByteArray CommandBytes = Command.toLocal8Bit(); //Make it so the string will play with serial
    if(serial.isOpen()){

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
        if (Command.contains("M6"))
        {   MachineReady =true;
            MessageIncoming = true;
            return "";//Don't send the message command
        }
        if (MessageIncoming == true)
        {   MachineReady =true;
            MessageIncoming = false;

            QMessageBox Message;
            Message.setWindowTitle("G-Code Message");
            Message.setText(Command);
            Message.setStandardButtons(QMessageBox::Ok);
            Message.exec();

            return "";//Don't send the message command
        }
        if ((Command.contains("G90"))&(RelativeOverride==0)){
            //We are in absoloute movement
            RelativeEnabled = false;
        }
        if ((Command.contains("G91"))&(RelativeOverride==0)){
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
            if(GCodeLib::IsMovementCommand(Command)){
                //Extract positional values
                QStringList CommandParts = Command.split(" ");
                if (Command.contains("X")){
                      QString Xpos = CommandParts.filter("X").at(0);
                      Xpos = Xpos.mid(1);
                      float XposVal = Xpos.toFloat();
                      //qDebug()<<"X Destination: "<<XposVal;
                      if (RelativeEnabled|RelativeOverride){
                          XposVal = XposVal + PosX;
                      }
                      if ((XposVal > PosX) && (DeltaX < 0)){
                        //Compensate with positive backlash
                        //qDebug()<<"Compensating for X Backlash";
                        BacklashDirections[0]=true;
                        CompensationState=1;
                        DeltaX = XposVal - PosX;
                        PosX = XposVal;
                      }
                       else if ((XposVal < PosX) && (DeltaX >= 0)){
                        //Compensate with positive backlash
                        //qDebug()<<"Compensating for X Backlash";
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
                    //qDebug()<<"Y Destination: "<<YposVal;
                    if (RelativeEnabled|RelativeOverride){
                        YposVal = YposVal + PosY;
                    }
                    if ((YposVal > PosY) && (DeltaY < 0)){
                      //Compensate with positive backlash
                      //qDebug()<<"Compensating for Y Backlash";
                      BacklashDirections[2]=true;
                      CompensationState=1;
                      DeltaY = YposVal - PosY;
                      PosY = YposVal;
                    }
                     else if ((YposVal < PosY) && (DeltaY >= 0)){
                      //Compensate with positive backlash
                      //qDebug()<<"Compensating for Y Backlash";
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
                    //qDebug()<<"Z Destination: "<<ZposVal;
                    if (RelativeEnabled|RelativeOverride){
                        ZposVal = ZposVal + PosZ;
                    }
                    if ((ZposVal > PosZ) && (DeltaZ < 0)){
                      //Compensate with positive backlash
                      //qDebug()<<"Compensating for Z Backlash";
                      BacklashDirections[4]=true;
                      CompensationState=1;
                      DeltaZ = ZposVal - PosZ;
                      PosZ = ZposVal;
                    }
                     else if ((ZposVal < PosZ) && (DeltaZ >= 0)){
                      //Compensate with positive backlash
                      //qDebug()<<"Compensating for Z Backlash";
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
                //BacklashTimer->start(1);
                CommandMemory=Command;
                MachineReady =true;
                BacklashSlot();
                return "";
            }
        }
        serial.write(CommandBytes + "\n"); //Send command & append new line characer
        ui->tbxCommandHistory->append("S: " + Command); //Add the command to the history
    } else {
        qDebug() << "Not connected";
        ui->tbxCommandHistory->append("ERROR: Failure to send command - not connected");
    }
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
    while(true)
    {
        QString line = textStream.readLine();
        if (line.isNull())
            break;
        else{
            ui->txtGCode->append(line);
           }
    }
    FindGCodeFootPrint();

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
    GCode.append(ui->txtCommand->text());
    if (~(StreamTimer->isActive()))
    {
    lineNumber=0;
    StreamTimer->start(1);
    }

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
    SendGCode("M84");//Disable motors This only works in Marlin
    int rows = 4;
    int columns = 3;
    float* array = new float[rows*columns];
    for (int i=0; i<=columns-1; i++)
    {
        for (int j=0; j<=rows-1; j++)
        {
            array[j+i*(columns+1)]=j+i*(columns+1);
            qDebug()<<"X"<<i<<"Y"<<j<<array[j+i*(columns+1)];
        }
    }
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

    QFile textFile(FilePath);
    textFile.open(QIODevice::ReadOnly | QIODevice::Text);
    QTextStream textStream(&textFile);
    GCode.clear();
    float ZPrev = INFINITY;
    while(true)
    {
        QString line = textStream.readLine();
        if (line.isNull())
            break;
        else{
            //Conform the Line
            line=GCodeLib::ConformGCodeCommand(line,ZPrev,MeasurementArray,PointsX,PointsY,0,0,SizeX/(PointsX-1),SizeY/(PointsY-1));
            ZPrev = GCodeLib::ExtractZValue(line);
            GCode.append(line);
           }
    }

    StreamTimer->start(1);
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
    ProbeTimer->start(500);
    ProbeState = 0;
    delete [] MeasurementArray;
    MeasurementArray=new float[PointsX*PointsY];
    //Prompt to append G-Code
    //If Yes - Append G-Code
    //If No - do not append, but keep probe data
}

void MainWindow::on_btnSpindleOff_released()
{
    SendGCode("M107");
}

void MainWindow::on_btnAbortProbe_released()
{
    ProbeTimer->stop();
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

void MainWindow::on_txtBacklashX_textChanged(const QString &arg1)
{
    BacklashX = ui->txtBacklashX->text().toFloat();
}

void MainWindow::on_txtBacklashY_textChanged(const QString &arg1)
{
    BacklashY= ui->txtBacklashY->text().toFloat();
}

void MainWindow::on_txtBacklashNegX_textChanged(const QString &arg1)
{
    BacklashNegX = ui->txtBacklashNegX->text().toFloat();
}

void MainWindow::on_txtBacklashNegY_textChanged(const QString &arg1)
{
    BacklashNegY= ui->txtBacklashNegY->text().toFloat();
}

void MainWindow::on_txtBacklashZ_textChanged(const QString &arg1)
{
    BacklashZ = ui->txtBacklashZ->text().toFloat();
}

void MainWindow::on_txtBacklashNegZ_textChanged(const QString &arg1)
{
    BacklashNegZ = ui->txtBacklashNegZ->text().toFloat();
}

void MainWindow::on_pushButton_released()
{
    //GCodeLib::LoadSubstitutionList();
    SendGCode("M106 P1 S255");//Turn on the fan
    SendGCode("G91");//Relative Movement Mode
    SendGCode("M106 P0 S255");//Turn on the welder
    SendGCode("G1 X10 E20 F1000");//Deposit Some Material
    //SendGCode("G4 P500");//Wait
    SendGCode("M106 P0 S00");//Turn off the welder
}
