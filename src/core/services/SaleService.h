#pragma once

#include <QSqlDatabase>
#include <QVector>

#include "core/Sale.h"
#include "core/services/InventoryService.h"
#include "data/repositories/CashRepository.h"
#include "data/repositories/CurrencyRepository.h"
#include "data/repositories/SaleRepository.h"

class SaleService
{
public:
    explicit SaleService(QSqlDatabase db);

    // Registra la venta completa en una transaccion: venta, lineas, pagos y descuento de stock.
    // Las tasas de cambio se toman de la tabla de monedas (no del llamador).
    // Si algun producto quedaria con stock negativo y allowNegativeStock es false,
    // no guarda nada y devuelve result.stockIssues para pedir confirmacion.
    CheckoutResult checkout(const QVector<CartLine> &lines, qint64 discountCents, QVector<Payment> payments,
                            int userId, bool allowNegativeStock);

private:
    QSqlDatabase m_db;
    SaleRepository m_sales;
    CashRepository m_cash;
    CurrencyRepository m_currencies;
    InventoryService m_inventory;
};
