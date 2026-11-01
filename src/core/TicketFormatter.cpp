#include "TicketFormatter.h"

#include <QCoreApplication>
#include <QDateTime>

#include "core/Money.h"

namespace {
QString tr(const char *s) { return QCoreApplication::translate("TicketFormatter", s); }
} // namespace

int TicketFormatter::columnsForPaper(int paperWidthMm)
{
    return paperWidthMm >= 80 ? 48 : 32;
}

QString TicketFormatter::center(const QString &text, int columns)
{
    const QString t = text.trimmed().left(columns);
    const int pad = (columns - t.size()) / 2;
    return QString(pad, QLatin1Char(' ')) + t;
}

QString TicketFormatter::justify(const QString &left, const QString &right, int columns)
{
    const QString r = right.left(columns);
    const int room = columns - r.size() - 1;
    QString l = left;
    if (l.size() > room)
        l = room > 1 ? l.left(room - 1) + QStringLiteral("…") : QString();
    return l + QString(columns - l.size() - r.size(), QLatin1Char(' ')) + r;
}

QStringList TicketFormatter::wrap(const QString &text, int columns, int indent)
{
    QStringList lines;
    const QString prefix(indent, QLatin1Char(' '));
    const int width = qMax(1, columns - indent);
    QString current;
    const QStringList words = text.simplified().split(QLatin1Char(' '));
    for (QString word : words) {
        while (word.size() > width) {   // palabra mas larga que la linea
            if (!current.isEmpty()) {
                lines << prefix + current;
                current.clear();
            }
            lines << prefix + word.left(width);
            word = word.mid(width);
        }
        if (word.isEmpty())
            continue;
        if (current.isEmpty())
            current = word;
        else if (current.size() + 1 + word.size() <= width)
            current += QLatin1Char(' ') + word;
        else {
            lines << prefix + current;
            current = word;
        }
    }
    if (!current.isEmpty())
        lines << prefix + current;
    return lines;
}

QStringList TicketFormatter::format(const SaleDetail &sale, const TicketInfo &info, int columns)
{
    QStringList out;
    const QString rule(columns, QLatin1Char('-'));

    // Encabezado
    if (!info.businessName.trimmed().isEmpty()) {
        for (const QString &l : wrap(info.businessName.toUpper(), columns))
            out << center(l, columns);
    }
    for (const QString &l : wrap(info.address, columns))
        out << center(l, columns);
    if (!info.phone.trimmed().isEmpty())
        out << center(tr("Tel: %1").arg(info.phone.trimmed()), columns);
    out << rule;

    const QDateTime dt = QDateTime::fromString(sale.createdAt, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    out << justify(tr("Ticket: %1").arg(sale.number, 6, 10, QLatin1Char('0')),
                   dt.isValid() ? dt.toString(QStringLiteral("dd/MM/yyyy HH:mm")) : sale.createdAt, columns);
    if (!sale.userName.isEmpty())
        out << tr("Cajero: %1").arg(sale.userName).left(columns);
    if (sale.status == QLatin1String("voided"))
        out << center(tr("*** ANULADA ***"), columns);
    out << rule;

    // Lineas
    for (const SaleItemDetail &it : sale.items) {
        out << wrap(it.name, columns);
        const QString unit = it.unit == QLatin1String("u") ? QString() : QLatin1Char(' ') + it.unit;
        out << justify(QStringLiteral("  %1%2 x %3").arg(Money::formatMilli(it.qtyMilli), unit,
                                                        Money::formatCents(it.priceCents)),
                       Money::formatCents(it.lineTotalCents), columns);
    }
    out << rule;

    // Totales
    if (sale.discount > 0) {
        out << justify(tr("Subtotal:"), Money::formatCents(sale.subtotal), columns);
        out << justify(tr("Descuento:"), QStringLiteral("-") + Money::formatCents(sale.discount), columns);
    }
    out << justify(tr("TOTAL %1:").arg(info.baseCurrency), Money::formatCents(sale.total), columns);
    out << rule;

    // Pagos
    for (const PaymentDetail &p : sale.payments) {
        if (p.method == QLatin1String("transfer")) {
            out << justify(tr("Transferencia:"), Money::formatCents(p.amountCents), columns);
            if (!p.reference.isEmpty())
                out << tr("  Ref: %1").arg(p.reference).left(columns);
        } else if (p.currency == info.baseCurrency) {
            out << justify(tr("Efectivo:"), Money::formatCents(p.amountCents), columns);
        } else {
            out << justify(tr("Efectivo %1:").arg(p.currency), Money::formatCents(p.amountCents), columns);
            out << justify(tr("  equivale a"), Money::formatCents(p.baseCents), columns);
        }
    }
    if (sale.change > 0)
        out << justify(tr("Vuelto:"), Money::formatCents(sale.change), columns);
    out << rule;

    const QString footer = info.footer.trimmed().isEmpty() ? tr("¡Gracias por su compra!") : info.footer;
    for (const QString &l : footer.split(QLatin1Char('\n'))) {
        for (const QString &w : wrap(l, columns))
            out << center(w, columns);
    }
    return out;
}
