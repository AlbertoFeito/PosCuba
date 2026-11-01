#include "InventoryController.h"

#include "core/Money.h"

namespace {
QVariantMap result(bool ok, const QString &error)
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("error"), error}};
}
} // namespace

InventoryController::InventoryController(QSqlDatabase db, const Session *session, QObject *parent)
    : QObject(parent)
    , m_service(db)
    , m_session(session)
{
}

void InventoryController::setSearchText(const QString &text)
{
    if (m_filter.text == text)
        return;
    m_filter.text = text;
    emit filterChanged();
    reloadProducts();
}

void InventoryController::setLowStockOnly(bool on)
{
    if (m_filter.lowStockOnly == on)
        return;
    m_filter.lowStockOnly = on;
    emit filterChanged();
    reloadProducts();
}

void InventoryController::setShowAllMovements(bool on)
{
    if (m_showAll == on)
        return;
    m_showAll = on;
    emit filterChanged();
    reloadMovements();
}

void InventoryController::setSelectedProductId(int id)
{
    if (m_selectedId == id)
        return;
    m_selectedId = id;
    reloadSelection();
    if (!m_showAll)
        reloadMovements();
}

void InventoryController::reload()
{
    reloadProducts();
    reloadSelection();
    reloadMovements();
    reloadLowStockCount();
}

void InventoryController::reloadProducts()
{
    m_products.setProducts(m_service.products().find(m_filter));
}

void InventoryController::reloadSelection()
{
    Product p;
    if (m_selectedId > 0 && m_service.products().get(m_selectedId, &p)) {
        m_selected = {
            {QStringLiteral("id"), p.id},
            {QStringLiteral("code"), p.code},
            {QStringLiteral("name"), p.name},
            {QStringLiteral("categoryName"), p.categoryName},
            {QStringLiteral("unit"), p.unit},
            {QStringLiteral("stockText"), Money::formatMilli(qAbs(p.stockMilli))},
            {QStringLiteral("stockNegative"), p.stockMilli < 0},
            {QStringLiteral("stockMilli"), static_cast<double>(p.stockMilli)},
            {QStringLiteral("minStockText"), Money::formatMilli(p.minStockMilli)},
            {QStringLiteral("costText"), Money::formatCents(p.costCents)},
            {QStringLiteral("lowStock"), p.isLowStock()},
            {QStringLiteral("active"), p.active},
        };
    } else {
        m_selected.clear();
    }
    emit selectionChanged();
}

void InventoryController::reloadMovements()
{
    const int productId = m_showAll ? 0 : m_selectedId;
    if (!m_showAll && productId <= 0) {
        m_movements.setMovements({});
        return;
    }
    m_movements.setMovements(m_service.history(productId, m_showAll ? 500 : 300));
}

void InventoryController::reloadLowStockCount()
{
    const int n = m_service.products().countLowStock();
    if (n != m_lowStockCount) {
        m_lowStockCount = n;
        emit lowStockCountChanged();
    }
}

QVariantMap InventoryController::registerPurchase(const QVariantMap &form)
{
    if (!m_session || !m_session->isAdmin())
        return result(false, tr("Solo un administrador puede hacer esto."));
    const int productId = form.value(QStringLiteral("productId")).toInt();
    qint64 qty = 0;
    if (!Money::parseMilli(form.value(QStringLiteral("qty")).toString(), &qty) || qty <= 0)
        return result(false, tr("Cantidad no válida (mayor que cero, hasta 3 decimales)."));

    qint64 cost = -1;
    const QString costText = form.value(QStringLiteral("cost")).toString().trimmed();
    if (!costText.isEmpty() && !Money::parseCents(costText, &cost))
        return result(false, tr("Costo unitario no válido (ej.: 125.50)."));

    QString error;
    if (!m_service.registerPurchase(productId, qty, cost, form.value(QStringLiteral("updateCost")).toBool(),
                                    form.value(QStringLiteral("note")).toString(),
                                    m_session ? m_session->userId : 0, &error))
        return result(false, error);

    reload();
    emit stockChanged();
    return result(true, QString());
}

QVariantMap InventoryController::adjust(const QVariantMap &form)
{
    if (!m_session || !m_session->isAdmin())
        return result(false, tr("Solo un administrador puede hacer esto."));
    const int productId = form.value(QStringLiteral("productId")).toInt();
    qint64 counted = 0;
    if (!Money::parseMilli(form.value(QStringLiteral("counted")).toString(), &counted))
        return result(false, tr("Stock contado no válido (0 o más, hasta 3 decimales)."));

    QString error;
    if (!m_service.adjustToCounted(productId, counted, form.value(QStringLiteral("reason")).toString(),
                                   form.value(QStringLiteral("note")).toString(),
                                   m_session ? m_session->userId : 0, &error))
        return result(false, error);

    reload();
    emit stockChanged();
    return result(true, QString());
}
