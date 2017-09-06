#ifndef GLOBAL_H
#define GLOBAL_H

#endif // GLOBAL_H

#include <QString>
#include <QSerialPort>

//Here are all of the global parameters referenced by the program

//Tied to GUI elements
QString FilePath;
extern QString ComPort;
extern QString Baud;
extern QString ConnectionStatus;
extern float SizeX;
extern float SizeY;
extern int PointsX;
extern int PointsY;
extern float Increment;
extern float Feedrate;
extern float MovementSpeed;
extern float ProbingSpeed;
extern float TravelClearance;
extern bool CompensateBacklash;
extern float BacklashX;
extern float BacklashY;
extern float BacklashZ;
extern QString RelativeMovement;
extern QString SetPosition;
extern QString Movement;
extern QString CMDComplete;
extern QString EndstopHit;
/*//////////////////////*/

//Variables for program book keeping
extern bool ProbeDataExists =false;
/*//////////////////////*/

//Program resources
QSerialPort serial;
