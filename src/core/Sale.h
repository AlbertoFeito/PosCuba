#pragma once

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

namespace PaymentMethod {
inline QString cash() { return QStringLiteral("cash"); }
inline QString transfer() { return QStringLiteral("transfer"); }
// A credito (fiado): lo que el cliente queda debiendo. No se guarda en payments sino en sales.credit_cents.
// En devoluciones: rebajar la deuda del cliente en lugar de entregar dinero.
inline QString credit() { return QStringLiteral("credit"); }
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
    qint64 creditPaid = 0;    // queda a credito (fiado)
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
    qint64 salesTotal = 0;      // total vendido (moneda base), ya restadas las devoluciones
    int refundsCount = 0;
    qint64 refundsTotal = 0;    // devuelto en esta caja (efectivo + transferencia)
    qint64 cashSales = 0;       // efectivo recibido menos vuelto (moneda base)
    qint64 transferSales = 0;
    qint64 movementsIn = 0;     // ingresos manuales
    qint64 movementsOut = 0;    // retiros (positivo)
    qint64 profit = 0;          // venta - costo
    qint64 creditSales = 0;     // vendido a credito (fiado), menos lo devuelto a la cuenta
    qint64 creditPaymentsCash = 0;      // abonos de clientes en efectivo (entran a la gaveta)
    qint64 creditPaymentsTransfer = 0;  // abonos por transferencia
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

struct RefundInfo
{
    int number = 0;
    QString createdAt;
    QString userName;
    QString method;           // PaymentMethod::cash() / transfer() / credit() (a la cuenta del cliente)
    QString reason;
    qint64 total = 0;
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
    qint64 credit = 0;      // a credito (fiado)
    QString customerName;
    QVector<SaleItemDetail> items;
    QVector<PaymentDetail> payments;
    QVector<RefundInfo> refunds;

    qint64 refundedTotal() const
    {
        qint64 t = 0;
        for (const RefundInfo &r : refunds)
            t += r.total;
        return t;
    }
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
    qint64 refunded = 0;
};

// ---- Devoluciones ----

struct RefundableItem
{
    int saleItemId = 0;
    int productId = 0;
    QString name;
    QString unit;
    qint64 soldMilli = 0;
    qint64 returnedMilli = 0;
    qint64 priceCents = 0;
    qint64 lineTotalCents = 0;
    qint64 costCents = 0;     // costo unitario al vender

    qint64 availableMilli() const { return soldMilli - returnedMilli; }
};

// Estado de una venta frente a devoluciones.
struct SaleRefundState
{
    int saleId = 0;
    int number = 0;
    QString createdAt;
    QString status;
    qint64 subtotal = 0;
    qint64 total = 0;
    qint64 refunded = 0;      // ya devuelto (moneda base)
    int customerId = 0;       // venta a credito: se puede devolver a la cuenta del cliente
    QString customerName;
    QVector<RefundableItem> items;

    bool voided() const { return status == QLatin1String("voided"); }
};

// Calculo de una devolucion (sin escribir nada).
struct RefundCalc
{
    QMap<int, qint64> amountByItem;   // sale_item_id -> importe a devolver
    QMap<int, qint64> costByItem;     // sale_item_id -> costo de lo devuelto
    qint64 total = 0;
    qint64 cost = 0;
    bool completesSale = false;       // tras esta devolucion no queda nada: la venta se anula
};

struct RefundRequest
{
    int saleId = 0;
    QMap<int, qint64> qtyByItem;      // sale_item_id -> cantidad (milesimas)
    bool all = false;                 // devolver todo lo que queda (anular)
    QString method = PaymentMethod::cash();   // o transfer(), o credit() = rebajar la deuda del cliente
    QString reason;
    bool restock = true;              // reintegrar al inventario
    int userId = 0;
    int authorizedBy = 0;             // administrador que autoriza
};

struct RefundResult
{
    bool ok = false;
    int refundId = 0;
    int number = 0;
    qint64 total = 0;
    bool voided = false;
    QString error;
};

struct RefundTotals
{
    int count = 0;
    qint64 total = 0;
    qint64 cost = 0;
    qint64 cash = 0;
    qint64 transfer = 0;
    qint64 account = 0;    // a la cuenta del cliente (no mueve dinero)
};
