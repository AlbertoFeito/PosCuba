#include "Money.h"

namespace {

qint64 pow10(int n)
{
    qint64 r = 1;
    while (n-- > 0)
        r *= 10;
    return r;
}

bool allDigits(const QString &s)
{
    for (const QChar c : s) {
        if (c < QLatin1Char('0') || c > QLatin1Char('9'))
            return false;
    }
    return true;
}

bool parseFixed(const QString &text, int decimals, qint64 *out)
{
    QString s = text.trimmed();
    s.remove(QLatin1Char(' '));
    s.replace(QLatin1Char(','), QLatin1Char('.'));
    if (s.isEmpty() || s == QLatin1String("."))
        return false;

    const int dot = s.indexOf(QLatin1Char('.'));
    if (dot != s.lastIndexOf(QLatin1Char('.')))
        return false;

    const QString intPart = dot < 0 ? s : s.left(dot);
    const QString fracPart = dot < 0 ? QString() : s.mid(dot + 1);
    if (!allDigits(intPart) || !allDigits(fracPart))
        return false;
    if (fracPart.size() > decimals || intPart.size() > 12)
        return false;

    const qint64 i = intPart.isEmpty() ? 0 : intPart.toLongLong();
    const qint64 f = fracPart.isEmpty()
                         ? 0
                         : fracPart.toLongLong() * pow10(decimals - fracPart.size());
    if (out)
        *out = i * pow10(decimals) + f;
    return true;
}

QString formatFixed(qint64 value, int decimals)
{
    const qint64 base = pow10(decimals);
    const bool neg = value < 0;
    const qint64 abs = neg ? -value : value;
    return QStringLiteral("%1%2.%3")
        .arg(neg ? QStringLiteral("-") : QString())
        .arg(abs / base)
        .arg(abs % base, decimals, 10, QLatin1Char('0'));
}

} // namespace

bool Money::parseCents(const QString &text, qint64 *out)
{
    return parseFixed(text, 2, out);
}

QString Money::formatCents(qint64 cents)
{
    return formatFixed(cents, 2);
}

bool Money::parseMilli(const QString &text, qint64 *out)
{
    return parseFixed(text, 3, out);
}

QString Money::formatMilli(qint64 milli)
{
    QString s = formatFixed(milli, 3);
    while (s.endsWith(QLatin1Char('0')))
        s.chop(1);
    if (s.endsWith(QLatin1Char('.')))
        s.chop(1);
    return s;
}
