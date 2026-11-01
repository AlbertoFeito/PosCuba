#include "StockMovementRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

StockMovementRepository::StockMovementRepository(QSqlDatabase db)
    : m_db(db)
{
}

int StockMovementRepository::insert(const StockMovement &m)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO stock_movements (product_id, type, qty_milli, cost_cents, sale_id, user_id, "
        "note, stock_after_milli) VALUES (:p, :t, :q, :c, :s, :u, :n, :after)"));
    q.bindValue(QStringLiteral(":p"), m.productId);
    q.bindValue(QStringLiteral(":t"), m.type);
    q.bindValue(QStringLiteral(":q"), m.qtyMilli);
    q.bindValue(QStringLiteral(":c"), m.costCents >= 0 ? QVariant(m.costCents) : QVariant());
    q.bindValue(QStringLiteral(":s"), m.saleId > 0 ? QVariant(m.saleId) : QVariant());
    q.bindValue(QStringLiteral(":u"), m.userId);
    q.bindValue(QStringLiteral(":n"), m.note.isEmpty() ? QVariant() : QVariant(m.note));
    q.bindValue(QStringLiteral(":after"), m.stockAfterMilli);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

QVector<StockMovement> StockMovementRepository::list(int productId, int limit)
{
    QVector<StockMovement> out;
    QString sql = QStringLiteral(
        "SELECT m.id, m.product_id, p.name, p.unit, m.type, m.qty_milli, m.cost_cents, "
        "m.stock_after_milli, m.sale_id, m.user_id, u.name, m.note, m.created_at "
        "FROM stock_movements m "
        "JOIN products p ON p.id = m.product_id "
        "LEFT JOIN users u ON u.id = m.user_id ");
    if (productId > 0)
        sql += QStringLiteral("WHERE m.product_id = :p ");
    sql += QStringLiteral("ORDER BY m.id DESC LIMIT :limit");

    QSqlQuery q(m_db);
    q.prepare(sql);
    if (productId > 0)
        q.bindValue(QStringLiteral(":p"), productId);
    q.bindValue(QStringLiteral(":limit"), limit);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        StockMovement m;
        m.id = q.value(0).toInt();
        m.productId = q.value(1).toInt();
        m.productName = q.value(2).toString();
        m.unit = q.value(3).toString();
        m.type = q.value(4).toString();
        m.qtyMilli = q.value(5).toLongLong();
        m.costCents = q.value(6).isNull() ? -1 : q.value(6).toLongLong();
        m.stockAfterMilli = q.value(7).toLongLong();
        m.saleId = q.value(8).toInt();
        m.userId = q.value(9).toInt();
        m.userName = q.value(10).toString();
        m.note = q.value(11).toString();
        m.createdAt = q.value(12).toString();
        out.append(m);
    }
    return out;
}
