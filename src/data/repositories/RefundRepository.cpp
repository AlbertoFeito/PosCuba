#include "RefundRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

RefundRepository::RefundRepository(QSqlDatabase db)
    : m_db(db)
{
}

bool RefundRepository::loadState(int saleId, SaleRefundState *out)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT s.id, s.number, s.created_at, s.status, s.subtotal_cents, s.total_cents, "
        "(SELECT COALESCE(SUM(total_cents), 0) FROM refunds WHERE sale_id = s.id) "
        "FROM sales s WHERE s.id = :id"));
    q.bindValue(QStringLiteral(":id"), saleId);
    if (!q.exec() || !q.next()) {
        m_lastError = q.lastError().text();
        return false;
    }
    SaleRefundState st;
    st.saleId = q.value(0).toInt();
    st.number = q.value(1).toInt();
    st.createdAt = q.value(2).toString();
    st.status = q.value(3).toString();
    st.subtotal = q.value(4).toLongLong();
    st.total = q.value(5).toLongLong();
    st.refunded = q.value(6).toLongLong();

    q.prepare(QStringLiteral(
        "SELECT i.id, i.product_id, i.product_name, COALESCE(p.unit, 'u'), i.qty_milli, "
        "(SELECT COALESCE(SUM(r.qty_milli), 0) FROM refund_items r WHERE r.sale_item_id = i.id), "
        "i.price_cents, i.line_total_cents, i.cost_cents "
        "FROM sale_items i LEFT JOIN products p ON p.id = i.product_id WHERE i.sale_id = :id ORDER BY i.id"));
    q.bindValue(QStringLiteral(":id"), saleId);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    while (q.next()) {
        RefundableItem it;
        it.saleItemId = q.value(0).toInt();
        it.productId = q.value(1).toInt();
        it.name = q.value(2).toString();
        it.unit = q.value(3).toString();
        it.soldMilli = q.value(4).toLongLong();
        it.returnedMilli = q.value(5).toLongLong();
        it.priceCents = q.value(6).toLongLong();
        it.lineTotalCents = q.value(7).toLongLong();
        it.costCents = q.value(8).toLongLong();
        st.items.append(it);
    }
    if (out)
        *out = st;
    return true;
}

int RefundRepository::nextNumber()
{
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT COALESCE(MAX(number), 0) + 1 FROM refunds")) && q.next())
        return q.value(0).toInt();
    return 1;
}

int RefundRepository::insertRefund(int number, const RefundRequest &req, int sessionId, qint64 total, qint64 cost)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO refunds (number, sale_id, session_id, user_id, authorized_by, method, reason, restock, "
        "total_cents, cost_cents) VALUES (:n, :s, :ss, :u, :a, :m, :r, :rs, :t, :c)"));
    q.bindValue(QStringLiteral(":n"), number);
    q.bindValue(QStringLiteral(":s"), req.saleId);
    q.bindValue(QStringLiteral(":ss"), sessionId);
    q.bindValue(QStringLiteral(":u"), req.userId);
    q.bindValue(QStringLiteral(":a"), req.authorizedBy);
    q.bindValue(QStringLiteral(":m"), req.method);
    q.bindValue(QStringLiteral(":r"), req.reason.trimmed());
    q.bindValue(QStringLiteral(":rs"), req.restock ? 1 : 0);
    q.bindValue(QStringLiteral(":t"), total);
    q.bindValue(QStringLiteral(":c"), cost);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool RefundRepository::insertItem(int refundId, int saleItemId, int productId, qint64 qtyMilli, qint64 amountCents,
                                  qint64 costCents)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO refund_items (refund_id, sale_item_id, product_id, qty_milli, amount_cents, cost_cents) "
        "VALUES (:r, :i, :p, :q, :a, :c)"));
    q.bindValue(QStringLiteral(":r"), refundId);
    q.bindValue(QStringLiteral(":i"), saleItemId);
    q.bindValue(QStringLiteral(":p"), productId);
    q.bindValue(QStringLiteral(":q"), qtyMilli);
    q.bindValue(QStringLiteral(":a"), amountCents);
    q.bindValue(QStringLiteral(":c"), costCents);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool RefundRepository::setSaleStatus(int saleId, const QString &status)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE sales SET status = :st WHERE id = :id"));
    q.bindValue(QStringLiteral(":st"), status);
    q.bindValue(QStringLiteral(":id"), saleId);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

QVector<RefundInfo> RefundRepository::forSale(int saleId)
{
    QVector<RefundInfo> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT r.number, r.created_at, u.name, r.method, r.reason, r.total_cents "
        "FROM refunds r LEFT JOIN users u ON u.id = r.user_id WHERE r.sale_id = :s ORDER BY r.id"));
    q.bindValue(QStringLiteral(":s"), saleId);
    if (q.exec()) {
        while (q.next()) {
            RefundInfo r;
            r.number = q.value(0).toInt();
            r.createdAt = q.value(1).toString();
            r.userName = q.value(2).toString();
            r.method = q.value(3).toString();
            r.reason = q.value(4).toString();
            r.total = q.value(5).toLongLong();
            out.append(r);
        }
    }
    return out;
}

RefundTotals RefundRepository::sessionTotals(int sessionId)
{
    RefundTotals t;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT COUNT(*), COALESCE(SUM(total_cents), 0), COALESCE(SUM(cost_cents), 0), "
        "COALESCE(SUM(CASE WHEN method = 'cash' THEN total_cents END), 0), "
        "COALESCE(SUM(CASE WHEN method = 'transfer' THEN total_cents END), 0) "
        "FROM refunds WHERE session_id = :s"));
    q.bindValue(QStringLiteral(":s"), sessionId);
    if (q.exec() && q.next()) {
        t.count = q.value(0).toInt();
        t.total = q.value(1).toLongLong();
        t.cost = q.value(2).toLongLong();
        t.cash = q.value(3).toLongLong();
        t.transfer = q.value(4).toLongLong();
    }
    return t;
}
