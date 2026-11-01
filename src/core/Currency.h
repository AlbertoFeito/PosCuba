#pragma once

#include <QString>
#include <QtGlobal>

struct Currency
{
    QString code;
    QString name;
    QString symbol;
    qint64 rateMilli = 1000; // 1 unidad = rate/1000 moneda base
    bool isBase = false;
    bool active = true;
};
