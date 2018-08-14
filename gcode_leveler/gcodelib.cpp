#include <QtCore/QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <QString>
#include <QStringList>
#include "gcodelib.h"

GCodeLib::GCodeLib()
{

}

float Info[3][4] = {{0,1,2,3}, {4,5,6,7}, {8,9,10,11}};

float GCodeLib::Interpolate3D(float X, float Y, float Z, float Data[], int XPoints, int YPoints, float XDatum, float YDatum, float XDelta, float YDelta)
{
    //Declare Function variables
    float X1, Y1, Z1, X2, Y2, Z2, X3, Y3, Z3;
    int Xindex, Yindex;
    //Calculate function constants
    //Find which data points to use
    for(int i=1; i<=XPoints; i++)
    {
        if (X<=(i*XDelta+XDatum))
        {
            //We've found the X reference point
            X1 = i*XDelta+XDatum;
            Xindex = i;
            break;
        }
    }
    for(int i=1; i<=YPoints; i++)
    {
        if (Y<=(i*YDelta+YDatum))
        {
            //We've found the Y reference point
            Y1 = i*YDelta+YDatum;
            Yindex = i;
            break;
        }
    }
    Z1 = Data[Yindex+Xindex*(XPoints+1)];
    //Declare the second point
    X2 = X1 - XDelta;
    Y2 = Y1 - YDelta;
    Z2 = Data[(Yindex-1)+(Xindex-1)*(XPoints+1)];
    //Now we must determine the 3rd point
    if((X-X2)/XDelta > (Y-Y2)/YDelta)
    {
        X3 = X1;
        Y3 = Y1-YDelta;
        Z3 = Data[(Yindex-1)+Xindex*(XPoints+1)];
    }
    else
    {
        X3 = X1 - XDelta;
        Y3 = Y1;
        Z3 = Data[Yindex+(Xindex-1)*(XPoints+1)];
    }
//    //Debug Output
//    qDebug()<<"Original:"<<X<<Y<<Z;
//    qDebug()<<"Point 1:"<<X1<<Y1<<Z1;
//    qDebug()<<"Point 2:"<<X2<<Y2<<Z2;
//    qDebug()<<"Point 3:"<<X3<<Y3<<Z3;

    //Interpolate value
    float L = (Y2-Y1)*(Z3-Z1)-(Z2-Z1)*(Y3-Y1);
    float M = (Z2-Z1)*(X3-X1)-(X2-X1)*(Z3-Z1);
    float N = (X2-X1)*(Y3-Y1)-(Y2-Y1)*(X3-X1);
    float Zinterp = ((-1*L*(X-X2)-M*(Y-Y2))/N)+Z2;
    //Return
    return Zinterp+Z;
}

QString GCodeLib::ExtractPrefix(QString Command)
{
        QRegExp rx("(\\ |\\X|\\Y|\\Z)"); //RegEx for ' ' or 'X' or 'Y' or 'Z'
        QStringList CommandParts = Command.split(rx);
        return CommandParts[0];
}

bool GCodeLib::IsMovementCommand(QString Command)
{
    QString Prefix = ExtractPrefix(Command);
    if((Prefix == "G1")|(Prefix == "G0")|(Prefix == "G01")|(Prefix == "G00"))
        return true;
    else
        return false;
}

QString GCodeLib::InsertSpaces(QString Command)
{
    if (!Command.contains(" X"))
        Command.replace("X"," X");
    if (!Command.contains(" Y"))
        Command.replace("Y"," Y");
    if (!Command.contains(" Z"))
        Command.replace("Z"," Z");
    if (!Command.contains(" F"))
        Command.replace("F"," F");
    return Command;
}

float GCodeLib::ExtractXValue(QString Command)
{
    if(GCodeLib::IsMovementCommand(Command))
    {
        QStringList CommandParts = Command.split(" ");
        if (Command.contains("X"))
        {
            QString Xpos = CommandParts.filter("X").at(0);
            Xpos = Xpos.mid(1);
            float XposVal = Xpos.toFloat();
            return XposVal;
        }
        else
        {
            return INFINITY;
        }
    }
    else
    {
        return INFINITY;
    }
}

float GCodeLib::ExtractYValue(QString Command)
{
    if(GCodeLib::IsMovementCommand(Command))
    {
        QStringList CommandParts = Command.split(" ");
        if (Command.contains("Y"))
        {
            QString Ypos = CommandParts.filter("Y").at(0);
            Ypos = Ypos.mid(1);
            float YposVal = Ypos.toFloat();
            return YposVal;
        }
        else
        {
            return INFINITY;
        }
    }
    else
    {
        return INFINITY;
    }
}

float GCodeLib::ExtractZValue(QString Command)
{
    if(GCodeLib::IsMovementCommand(Command))
    {
        QStringList CommandParts = Command.split(" ");
        if (Command.contains("Z"))
        {
            QString Zpos = CommandParts.filter("Z").at(0);
            Zpos = Zpos.mid(1);
            float ZposVal = Zpos.toFloat();
            return ZposVal;
        }
        else
        {
            return INFINITY;
        }
    }
    else
    {
        return INFINITY;
    }
}

float GCodeLib::ExtractFeedRate(QString Command)
{
    if(GCodeLib::IsMovementCommand(Command))
    {
        QStringList CommandParts = Command.split(" ");
        if (Command.contains("F"))
        {
            QString Feed = CommandParts.filter("F").at(0);
            Feed = Feed.mid(1);
            float FeedVal = Feed.toFloat();
            return FeedVal;
        }
        else
        {
            return INFINITY;
        }
    }
    else
    {
        return INFINITY;
    }
}

QString GCodeLib::ConformGCodeCommand(QString Command, float ZPrevious, float Data[], int XPoints, int YPoints, float XDatum, float YDatum, float XDelta, float YDelta)
{
    if(GCodeLib::IsMovementCommand(Command))
    {
        float X = ExtractXValue(Command);
        float Y = ExtractYValue(Command);
        float Z = ExtractZValue(Command);
        float Feed = ExtractFeedRate(Command);
        if ((Z == INFINITY) & (ZPrevious != INFINITY))
            Z = ZPrevious;

        float ZNew = GCodeLib::Interpolate3D(X,Y,Z,Data,XPoints,YPoints,XDatum,YDatum,XDelta,YDelta);
        //Construct the String with new Z Value
        QString CommandNew = ExtractPrefix(Command);
        if (X != INFINITY)
            CommandNew += " X" + QString::number(X);
        if (Y != INFINITY)
            CommandNew += " Y" + QString::number(Y);
        if (Z != INFINITY)
            CommandNew += " Z" + QString::number(ZNew);
        if (Feed != INFINITY)
            CommandNew += " F" + QString::number(Feed);
        return CommandNew;
    }
    else
    {
        return Command;
    }
}

QString GCodeLib::RelativeToAbsolute(QString Command,float XPrevious,float YPrevious, float ZPrevious)
{
    if(GCodeLib::IsMovementCommand(Command))
    {
        float X = ExtractXValue(Command);
        float Y = ExtractYValue(Command);
        float Z = ExtractZValue(Command);
        float Feed = ExtractFeedRate(Command);

        QString CommandNew = ExtractPrefix(Command);
        if (X !=INFINITY)
            CommandNew += " X" + QString::number(X+XPrevious);
        if (Y !=INFINITY)
            CommandNew += " Y" + QString::number(Y+YPrevious);
        if (Z !=INFINITY)
            CommandNew += " Z" + QString::number(Z+ZPrevious);
        if (Feed != INFINITY)
            CommandNew += " F" + QString::number(Feed);
        return CommandNew;
    }
    else
    {
        return Command;
    }
}

QString GCodeLib::AbsoluteToRelative(QString Command,float XPrevious,float YPrevious, float ZPrevious)
{
    if(GCodeLib::IsMovementCommand(Command))
    {
        float X = ExtractXValue(Command);
        float Y = ExtractYValue(Command);
        float Z = ExtractZValue(Command);
        float Feed = ExtractFeedRate(Command);

        QString CommandNew = ExtractPrefix(Command);
        if (X !=INFINITY)
            CommandNew += " X" + QString::number(X-XPrevious);
        if (Y !=INFINITY)
            CommandNew += " Y" + QString::number(Y-YPrevious);
        if (Z !=INFINITY)
            CommandNew += " Z" + QString::number(Z-ZPrevious);
        if (Feed != INFINITY)
            CommandNew += " F" + QString::number(Feed);
        return CommandNew;
    }
    else
    {
        return Command;
    }
}
