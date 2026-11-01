#include "SaleRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

SaleRepository::SaleRepository(QSqlDatabase db)
    : m_db(db)
{
}

int SaleRepository::nextNumber()
{
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT COALESCE(MAX(number), 0) + 1 FROM sales")) && q.next())
        return q.value(0).toInt();
    return 1;
}

int SaleRepository::insertSale(int number, int sessionId, int userId, const SaleTotals &t)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO sales (number, session_id, user_id, subtotal_cents, discount_cents, total_cents, "
        "change_cents) VALUES (:n, :s, :u, :sub, :d, :t, :c)"));
    q.bindValue(QStringLiteral(":n"), number);
    q.bindValue(QStringLiteral(":s"), sessionId);
    q.bindValue(QStringLiteral(":u"), userId);
    q.bindValue(QStringLiteral(":sub"), t.subtotal);
    q.bindValue(QStringLiteral(":d"), t.discount);
    q.bindValue(QStringLiteral(":t"), t.total);
    q.bindValue(QStringLiteral(":c"), t.change);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool SaleRepository::insertItem(int saleId, const CartLine &l, qint64 lineTotal)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO sale_items (sale_id, product_id, product_name, qty_milli, price_cents, cost_cents, "
        "discount_cents, line_total_cents) VALUES (:s, :p, :name, :q, :price, :cost, 0, :total)"));
    q.bindValue(QStringLiteral(":s"), saleId);
    q.bindValue(QStringLiteral(":p"), l.productId);
    q.bindValue(QStringLiteral(":name"), l.name);
    q.bindValue(QStringLiteral(":q"), l.qtyMilli);
    q.bindValue(QStringLiteral(":price"), l.priceCents);
    q.bindValue(QStringLiteral(":cost"), l.costCents);
    q.bindValue(QStringLiteral(":total"), lineTotal);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool SaleRepository::insertPayment(int saleId, const Payment &p, qint64 baseCents)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO payments (sale_id, method, currency_code, amount_cents, rate_milli, base_cents, reference) "
        "VALUES (:s, :m, :c, :a, :r, :b, :ref)"));
    q.bindValue(QStringLiteral(":s"), saleId);
    q.bindValue(QStringLiteral(":m"), p.method);
    q.bindValue(QStringLiteral(":c"), p.currency);
    q.bindValue(QStringLiteral(":a"), p.amountCents);
    q.bindValue(QStringLiteral(":r"), p.rateMilli);
    q.bindValue(QStringLiteral(":b"), baseCents);
    q.bindValue(QStringLiteral(":ref"), p.reference.trimmed().isEmpty() ? QVariant() : QVariant(p.reference.trimmed()));
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool SaleRepository::load(int saleId, SaleDetail *out)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT s.id, s.number, s.created_at, u.name, s.status, s.subtotal_cents, s.discount_cents, "
        "s.total_cents, s.change_cents FROM sales s LEFT JOIN users u ON u.id = s.user_id WHERE s.id = :id"));
    q.bindValue(QStringLiteral(":id"), saleId);
    if (!q.exec() || !q.next()) {
        m_lastError = q.lastError().text();
        return false;
    }
    SaleDetail d;
    d.id = q.value(0).toInt();
    d.number = q.value(1).toInt();
    d.createdAt = q.value(2).toString();
    d.userName = q.value(3).toString();
    d.status = q.value(4).toString();
    d.subtotal = q.value(5).toLongLong();
    d.discount = q.value(6).toLongLong();
    d.total = q.value(7).toLongLong();
    d.change = q.value(8).toLongLong();

    q.prepare(QStringLiteral(
        "SELECT i.product_name, p.unit, i.qty_milli, i.price_cents, i.line_total_cents "
        "FROM sale_items i LEFT JOIN products p ON p.id = i.product_id WHERE i.sale_id = :id ORDER BY i.id"));
    q.bindValue(QStringLiteral(":id"), saleId);
    if (q.exec()) {
        while (q.next()) {
            SaleItemDetail it;
            it.name = q.value(0).toString();
            it.unit = q.value(1).toString();
            it.qtyMilli = q.value(2).toLongLong();
            it.priceCents = q.value(3).toLongLong();
            it.lineTotalCents = q.value(4).toLongLong();
            d.items.append(it);
        }
    }

    q.prepare(QStringLiteral("SELECT method, currency_code, amount_cents, base_cents, reference "
                             "FROM payments WHERE sale_id = :id ORDER BY id"));
    q.bindValue(QStringLiteral(":id"), saleId);
    if (q.exec()) {
        while (q.next()) {
            PaymentDetail p;
            p.method = q.value(0).toString();
            p.currency = q.value(1).toString();
            p.amountCents = q.value(2).toLongLong();
            p.baseCents = q.value(3).toLongLong();
            p.reference = q.value(4).toString();
            d.payments.append(p);
        }
    }

    q.prepare(QStringLiteral("SELECT r.number, r.created_at, u.name, r.method, r.reason, r.total_cents "
                             "FROM refunds r LEFT JOIN users u ON u.id = r.user_id WHERE r.sale_id = :id ORDER BY r.id"));
    q.bindValue(QStringLiteral(":id"), saleId);
    if (q.exec()) {
        while (q.next()) {
            RefundInfo r;
            r.number = q.value(0).toInt();
            r.createdAt = q.value(1).toString();
            r.userName = q.value(2).toString();
            r.method = q.value(3).toString();
            r.reason = q.value(4).toString();
            r.total = q.value(5).toLongLong();
            d.refunds.append(r);
        }
    }
    if (out)
        *out = d;
    return true;
}

QVector<SaleRow> SaleRepository::recent(int limit)
{
    QVector<SaleRow> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT s.id, s.number, s.created_at, u.name, s.total_cents, "
        "(SELECT COUNT(*) FROM sale_items i WHERE i.sale_id = s.id), s.status, "
        "(SELECT COALESCE(SUM(total_cents), 0) FROM refunds r WHERE r.sale_id = s.id) "
        "FROM sales s LEFT JOIN users u ON u.id = s.user_id ORDER BY s.id DESC LIMIT :l"));
    q.bindValue(QStringLiteral(":l"), limit);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        SaleRow r;
        r.id = q.value(0).toInt();
        r.number = q.value(1).toInt();
        r.createdAt = q.value(2).toString();
        r.userName = q.value(3).toString();
        r.total = q.value(4).toLongLong();
        r.itemCount = q.value(5).toInt();
        r.status = q.value(6).toString();
        r.refunded = q.value(7).toLongLong();
        out.append(r);
    }
    return out;
}

int SaleRepository::lastSaleId()
{
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT COALESCE(MAX(id), 0) FROM sales")) && q.next())
        return q.value(0).toInt();
    return 0;
}

QVector<int> SaleRepository::idsForDay(const QDate &day)
{
    QVector<int> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id FROM sales WHERE created_at >= :a AND created_at < :b ORDER BY id"));
    q.bindValue(QStringLiteral(":a"), day.toString(QStringLiteral("yyyy-MM-dd")) + QStringLiteral(" 00:00:00"));
    q.bindValue(QStringLiteral(":b"), day.addDays(1).toString(QStringLiteral("yyyy-MM-dd")) + QStringLiteral(" 00:00:00"));
    if (q.exec()) {
        while (q.next())
            out.append(q.value(0).toInt());
    }
    return out;
}
