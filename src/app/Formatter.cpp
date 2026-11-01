#include "Formatter.h"

#include <cmath>

#include "core/Money.h"

QString Formatter::money(double cents) const
{
    return Money::formatCents(static_cast<qint64>(std::llround(cents)));
}

double Formatter::parseMoney(const QString &text) const
{
    qint64 v = 0;
    return Money::parseCents(text, &v) ? static_cast<double>(v) : -1.0;
}

QString Formatter::qty(double milli) const
{
    return Money::formatMilli(static_cast<qint64>(std::llround(milli)));
}

double Formatter::parseQty(const QString &text) const
{
    qint64 v = 0;
    return Money::parseMilli(text, &v) ? static_cast<double>(v) : -1.0;
}
