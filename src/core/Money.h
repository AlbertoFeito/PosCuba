#pragma once

#include <QString>
#include <QtGlobal>

// Conversion texto <-> enteros de punto fijo.
// Dinero: centavos (2 decimales). Cantidades: milesimas (3 decimales).
// Acepta "." o "," como separador decimal. No acepta negativos.
namespace Money {

bool parseCents(const QString &text, qint64 *out);
QString formatCents(qint64 cents);

bool parseMilli(const QString &text, qint64 *out);
QString formatMilli(qint64 milli); // sin ceros sobrantes: "2", "0.25"

} // namespace Money
