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
float SizeX;
float SizeY;
int PointsX;
int PointsY;
float Increment = 10;
float Feedrate = 200;
float MovementSpeed;
float ProbingSpeed;
float TravelClearance;
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
/*//////////////////////*/

//Program resources
QSerialPort serial;
QStringList GCode;
