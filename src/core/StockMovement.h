#pragma once

#include <QString>
#include <QtGlobal>

namespace MovementType {
inline QString purchase() { return QStringLiteral("purchase"); }
inline QString sale() { return QStringLiteral("sale"); }
inline QString adjust() { return QStringLiteral("adjust"); }
inline QString voidSale() { return QStringLiteral("void"); }
} // namespace MovementType

struct StockMovement
{
    int id = 0;
    int productId = 0;
    QString productName;      // solo lectura (JOIN)
    QString unit;             // solo lectura (JOIN)
    QString type;
    qint64 qtyMilli = 0;      // con signo
    qint64 costCents = -1;    // -1 = sin costo
    qint64 stockAfterMilli = 0;
    int saleId = 0;
    int userId = 0;
    QString userName;         // solo lectura (JOIN)
    QString note;
    QString createdAt;        // "YYYY-MM-DD HH:MM:SS" hora local
};

// Datos de entrada para aplicar un cambio de stock.
struct StockChange
{
    int productId = 0;
    qint64 deltaMilli = 0;
    QString type;
    qint64 costCents = -1;
    int saleId = 0;
    int userId = 0;
    QString note;
};
