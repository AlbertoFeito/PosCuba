#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "core/Sale.h"

// Devoluciones. Las escrituras van dentro de una transaccion (RefundService).
class RefundRepository
{
public:
    explicit RefundRepository(QSqlDatabase db);

    // Venta con lo vendido y lo ya devuelto por linea.
    bool loadState(int saleId, SaleRefundState *out);
    int nextNumber();
    int insertRefund(int number, const RefundRequest &req, int sessionId, qint64 total, qint64 cost); // id o -1
    bool insertItem(int refundId, int saleItemId, int productId, qint64 qtyMilli, qint64 amountCents,
                    qint64 costCents);
    bool setSaleStatus(int saleId, const QString &status);

    QVector<RefundInfo> forSale(int saleId);
    RefundTotals sessionTotals(int sessionId);

    QString lastError() const { return m_lastError; }

private:
    QSqlDatabase m_db;
    QString m_lastError;
};
