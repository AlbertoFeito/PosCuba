#pragma once

#include <QSqlDatabase>
#include <QDate>
#include <QString>
#include <QVector>

#include "core/Sale.h"

// Escritura de ventas. Usar dentro de una transaccion (SaleService).
class SaleRepository
{
public:
    explicit SaleRepository(QSqlDatabase db);

    int nextNumber();
    int insertSale(int number, int sessionId, int userId, const SaleTotals &t); // id o -1
    bool insertItem(int saleId, const CartLine &line, qint64 lineTotal);
    bool insertPayment(int saleId, const Payment &p, qint64 baseCents);

    bool load(int saleId, SaleDetail *out);
    QVector<SaleRow> recent(int limit);
    int lastSaleId();
    QVector<int> idsForDay(const QDate &day);   // en orden de venta

    QString lastError() const { return m_lastError; }

private:
    QSqlDatabase m_db;
    QString m_lastError;
};
