#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

namespace PaymentMethod {
inline QString cash() { return QStringLiteral("cash"); }
inline QString transfer() { return QStringLiteral("transfer"); }
} // namespace PaymentMethod

struct CartLine
{
    int productId = 0;
    QString code;
    QString name;
    QString unit;
    qint64 qtyMilli = 0;
    qint64 priceCents = 0;   // precio unitario
    qint64 costCents = 0;    // costo unitario (snapshot para la ganancia)
    qint64 stockMilli = 0;   // stock al agregar (solo informativo)
};

struct Payment
{
    QString method;          // PaymentMethod::cash() / transfer()
    QString currency;        // codigo de moneda
    qint64 amountCents = 0;  // en la moneda del pago
    qint64 rateMilli = 1000; // 1 unidad = rate/1000 moneda base
    QString reference;       // nro. de transferencia
};

struct SaleTotals
{
    qint64 subtotal = 0;
    qint64 discount = 0;
    qint64 total = 0;
    qint64 paid = 0;          // todo en moneda base
    qint64 cashPaid = 0;
    qint64 transferPaid = 0;
    qint64 change = 0;        // vuelto, en efectivo y moneda base
    qint64 remaining = 0;     // falta por pagar
};

struct CashSession
{
    int id = 0;
    int openedBy = 0;
    QString openedByName;
    QString openedAt;
    QString closedAt;
    qint64 openingCents = 0;
    qint64 expectedCents = 0;
    qint64 countedCents = 0;
    qint64 differenceCents = 0;
    QString status;
};

struct CurrencyAmount
{
    QString currency;
    qint64 rateMilli = 1000;
    qint64 expectedCents = 0;
    qint64 countedCents = 0;
};

struct CashSummary
{
    int salesCount = 0;
    qint64 salesTotal = 0;      // total vendido (moneda base)
    qint64 cashSales = 0;       // efectivo recibido menos vuelto (moneda base)
    qint64 transferSales = 0;
    qint64 movementsIn = 0;     // ingresos manuales
    qint64 movementsOut = 0;    // retiros (positivo)
    qint64 profit = 0;          // venta - costo
    QVector<CurrencyAmount> expected; // efectivo esperado por moneda (incluye fondo inicial)
};

struct CheckoutResult
{
    bool ok = false;
    int saleId = 0;
    int number = 0;
    qint64 change = 0;
    QString error;
    QStringList stockIssues; // productos que quedarian en negativo (requiere confirmacion)
};

// Venta registrada, leida de la base de datos (ticket, reimpresion, reportes).
struct SaleItemDetail
{
    QString name;
    QString unit;
    qint64 qtyMilli = 0;
    qint64 priceCents = 0;
    qint64 lineTotalCents = 0;
};

struct PaymentDetail
{
    QString method;
    QString currency;
    qint64 amountCents = 0;
    qint64 baseCents = 0;
    QString reference;
};

struct SaleDetail
{
    int id = 0;
    int number = 0;
    QString createdAt;      // "YYYY-MM-DD HH:MM:SS"
    QString userName;
    QString status;
    qint64 subtotal = 0;
    qint64 discount = 0;
    qint64 total = 0;
    qint64 change = 0;
    QVector<SaleItemDetail> items;
    QVector<PaymentDetail> payments;
};

struct SaleRow
{
    int id = 0;
    int number = 0;
    QString createdAt;
    QString userName;
    qint64 total = 0;
    int itemCount = 0;
    QString status;
};
