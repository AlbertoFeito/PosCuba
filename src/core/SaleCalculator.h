#pragma once

#include <QString>
#include <QVector>

#include "core/Sale.h"

// Calculos de venta sin dependencias de base de datos (testeables).
// Redondeo: al centavo mas cercano, mitades hacia arriba.
namespace SaleCalculator {

constexpr qint64 MaxQtyMilli = 100000000;   // 100 000 unidades por linea
constexpr qint64 MaxPriceCents = 10000000000; // 100 millones

qint64 lineTotal(const CartLine &line);
qint64 subtotal(const QVector<CartLine> &lines);
// percentHundredths: 1250 = 12.50 %
qint64 percentOf(qint64 amount, qint64 percentHundredths);
qint64 toBase(qint64 amountCents, qint64 rateMilli);

SaleTotals compute(const QVector<CartLine> &lines, qint64 discountCents, const QVector<Payment> &payments);

// Valida carrito y descuento (antes de cobrar). Vacio si esta bien.
QString validateCart(const QVector<CartLine> &lines, qint64 discountCents);
// Valida que los pagos cubran el total. Vacio si esta bien.
QString validatePayments(const SaleTotals &totals, const QVector<Payment> &payments);

} // namespace SaleCalculator
