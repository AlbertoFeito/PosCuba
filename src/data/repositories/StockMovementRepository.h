#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "core/StockMovement.h"

class StockMovementRepository
{
public:
    explicit StockMovementRepository(QSqlDatabase db);

    int insert(const StockMovement &m);  // id, o -1 si falla
    // productId 0 = todos los productos. Mas recientes primero.
    QVector<StockMovement> list(int productId, int limit);
    QString lastError() const { return m_lastError; }

private:
    QSqlDatabase m_db;
    QString m_lastError;
};
