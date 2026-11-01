#include "ProductValidator.h"

#include <QCoreApplication>

namespace {
QString tr(const char *s) { return QCoreApplication::translate("ProductValidator", s); }

bool hasSpace(const QString &s)
{
    for (const QChar c : s) {
        if (c.isSpace())
            return true;
    }
    return false;
}
} // namespace

QStringList ProductValidator::units()
{
    return {QStringLiteral("u"), QStringLiteral("kg"), QStringLiteral("lb"),
            QStringLiteral("l"), QStringLiteral("m")};
}

QString ProductValidator::validate(Product &p)
{
    p.code = p.code.trimmed();
    p.barcode = p.barcode.trimmed();
    p.name = p.name.simplified();

    if (p.code.isEmpty())
        return tr("El código es obligatorio.");
    if (p.code.size() > 32 || hasSpace(p.code))
        return tr("El código no puede tener espacios ni más de 32 caracteres.");
    if (p.barcode.size() > 64 || hasSpace(p.barcode))
        return tr("El código de barras no puede tener espacios ni más de 64 caracteres.");
    if (p.name.isEmpty())
        return tr("El nombre es obligatorio.");
    if (p.name.size() > 120)
        return tr("El nombre no puede tener más de 120 caracteres.");
    if (!units().contains(p.unit))
        return tr("Unidad no válida.");
    if (p.costCents < 0 || p.priceCents < 0)
        return tr("Los precios no pueden ser negativos.");
    if (p.minStockMilli < 0)
        return tr("El stock mínimo no puede ser negativo.");
    return QString();
}
