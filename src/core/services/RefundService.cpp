#include "RefundService.h"

#include <QCoreApplication>
#include <cmath>

#include "core/Money.h"
#include "data/Transaction.h"

namespace {
QString tr(const char *s) { return QCoreApplication::translate("RefundService", s); }
void setErr(QString *error, const QString &msg) { if (error) *error = msg; }

RefundResult fail(const QString &msg)
{
    RefundResult r;
    r.error = msg;
    return r;
}
} // namespace

RefundService::RefundService(QSqlDatabase db)
    : m_db(db)
    , m_refunds(db)
    , m_cashRepo(db)
    , m_cash(db)
    , m_inventory(db)
{
}

bool RefundService::calculate(const SaleRefundState &state, const QMap<int, qint64> &qtyByItem, bool all,
                              RefundCalc *out, QString *error)
{
    if (state.voided()) {
        setErr(error, tr("La venta #%1 ya está anulada.").arg(state.number));
        return false;
    }

    QMap<int, qint64> qty;
    if (all) {
        for (const RefundableItem &it : state.items) {
            if (it.availableMilli() > 0)
                qty.insert(it.saleItemId, it.availableMilli());
        }
    } else {
        for (auto it = qtyByItem.constBegin(); it != qtyByItem.constEnd(); ++it) {
            if (it.value() < 0) {
                setErr(error, tr("Las cantidades no pueden ser negativas."));
                return false;
            }
            if (it.value() > 0)
                qty.insert(it.key(), it.value());
        }
    }
    if (qty.isEmpty()) {
        setErr(error, tr("Indica qué productos se devuelven."));
        return false;
    }

    RefundCalc c;
    bool completes = true;
    int lastItem = 0;
    for (const RefundableItem &it : state.items) {
        const qint64 q = qty.value(it.saleItemId, 0);
        if (q > it.availableMilli()) {
            setErr(error, tr("No se puede devolver más de lo vendido de «%1» (quedan %2).")
                              .arg(it.name, Money::formatMilli(it.availableMilli())));
            return false;
        }
        if (it.availableMilli() - q > 0)
            completes = false;
        if (q == 0)
            continue;
        // Parte proporcional de la linea, con su parte del descuento general de la venta.
        long double amount = it.soldMilli > 0 ? (long double)it.lineTotalCents * q / it.soldMilli : 0;
        if (state.subtotal > 0)
            amount = amount * state.total / state.subtotal;
        else
            amount = 0;
        const qint64 cents = qint64(std::llround((double)amount));
        c.amountByItem.insert(it.saleItemId, cents);
        c.costByItem.insert(it.saleItemId, (q * it.costCents + 500) / 1000);
        c.total += cents;
        c.cost += (q * it.costCents + 500) / 1000;
        lastItem = it.saleItemId;
    }
    for (auto it = qty.constBegin(); it != qty.constEnd(); ++it) {
        bool found = false;
        for (const RefundableItem &i : state.items)
            found = found || i.saleItemId == it.key();
        if (!found) {
            setErr(error, tr("Una línea no pertenece a esta venta."));
            return false;
        }
    }

    // Al completar la venta se devuelve exactamente lo que falta (absorbe los redondeos);
    // nunca se devuelve mas que el total cobrado.
    const qint64 remaining = qMax<qint64>(0, state.total - state.refunded);
    const qint64 target = completes ? remaining : qMin(c.total, remaining);
    if (target != c.total && lastItem > 0) {
        c.amountByItem[lastItem] += target - c.total;
        c.total = target;
    }
    c.completesSale = completes;
    if (out)
        *out = c;
    return true;
}

RefundResult RefundService::refund(const RefundRequest &req)
{
    if (req.reason.trimmed().isEmpty())
        return fail(tr("Indica el motivo de la devolución."));
    if (req.method != PaymentMethod::cash() && req.method != PaymentMethod::transfer())
        return fail(tr("Forma de devolución no válida."));
    if (req.userId <= 0)
        return fail(tr("No hay un usuario activo."));
    if (req.authorizedBy <= 0)
        return fail(tr("La devolución debe autorizarla un administrador."));

    Transaction tx(m_db);
    if (!tx.isActive())
        return fail(tr("No se pudo iniciar la transacción."));

    SaleRefundState st;
    if (!m_refunds.loadState(req.saleId, &st))
        return fail(tr("No se encontró la venta."));

    RefundCalc calc;
    QString error;
    if (!calculate(st, req.qtyByItem, req.all, &calc, &error))
        return fail(error);

    CashSession session;
    if (!m_cashRepo.current(&session))
        return fail(tr("Abre la caja para hacer devoluciones."));

    if (req.method == PaymentMethod::cash()) {
        const qint64 available = m_cash.summary(session).expected.first().expectedCents;
        if (calc.total > available)
            return fail(tr("No hay suficiente efectivo en caja (hay %1). Devuelve por transferencia o registra un "
                           "ingreso en Caja.").arg(Money::formatCents(available)));
    }

    const int number = m_refunds.nextNumber();
    const int refundId = m_refunds.insertRefund(number, req, session.id, calc.total, calc.cost);
    if (refundId <= 0)
        return fail(tr("No se pudo guardar la devolución: %1").arg(m_refunds.lastError()));

    QMap<int, qint64> restockByProduct;
    for (const RefundableItem &it : st.items) {
        if (!calc.amountByItem.contains(it.saleItemId))
            continue;
        const qint64 q = req.all ? it.availableMilli() : req.qtyByItem.value(it.saleItemId);
        if (!m_refunds.insertItem(refundId, it.saleItemId, it.productId, q, calc.amountByItem.value(it.saleItemId),
                                  calc.costByItem.value(it.saleItemId)))
            return fail(tr("No se pudo guardar la devolución: %1").arg(m_refunds.lastError()));
        restockByProduct[it.productId] += q;
    }

    if (req.restock) {
        for (auto it = restockByProduct.constBegin(); it != restockByProduct.constEnd(); ++it) {
            StockChange c;
            c.productId = it.key();
            c.deltaMilli = it.value();
            c.type = MovementType::voidSale();
            c.saleId = st.saleId;
            c.userId = req.userId;
            c.note = calc.completesSale ? tr("Anulación · venta #%1").arg(st.number)
                                        : tr("Devolución #%1 · venta #%2").arg(number).arg(st.number);
            if (!m_inventory.applyChange(c, true, nullptr, &error))
                return fail(error);
        }
    }

    if (calc.completesSale && !m_refunds.setSaleStatus(st.saleId, QStringLiteral("voided")))
        return fail(tr("No se pudo anular la venta: %1").arg(m_refunds.lastError()));

    if (!tx.commit())
        return fail(tr("No se pudo confirmar la devolución."));

    RefundResult r;
    r.ok = true;
    r.refundId = refundId;
    r.number = number;
    r.total = calc.total;
    r.voided = calc.completesSale;
    return r;
}
