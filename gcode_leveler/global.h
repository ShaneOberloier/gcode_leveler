#ifndef GLOBAL_H
#define GLOBAL_H

#endif // GLOBAL_H

#include <QString>
#include <QSerialPort>

//Here are all of the global parameters referenced by the program


//Tied to GUI elements
QString FilePath;
QString ComPort;
QString Baud;
QString ConnectionStatus;
float SizeX = 0;
float SizeY = 0;
int PointsX = 0;
int PointsY = 0;
float Measurement = 0;
bool MeasRecorded = false;
bool ProbeComplete = false;
bool ApplyProbeData = true;
int ProbeState=0;
int ProbeX=0; //X coordinate in probing array
int ProbeY=0; //Y coordinate in probing array
float MeasurementArray[256];
float Increment = 10;
float Feedrate = 500;
float MovementSpeed = 1000;
float ProbingSpeed = 200;
float TravelClearance = 5;
bool CompensateBacklash = true;
bool MessageIncoming = false;
int CompensationState = 0;
bool BacklashDirections[6] ={0,0,0,0,0,0};
float BacklashX = 0;//16
float BacklashNegX = 0;//16
float BacklashY = 0;//0.0508;//.1
float BacklashNegY = 0;//0.0508;//.1
float BacklashZ = 0;//0.0508;
float BacklashNegZ = 0;//0.0508;
QString RelativeMovement = "G91";
QString AbsoluteMovement = "G90";
QString SetPosition = "G92";
QString Movement = "G1";
QString CMDComplete;
QString EndstopHit;
/*//////////////////////*/

//Variables for program book keeping
bool ProbeDataExists =false;
QString MeasX = 0; //Used to store values during backlash compensation
QString MeasY = 0; //Used to store values during backlash compensation
QString MeasZ = 0; //Used to store values during backlash compensation
float PosX = 0; //Tracking position of X motors
float PosY = 0; //Tracking position of Y motors
float PosZ = 0; //Tracking position of Z motors
float DeltaX = 0;
float DeltaY = 0;
float DeltaZ = 0;
bool RelativeEnabled = 0;
bool RelativeOverride = 0;
int lineNumber=0;
QString Response ="";
QString CommandMemory="";
bool MachineReady =0;
/*//////////////////////*/

//Program resources
QSerialPort serial;
QStringList GCode;
