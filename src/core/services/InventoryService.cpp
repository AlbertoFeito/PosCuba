#include "InventoryService.h"

#include <QCoreApplication>

#include "core/Money.h"
#include "data/Transaction.h"

namespace {
QString tr(const char *s) { return QCoreApplication::translate("InventoryService", s); }
void setErr(QString *error, const QString &msg) { if (error) *error = msg; }
} // namespace

InventoryService::InventoryService(QSqlDatabase db)
    : m_db(db)
    , m_products(db)
    , m_movements(db)
{
}

QStringList InventoryService::adjustReasons()
{
    return {tr("Conteo físico"), tr("Merma"), tr("Rotura o vencido"), tr("Consumo interno"),
            tr("Corrección"), tr("Otro")};
}

bool InventoryService::applyChange(const StockChange &c, bool allowNegative, qint64 *newStock, QString *error)
{
    if (c.deltaMilli == 0) {
        setErr(error, tr("La cantidad no puede ser cero."));
        return false;
    }
    if (c.userId <= 0) {
        setErr(error, tr("No hay un usuario activo."));
        return false;
    }

    qint64 current = 0;
    if (!m_products.stockOf(c.productId, &current)) {
        setErr(error, tr("El producto no existe."));
        return false;
    }
    const qint64 after = current + c.deltaMilli;
    if (after < 0 && !allowNegative) {
        setErr(error, tr("Stock insuficiente: hay %1 y se necesitan %2.")
                          .arg(Money::formatMilli(current), Money::formatMilli(-c.deltaMilli)));
        return false;
    }

    if (!m_products.setStock(c.productId, after)) {
        setErr(error, tr("No se pudo actualizar el stock: %1").arg(m_products.lastError()));
        return false;
    }

    StockMovement m;
    m.productId = c.productId;
    m.type = c.type;
    m.qtyMilli = c.deltaMilli;
    m.costCents = c.costCents;
    m.saleId = c.saleId;
    m.userId = c.userId;
    m.note = c.note.trimmed();
    m.stockAfterMilli = after;
    if (m_movements.insert(m) <= 0) {
        setErr(error, tr("No se pudo registrar el movimiento: %1").arg(m_movements.lastError()));
        return false;
    }
    if (newStock)
        *newStock = after;
    return true;
}

bool InventoryService::registerPurchase(int productId, qint64 qtyMilli, qint64 unitCostCents,
                                        bool updateProductCost, const QString &note, int userId,
                                        QString *error)
{
    if (qtyMilli <= 0) {
        setErr(error, tr("La cantidad de la entrada debe ser mayor que cero."));
        return false;
    }

    Transaction tx(m_db);
    if (!tx.isActive()) {
        setErr(error, tr("No se pudo iniciar la transacción."));
        return false;
    }

    StockChange c;
    c.productId = productId;
    c.deltaMilli = qtyMilli;
    c.type = MovementType::purchase();
    c.costCents = unitCostCents;
    c.userId = userId;
    c.note = note;
    if (!applyChange(c, false, nullptr, error))
        return false;

    if (updateProductCost && unitCostCents >= 0 && !m_products.setCost(productId, unitCostCents)) {
        setErr(error, tr("No se pudo actualizar el costo: %1").arg(m_products.lastError()));
        return false;
    }

    if (!tx.commit()) {
        setErr(error, tr("No se pudo confirmar la operación."));
        return false;
    }
    return true;
}

bool InventoryService::adjustToCounted(int productId, qint64 countedMilli, const QString &reason,
                                       const QString &note, int userId, QString *error)
{
    if (countedMilli < 0) {
        setErr(error, tr("El stock contado no puede ser negativo."));
        return false;
    }
    if (reason.trimmed().isEmpty()) {
        setErr(error, tr("Indica el motivo del ajuste."));
        return false;
    }

    Transaction tx(m_db);
    if (!tx.isActive()) {
        setErr(error, tr("No se pudo iniciar la transacción."));
        return false;
    }

    qint64 current = 0;
    if (!m_products.stockOf(productId, &current)) {
        setErr(error, tr("El producto no existe."));
        return false;
    }
    if (countedMilli == current) {
        setErr(error, tr("El stock contado es igual al actual; no hay nada que ajustar."));
        return false;
    }

    StockChange c;
    c.productId = productId;
    c.deltaMilli = countedMilli - current;
    c.type = MovementType::adjust();
    c.userId = userId;
    const QString n = note.trimmed();
    c.note = n.isEmpty() ? reason.trimmed() : reason.trimmed() + QStringLiteral(": ") + n;
    // Corregir un stock ya negativo hacia arriba siempre se permite; el contado nunca es negativo.
    if (!applyChange(c, true, nullptr, error))
        return false;

    if (!tx.commit()) {
        setErr(error, tr("No se pudo confirmar la operación."));
        return false;
    }
    return true;
}

QVector<StockMovement> InventoryService::history(int productId, int limit)
{
    return m_movements.list(productId, limit);
}
