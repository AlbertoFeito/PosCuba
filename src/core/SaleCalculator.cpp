#include "SaleCalculator.h"

#include <QCoreApplication>

#include "core/Money.h"

namespace {
QString tr(const char *s) { return QCoreApplication::translate("SaleCalculator", s); }

// a * b / d redondeado (a, b, d >= 0)
qint64 mulDivRound(qint64 a, qint64 b, qint64 d)
{
    return (a * b + d / 2) / d;
}
} // namespace

qint64 SaleCalculator::lineTotal(const CartLine &line)
{
    return mulDivRound(line.qtyMilli, line.priceCents, 1000);
}

qint64 SaleCalculator::subtotal(const QVector<CartLine> &lines)
{
    qint64 s = 0;
    for (const CartLine &l : lines)
        s += lineTotal(l);
    return s;
}

qint64 SaleCalculator::percentOf(qint64 amount, qint64 percentHundredths)
{
    return mulDivRound(amount, percentHundredths, 10000);
}

qint64 SaleCalculator::toBase(qint64 amountCents, qint64 rateMilli)
{
    return mulDivRound(amountCents, rateMilli, 1000);
}

SaleTotals SaleCalculator::compute(const QVector<CartLine> &lines, qint64 discountCents,
                                   const QVector<Payment> &payments)
{
    SaleTotals t;
    t.subtotal = subtotal(lines);
    t.discount = qBound<qint64>(0, discountCents, t.subtotal);
    t.total = t.subtotal - t.discount;

    for (const Payment &p : payments) {
        const qint64 base = toBase(p.amountCents, p.rateMilli);
        if (p.method == PaymentMethod::transfer())
            t.transferPaid += base;
        else
            t.cashPaid += base;
    }
    t.paid = t.cashPaid + t.transferPaid;
    t.remaining = qMax<qint64>(0, t.total - t.paid);
    t.change = qMax<qint64>(0, t.paid - t.total);
    return t;
}

QString SaleCalculator::validateCart(const QVector<CartLine> &lines, qint64 discountCents)
{
    if (lines.isEmpty())
        return tr("La venta está vacía.");
    for (const CartLine &l : lines) {
        if (l.qtyMilli <= 0)
            return tr("La cantidad de «%1» debe ser mayor que cero.").arg(l.name);
        if (l.qtyMilli > MaxQtyMilli)
            return tr("La cantidad de «%1» es demasiado grande.").arg(l.name);
        if (l.priceCents < 0 || l.priceCents > MaxPriceCents)
            return tr("El precio de «%1» no es válido.").arg(l.name);
    }
    if (discountCents < 0)
        return tr("El descuento no puede ser negativo.");
    if (discountCents > subtotal(lines))
        return tr("El descuento no puede ser mayor que el subtotal.");
    return QString();
}

QString SaleCalculator::validatePayments(const SaleTotals &t, const QVector<Payment> &payments)
{
    for (const Payment &p : payments) {
        if (p.amountCents < 0)
            return tr("Los pagos no pueden ser negativos.");
        if (p.rateMilli <= 0)
            return tr("Tasa de cambio no válida para %1.").arg(p.currency);
        if (p.method != PaymentMethod::cash() && p.method != PaymentMethod::transfer())
            return tr("Método de pago no válido.");
    }
    if (t.remaining > 0)
        return tr("Falta por pagar %1.").arg(Money::formatCents(t.remaining));
    // El vuelto solo puede salir del efectivo.
    if (t.transferPaid > t.total)
        return tr("La transferencia (%1) no puede superar el total (%2).")
            .arg(Money::formatCents(t.transferPaid), Money::formatCents(t.total));
    return QString();
}
