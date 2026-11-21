#include "CustomerRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
const char *const kBalance =
    "COALESCE((SELECT SUM(CASE WHEN e.kind = 'charge' THEN e.amount_cents ELSE -e.amount_cents END) "
    "FROM customer_entries e WHERE e.customer_id = c.id), 0)";

Customer readRow(const QSqlQuery &q)
{
    Customer c;
    c.id = q.value(0).toInt();
    c.name = q.value(1).toString();
    c.phone = q.value(2).toString();
    c.note = q.value(3).toString();
    c.creditLimitCents = q.value(4).toLongLong();
    c.active = q.value(5).toInt() != 0;
    c.balanceCents = q.value(6).toLongLong();
    c.lastActivity = q.value(7).toString();
    return c;
}

QString selectSql()
{
    return QStringLiteral("SELECT c.id, c.name, COALESCE(c.phone, ''), COALESCE(c.note, ''), c.credit_limit_cents, "
                          "c.active, %1, "
                          "(SELECT MAX(e.created_at) FROM customer_entries e WHERE e.customer_id = c.id) "
                          "FROM customers c ")
        .arg(QString::fromLatin1(kBalance));
}
} // namespace

CustomerRepository::CustomerRepository(QSqlDatabase db)
    : m_db(db)
{
}

QVector<Customer> CustomerRepository::list(const QString &search, bool includeInactive, bool onlyWithDebt)
{
    QVector<Customer> out;
    QStringList where;
    if (!includeInactive)
        where << QStringLiteral("c.active = 1");
    if (!search.trimmed().isEmpty())
        where << QStringLiteral("(c.name LIKE :s1 OR c.phone LIKE :s2)");
    if (onlyWithDebt)
        where << QStringLiteral("%1 > 0").arg(QString::fromLatin1(kBalance));
    QString sql = selectSql();
    if (!where.isEmpty())
        sql += QStringLiteral("WHERE ") + where.join(QStringLiteral(" AND "));
    sql += QStringLiteral(" ORDER BY c.name COLLATE NOCASE");
    QSqlQuery q(m_db);
    q.prepare(sql);
    if (!search.trimmed().isEmpty())
    {
        const QString like = QStringLiteral("%") + search.trimmed() + QStringLiteral("%");
        q.bindValue(QStringLiteral(":s1"), like);
        q.bindValue(QStringLiteral(":s2"), like);
    }
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next())
        out.append(readRow(q));
    return out;
}

bool CustomerRepository::get(int id, Customer *out)
{
    QSqlQuery q(m_db);
    q.prepare(selectSql() + QStringLiteral("WHERE c.id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec() || !q.next())
        return false;
    if (out)
        *out = readRow(q);
    return true;
}

bool CustomerRepository::nameExists(const QString &name, int exceptId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT 1 FROM customers WHERE name = :n COLLATE NOCASE AND id <> :id"));
    q.bindValue(QStringLiteral(":n"), name);
    q.bindValue(QStringLiteral(":id"), exceptId);
    return q.exec() && q.next();
}

int CustomerRepository::insert(const Customer &c)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO customers (name, phone, note, credit_limit_cents, active) "
                             "VALUES (:n, :p, :no, :l, :a)"));
    q.bindValue(QStringLiteral(":n"), c.name);
    q.bindValue(QStringLiteral(":p"), c.phone.isEmpty() ? QVariant() : QVariant(c.phone));
    q.bindValue(QStringLiteral(":no"), c.note.isEmpty() ? QVariant() : QVariant(c.note));
    q.bindValue(QStringLiteral(":l"), c.creditLimitCents);
    q.bindValue(QStringLiteral(":a"), c.active ? 1 : 0);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool CustomerRepository::update(const Customer &c)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE customers SET name = :n, phone = :p, note = :no, credit_limit_cents = :l "
                             "WHERE id = :id"));
    q.bindValue(QStringLiteral(":n"), c.name);
    q.bindValue(QStringLiteral(":p"), c.phone.isEmpty() ? QVariant() : QVariant(c.phone));
    q.bindValue(QStringLiteral(":no"), c.note.isEmpty() ? QVariant() : QVariant(c.note));
    q.bindValue(QStringLiteral(":l"), c.creditLimitCents);
    q.bindValue(QStringLiteral(":id"), c.id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

bool CustomerRepository::setActive(int id, bool active)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE customers SET active = :a WHERE id = :id"));
    q.bindValue(QStringLiteral(":a"), active ? 1 : 0);
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

qint64 CustomerRepository::balance(int customerId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT COALESCE(SUM(CASE WHEN kind = 'charge' THEN amount_cents ELSE -amount_cents END), 0) "
                             "FROM customer_entries WHERE customer_id = :c"));
    q.bindValue(QStringLiteral(":c"), customerId);
    if (q.exec() && q.next())
        return q.value(0).toLongLong();
    return 0;
}

bool CustomerRepository::insertEntry(int customerId, const QString &kind, qint64 amountCents, int saleId, int refundId,
                                     int sessionId, const QString &method, const QString &note, int userId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO customer_entries (customer_id, kind, amount_cents, sale_id, refund_id, session_id, method, note, "
        "user_id) VALUES (:c, :k, :a, :s, :r, :ss, :m, :n, :u)"));
    q.bindValue(QStringLiteral(":c"), customerId);
    q.bindValue(QStringLiteral(":k"), kind);
    q.bindValue(QStringLiteral(":a"), amountCents);
    q.bindValue(QStringLiteral(":s"), saleId > 0 ? QVariant(saleId) : QVariant());
    q.bindValue(QStringLiteral(":r"), refundId > 0 ? QVariant(refundId) : QVariant());
    q.bindValue(QStringLiteral(":ss"), sessionId > 0 ? QVariant(sessionId) : QVariant());
    q.bindValue(QStringLiteral(":m"), method.isEmpty() ? QVariant() : QVariant(method));
    q.bindValue(QStringLiteral(":n"), note.trimmed().isEmpty() ? QVariant() : QVariant(note.trimmed()));
    q.bindValue(QStringLiteral(":u"), userId);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

QVector<CustomerEntry> CustomerRepository::entries(int customerId)
{
    QVector<CustomerEntry> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT e.id, e.kind, e.amount_cents, COALESCE(s.number, 0), COALESCE(r.number, 0), COALESCE(e.method, ''), "
        "COALESCE(e.note, ''), COALESCE(u.name, ''), e.created_at "
        "FROM customer_entries e LEFT JOIN sales s ON s.id = e.sale_id LEFT JOIN refunds r ON r.id = e.refund_id "
        "LEFT JOIN users u ON u.id = e.user_id WHERE e.customer_id = :c ORDER BY e.id"));
    q.bindValue(QStringLiteral(":c"), customerId);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    qint64 balance = 0;
    while (q.next()) {
        CustomerEntry e;
        e.id = q.value(0).toInt();
        e.kind = q.value(1).toString();
        e.amountCents = q.value(2).toLongLong();
        e.saleNumber = q.value(3).toInt();
        e.refundNumber = q.value(4).toInt();
        e.method = q.value(5).toString();
        e.note = q.value(6).toString();
        e.userName = q.value(7).toString();
        e.createdAt = q.value(8).toString();
        balance += e.kind == CustomerEntryKind::charge() ? e.amountCents : -e.amountCents;
        e.balanceAfter = balance;
        out.prepend(e);
    }
    return out;
}

qint64 CustomerRepository::receivables(int *customersWithDebt)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT COALESCE(SUM(b), 0), COUNT(*) FROM (SELECT %1 AS b FROM customers c) WHERE b > 0")
                  .arg(QString::fromLatin1(kBalance)));
    if (q.exec() && q.next()) {
        if (customersWithDebt)
            *customersWithDebt = q.value(1).toInt();
        return q.value(0).toLongLong();
    }
    if (customersWithDebt)
        *customersWithDebt = 0;
    return 0;
}
