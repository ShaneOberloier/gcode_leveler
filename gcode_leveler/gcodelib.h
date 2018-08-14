#ifndef GCODELIB_H
#define GCODELIB_H

#include <QtCore/QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QString>
#include <QStringList>


class GCodeLib
{
public:
    GCodeLib();
    static float Interpolate3D(float X, float Y, float Z, float Data[], int XPoints, int YPoints, float XDatum, float YDatum, float XDelta, float YDelta);
    static QString ExtractPrefix(QString Command);
    static bool IsMovementCommand(QString Command);
    static QString InsertSpaces(QString Command);
    static float ExtractXValue(QString Command);
    static float ExtractYValue(QString Command);
    static float ExtractZValue(QString Command);
    static float ExtractFeedRate(QString Command);
    static QString ConformGCodeCommand(QString Command,float XPrevious, float YPrevious, float ZPrevious, float Data[], int XPoints, int YPoints, float XDatum, float YDatum, float XDelta, float YDelta);
    static QString RelativeToAbsolute(QString Command,float XPrevious,float YPrevious, float ZPrevious);
    static QString AbsoluteToRelative(QString Command,float XPrevious,float YPrevious, float ZPrevious);
    static QString SubstituteCommand(QString Command);
};
#endif // GCODELIB_H
