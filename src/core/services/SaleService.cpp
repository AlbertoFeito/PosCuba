#include "SaleService.h"

#include <QCoreApplication>
#include <QMap>

#include "core/Money.h"
#include "core/SaleCalculator.h"
#include "data/Transaction.h"

namespace {
QString tr(const char *s) { return QCoreApplication::translate("SaleService", s); }

CheckoutResult fail(const QString &msg)
{
    CheckoutResult r;
    r.error = msg;
    return r;
}
} // namespace

SaleService::SaleService(QSqlDatabase db)
    : m_db(db)
    , m_sales(db)
    , m_cash(db)
    , m_currencies(db)
    , m_inventory(db)
{
}

CheckoutResult SaleService::checkout(const QVector<CartLine> &lines, qint64 discountCents,
                                     QVector<Payment> payments, int userId, bool allowNegativeStock)
{
    const QString cartError = SaleCalculator::validateCart(lines, discountCents);
    if (!cartError.isEmpty())
        return fail(cartError);
    if (userId <= 0)
        return fail(tr("No hay un usuario activo."));

    // Tasas vigentes; se descartan pagos en cero.
    QVector<Payment> effective;
    for (Payment p : payments) {
        if (p.amountCents == 0)
            continue;
        Currency c;
        if (!m_currencies.get(p.currency, &c) || !c.active)
            return fail(tr("Moneda no disponible: %1.").arg(p.currency));
        p.rateMilli = c.isBase ? 1000 : c.rateMilli;
        effective.append(p);
    }

    const SaleTotals totals = SaleCalculator::compute(lines, discountCents, effective);
    const QString payError = SaleCalculator::validatePayments(totals, effective);
    if (!payError.isEmpty())
        return fail(payError);

    Transaction tx(m_db);
    if (!tx.isActive())
        return fail(tr("No se pudo iniciar la transacción."));

    CashSession session;
    if (!m_cash.current(&session))
        return fail(tr("No hay caja abierta. Abre la caja antes de vender."));

    // Cantidad total por producto (un producto puede aparecer en varias lineas).
    QMap<int, qint64> qtyByProduct;
    for (const CartLine &l : lines)
        qtyByProduct[l.productId] += l.qtyMilli;

    QStringList issues;
    for (auto it = qtyByProduct.constBegin(); it != qtyByProduct.constEnd(); ++it) {
        qint64 stock = 0;
        if (!m_inventory.products().stockOf(it.key(), &stock))
            return fail(tr("Un producto de la venta ya no existe."));
        if (stock - it.value() < 0) {
            QString name;
            for (const CartLine &l : lines) {
                if (l.productId == it.key()) {
                    name = l.name;
                    break;
                }
            }
            issues << tr("%1: hay %2, se venden %3").arg(name, Money::formatMilli(stock), Money::formatMilli(it.value()));
        }
    }
    if (!issues.isEmpty() && !allowNegativeStock) {
        CheckoutResult r = fail(tr("Stock insuficiente."));
        r.stockIssues = issues;
        return r;
    }

    const int number = m_sales.nextNumber();
    const int saleId = m_sales.insertSale(number, session.id, userId, totals);
    if (saleId <= 0)
        return fail(tr("No se pudo guardar la venta: %1").arg(m_sales.lastError()));

    for (const CartLine &l : lines) {
        if (!m_sales.insertItem(saleId, l, SaleCalculator::lineTotal(l)))
            return fail(tr("No se pudo guardar la venta: %1").arg(m_sales.lastError()));
    }
    for (const Payment &p : effective) {
        if (!m_sales.insertPayment(saleId, p, SaleCalculator::toBase(p.amountCents, p.rateMilli)))
            return fail(tr("No se pudo guardar el pago: %1").arg(m_sales.lastError()));
    }

    QString error;
    for (auto it = qtyByProduct.constBegin(); it != qtyByProduct.constEnd(); ++it) {
        StockChange c;
        c.productId = it.key();
        c.deltaMilli = -it.value();
        c.type = MovementType::sale();
        c.saleId = saleId;
        c.userId = userId;
        c.note = tr("Venta #%1").arg(number);
        if (!m_inventory.applyChange(c, true, nullptr, &error))
            return fail(error);
    }

    if (!tx.commit())
        return fail(tr("No se pudo confirmar la venta."));

    CheckoutResult r;
    r.ok = true;
    r.saleId = saleId;
    r.number = number;
    r.change = totals.change;
    return r;
}
