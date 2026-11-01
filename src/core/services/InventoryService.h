#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QVector>

#include "core/StockMovement.h"
#include "data/repositories/ProductRepository.h"
#include "data/repositories/StockMovementRepository.h"

class InventoryService
{
public:
    explicit InventoryService(QSqlDatabase db);

    // Aplica un cambio de stock y registra el movimiento con el saldo resultante.
    // NO abre transaccion: el llamador (p. ej. una venta) debe tenerla abierta.
    // Si allowNegative es false y el stock quedaria negativo, falla sin cambiar nada.
    bool applyChange(const StockChange &change, bool allowNegative, qint64 *newStock, QString *error);

    // Entrada de mercancia. unitCostCents -1 = sin costo. Transaccion propia.
    bool registerPurchase(int productId, qint64 qtyMilli, qint64 unitCostCents, bool updateProductCost,
                          const QString &note, int userId, QString *error);

    // Ajuste a un stock contado (>= 0). La diferencia queda como movimiento. Transaccion propia.
    bool adjustToCounted(int productId, qint64 countedMilli, const QString &reason, const QString &note,
                         int userId, QString *error);

    QVector<StockMovement> history(int productId, int limit = 300);

    static QStringList adjustReasons();

    ProductRepository &products() { return m_products; }

private:
    QSqlDatabase m_db;
    ProductRepository m_products;
    StockMovementRepository m_movements;
};
