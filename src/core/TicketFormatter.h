#pragma once

#include <QString>
#include <QStringList>

#include "core/Sale.h"

struct TicketInfo
{
    QString businessName;
    QString address;
    QString phone;
    QString footer;
    QString baseCurrency = QStringLiteral("CUP");
};

// Ticket como lineas de texto de ancho fijo (fuente monoespaciada).
// 58 mm = 32 columnas, 80 mm = 48 columnas.
namespace TicketFormatter {

int columnsForPaper(int paperWidthMm);

QStringList format(const SaleDetail &sale, const TicketInfo &info, int columns);

// Utilidades (publicas para tests)
QString center(const QString &text, int columns);
QString justify(const QString &left, const QString &right, int columns);
QStringList wrap(const QString &text, int columns, int indent = 0);

} // namespace TicketFormatter
