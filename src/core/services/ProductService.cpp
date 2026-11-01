#include "ProductService.h"

#include <QCoreApplication>

#include "core/ProductValidator.h"

namespace {
QString tr(const char *s) { return QCoreApplication::translate("ProductService", s); }
void setErr(QString *error, const QString &msg) { if (error) *error = msg; }
} // namespace

ProductService::ProductService(QSqlDatabase db)
    : m_products(db)
    , m_categories(db)
{
}

bool ProductService::save(Product &p, QString *error)
{
    const QString invalid = ProductValidator::validate(p);
    if (!invalid.isEmpty()) {
        setErr(error, invalid);
        return false;
    }
    if (m_products.codeExists(p.code, p.id)) {
        setErr(error, tr("Ya existe un producto con el código %1.").arg(p.code));
        return false;
    }
    if (m_products.barcodeExists(p.barcode, p.id)) {
        setErr(error, tr("Ya existe un producto con el código de barras %1.").arg(p.barcode));
        return false;
    }

    if (p.id == 0) {
        const int id = m_products.insert(p);
        if (id <= 0) {
            setErr(error, tr("No se pudo guardar el producto: %1").arg(m_products.lastError()));
            return false;
        }
        p.id = id;
    } else if (!m_products.update(p)) {
        setErr(error, tr("No se pudo actualizar el producto: %1").arg(m_products.lastError()));
        return false;
    }
    return true;
}

bool ProductService::setActive(int id, bool active, QString *error)
{
    if (!m_products.setActive(id, active)) {
        setErr(error, tr("No se pudo cambiar el estado del producto."));
        return false;
    }
    return true;
}

int ProductService::ensureCategory(const QString &rawName, QString *error)
{
    const QString name = rawName.simplified();
    if (name.isEmpty()) {
        setErr(error, tr("El nombre de la categoría es obligatorio."));
        return -1;
    }
    if (name.size() > 60) {
        setErr(error, tr("El nombre de la categoría no puede tener más de 60 caracteres."));
        return -1;
    }
    const int existing = m_categories.findByName(name);
    if (existing > 0)
        return existing;
    const int id = m_categories.insert(name);
    if (id <= 0)
        setErr(error, tr("No se pudo crear la categoría: %1").arg(m_categories.lastError()));
    return id;
}
