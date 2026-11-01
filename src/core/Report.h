#pragma once

#include <QDate>
#include <QString>
#include <QVector>
#include <QtGlobal>

enum class ReportGroup { Day, Week, Month };
enum class TopOrder { Quantity, Revenue, Profit };

struct PeriodRow
{
    QDate start;          // dia, lunes de la semana o dia 1 del mes
    QString label;
    int salesCount = 0;
    qint64 total = 0;     // vendido (con descuentos aplicados)
    qint64 cost = 0;
    qint64 profit = 0;
    qint64 discount = 0;
};

struct ProductRow
{
    int productId = 0;
    QString name;
    QString unit;
    qint64 qtyMilli = 0;
    qint64 revenue = 0;   // importes de linea (antes del descuento general de la venta)
    qint64 cost = 0;
    qint64 profit = 0;
};

struct ReportSummary
{
    int salesCount = 0;
    qint64 total = 0;
    qint64 cost = 0;
    qint64 profit = 0;
    qint64 discount = 0;
    qint64 cash = 0;        // efectivo neto (recibido menos vuelto), moneda base
    qint64 transfer = 0;
    qint64 averageTicket = 0;
    int marginPercent = 0;  // ganancia / total
};

struct SaleReportRow
{
    int number = 0;
    QString createdAt;
    QString userName;
    qint64 subtotal = 0;
    qint64 discount = 0;
    qint64 total = 0;
    qint64 cost = 0;
    qint64 cash = 0;
    qint64 transfer = 0;
};
