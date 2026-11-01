#include "ProductController.h"

#include "core/Money.h"
#include "core/ProductValidator.h"

namespace {
QVariantMap result(bool ok, const QString &error, int id = 0)
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("error"), error}, {QStringLiteral("id"), id}};
}
} // namespace

ProductController::ProductController(QSqlDatabase db, const Session *session, QObject *parent)
    : QObject(parent)
    , m_service(db)
    , m_session(session)
    , m_categoryFilter(tr("Todas las categorías"))
    , m_categoryForm(tr("Sin categoría"))
{
}

QStringList ProductController::units() const
{
    return ProductValidator::units();
}

void ProductController::setSearchText(const QString &text)
{
    if (m_filter.text == text)
        return;
    m_filter.text = text;
    emit filterChanged();
    reload();
}

void ProductController::setCategoryFilter(int id)
{
    if (m_filter.categoryId == id)
        return;
    m_filter.categoryId = id;
    emit filterChanged();
    reload();
}

void ProductController::setShowInactive(bool show)
{
    if (m_filter.includeInactive == show)
        return;
    m_filter.includeInactive = show;
    emit filterChanged();
    reload();
}

void ProductController::reload()
{
    m_products.setProducts(m_service.products().find(m_filter));
}

void ProductController::reloadCategories()
{
    const QVector<Category> cats = m_service.categories().all();
    m_categoryFilter.setCategories(cats);
    m_categoryForm.setCategories(cats);
}

QVariantMap ProductController::get(int id)
{
    Product p;
    if (!m_service.products().get(id, &p))
        return QVariantMap();
    return {
        {QStringLiteral("id"), p.id},
        {QStringLiteral("code"), p.code},
        {QStringLiteral("barcode"), p.barcode},
        {QStringLiteral("name"), p.name},
        {QStringLiteral("categoryId"), p.categoryId},
        {QStringLiteral("cost"), Money::formatCents(p.costCents)},
        {QStringLiteral("price"), Money::formatCents(p.priceCents)},
        {QStringLiteral("unit"), p.unit},
        {QStringLiteral("minStock"), Money::formatMilli(p.minStockMilli)},
        {QStringLiteral("stock"), Money::formatMilli(p.stockMilli)},
        {QStringLiteral("active"), p.active},
    };
}

QVariantMap ProductController::save(const QVariantMap &form)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede hacer esto."));
    Product p;
    p.id = form.value(QStringLiteral("id")).toInt();
    p.code = form.value(QStringLiteral("code")).toString();
    p.barcode = form.value(QStringLiteral("barcode")).toString();
    p.name = form.value(QStringLiteral("name")).toString();
    p.categoryId = form.value(QStringLiteral("categoryId")).toInt();
    p.unit = form.value(QStringLiteral("unit")).toString();

    const QString cost = form.value(QStringLiteral("cost")).toString().trimmed();
    const QString price = form.value(QStringLiteral("price")).toString().trimmed();
    const QString minStock = form.value(QStringLiteral("minStock")).toString().trimmed();

    if (!cost.isEmpty() && !Money::parseCents(cost, &p.costCents))
        return result(false, tr("Precio de costo no válido (ej.: 125.50)."));
    if (!Money::parseCents(price, &p.priceCents))
        return result(false, tr("Precio de venta no válido (ej.: 150 o 150.00)."));
    if (!minStock.isEmpty() && !Money::parseMilli(minStock, &p.minStockMilli))
        return result(false, tr("Stock mínimo no válido (hasta 3 decimales)."));

    QString error;
    if (!m_service.save(p, &error))
        return result(false, error);

    reload();
    emit productsChanged();
    return result(true, QString(), p.id);
}

QVariantMap ProductController::setActive(int id, bool active)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede hacer esto."), id);
    QString error;
    if (!m_service.setActive(id, active, &error))
        return result(false, error, id);
    reload();
    emit productsChanged();
    return result(true, QString(), id);
}

QVariantMap ProductController::addCategory(const QString &name)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede hacer esto."));
    QString error;
    const int id = m_service.ensureCategory(name, &error);
    if (id <= 0)
        return result(false, error);
    reloadCategories();
    return result(true, QString(), id);
}

QString ProductController::nextCode()
{
    return m_service.products().nextNumericCode();
}
