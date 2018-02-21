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
float SizeX = 20;
float SizeY = 20;
int PointsX = 4;
int PointsY = 4;
float Measurement = 0;
bool MeasRecorded = false;
int ProbeState=0;
int ProbeX=0; //X coordinate in probing array
int ProbeY=0; //Y coordinate in probing array
float Increment = 10;
float Feedrate = 500;
float MovementSpeed = 500;
float ProbingSpeed = 500;
float TravelClearance = 5;
bool CompensateBacklash = true;
float BacklashX = 0;
float BacklashY = 0;
float BacklashZ = 0;
QString RelativeMovement = "G91";
QString AbsoluteMovement = "G90";
QString SetPosition = "G92";
QString Movement = "G1";
QString CMDComplete;
QString EndstopHit;
/*//////////////////////*/

//Variables for program book keeping
bool ProbeDataExists =false;
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
bool MachineReady =0;
/*//////////////////////*/

//Program resources
QSerialPort serial;
QStringList GCode;
