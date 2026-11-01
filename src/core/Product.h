#pragma once

#include <QString>
#include <QtGlobal>

struct Product
{
    int id = 0;
    QString code;
    QString barcode;
    QString name;
    int categoryId = 0;       // 0 = sin categoria
    QString categoryName;     // solo lectura (JOIN)
    qint64 costCents = 0;
    qint64 priceCents = 0;
    QString unit = QStringLiteral("u");
    qint64 stockMilli = 0;    // solo lectura aqui; lo cambia Inventario
    qint64 minStockMilli = 0;
    bool active = true;

    bool isLowStock() const { return minStockMilli > 0 && stockMilli <= minStockMilli; }
};

struct Category
{
    int id = 0;
    QString name;
};
