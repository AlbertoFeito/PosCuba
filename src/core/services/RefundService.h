#pragma once

#include <QSqlDatabase>
#include <QString>

#include "core/Sale.h"
#include "core/services/CashService.h"
#include "core/services/InventoryService.h"
#include "data/repositories/CashRepository.h"
#include "data/repositories/RefundRepository.h"

// Devoluciones parciales y anulaciones de ventas.
// - El importe de cada linea es proporcional a lo devuelto e incluye su parte del descuento general.
//   Si la devolucion deja la venta sin nada, se devuelve exactamente lo que falta del total.
// - El dinero sale de la caja abierta (efectivo en moneda base) o se devuelve por transferencia.
// - Lo devuelto puede volver al inventario (restock) o no (producto danado).
// - Si ya no queda nada vendido, la venta pasa a 'voided' (anulada).
class RefundService
{
public:
    explicit RefundService(QSqlDatabase db);

    bool state(int saleId, SaleRefundState *out) { return m_refunds.loadState(saleId, out); }

    // Calculo puro (sin base de datos). all = todo lo que queda. Devuelve false + error si no es valido.
    static bool calculate(const SaleRefundState &state, const QMap<int, qint64> &qtyByItem, bool all,
                          RefundCalc *out, QString *error);

    // Registra la devolucion en una transaccion.
    RefundResult refund(const RefundRequest &request);

private:
    QSqlDatabase m_db;
    RefundRepository m_refunds;
    CashRepository m_cashRepo;
    CashService m_cash;
    InventoryService m_inventory;
};
