#include "CashRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
const char *kSelect =
    "SELECT s.id, s.opened_by, u.name, s.opened_at, s.closed_at, s.opening_cents, s.expected_cents, "
    "s.counted_cents, s.difference_cents, s.status "
    "FROM cash_sessions s LEFT JOIN users u ON u.id = s.opened_by ";

CashSession readRow(const QSqlQuery &q)
{
    CashSession s;
    s.id = q.value(0).toInt();
    s.openedBy = q.value(1).toInt();
    s.openedByName = q.value(2).toString();
    s.openedAt = q.value(3).toString();
    s.closedAt = q.value(4).toString();
    s.openingCents = q.value(5).toLongLong();
    s.expectedCents = q.value(6).toLongLong();
    s.countedCents = q.value(7).toLongLong();
    s.differenceCents = q.value(8).toLongLong();
    s.status = q.value(9).toString();
    return s;
}
} // namespace

CashRepository::CashRepository(QSqlDatabase db)
    : m_db(db)
{
}

int CashRepository::open(int userId, qint64 openingCents)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO cash_sessions (opened_by, opening_cents) VALUES (:u, :o)"));
    q.bindValue(QStringLiteral(":u"), userId);
    q.bindValue(QStringLiteral(":o"), openingCents);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool CashRepository::current(CashSession *out)
{
    QSqlQuery q(m_db);
    if (!q.exec(QString::fromLatin1(kSelect) + QStringLiteral("WHERE s.status = 'open' LIMIT 1")) || !q.next())
        return false;
    if (out)
        *out = readRow(q);
    return true;
}

bool CashRepository::get(int id, CashSession *out)
{
    QSqlQuery q(m_db);
    q.prepare(QString::fromLatin1(kSelect) + QStringLiteral("WHERE s.id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec() || !q.next())
        return false;
    if (out)
        *out = readRow(q);
    return true;
}

bool CashRepository::close(int id, int userId, qint64 expected, qint64 counted)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE cash_sessions SET status = 'closed', closed_by = :u, closed_at = datetime('now','localtime'), "
        "expected_cents = :e, counted_cents = :c, difference_cents = :d WHERE id = :id AND status = 'open'"));
    q.bindValue(QStringLiteral(":u"), userId);
    q.bindValue(QStringLiteral(":e"), expected);
    q.bindValue(QStringLiteral(":c"), counted);
    q.bindValue(QStringLiteral(":d"), counted - expected);
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

bool CashRepository::insertCount(int sessionId, const CurrencyAmount &c)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO cash_counts (session_id, currency_code, rate_milli, expected_cents, counted_cents) "
        "VALUES (:s, :c, :r, :e, :n)"));
    q.bindValue(QStringLiteral(":s"), sessionId);
    q.bindValue(QStringLiteral(":c"), c.currency);
    q.bindValue(QStringLiteral(":r"), c.rateMilli);
    q.bindValue(QStringLiteral(":e"), c.expectedCents);
    q.bindValue(QStringLiteral(":n"), c.countedCents);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

QVector<CurrencyAmount> CashRepository::counts(int sessionId)
{
    QVector<CurrencyAmount> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT currency_code, rate_milli, expected_cents, counted_cents FROM cash_counts "
                             "WHERE session_id = :s ORDER BY id"));
    q.bindValue(QStringLiteral(":s"), sessionId);
    if (q.exec()) {
        while (q.next()) {
            CurrencyAmount c;
            c.currency = q.value(0).toString();
            c.rateMilli = q.value(1).toLongLong();
            c.expectedCents = q.value(2).toLongLong();
            c.countedCents = q.value(3).toLongLong();
            out.append(c);
        }
    }
    return out;
}

int CashRepository::insertMovement(int sessionId, int userId, qint64 amountCents, const QString &reason)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO cash_movements (session_id, user_id, amount_cents, reason) "
                             "VALUES (:s, :u, :a, :r)"));
    q.bindValue(QStringLiteral(":s"), sessionId);
    q.bindValue(QStringLiteral(":u"), userId);
    q.bindValue(QStringLiteral(":a"), amountCents);
    q.bindValue(QStringLiteral(":r"), reason);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

QVector<CashSession> CashRepository::recent(int limit)
{
    QVector<CashSession> out;
    QSqlQuery q(m_db);
    q.prepare(QString::fromLatin1(kSelect) + QStringLiteral("ORDER BY s.id DESC LIMIT :l"));
    q.bindValue(QStringLiteral(":l"), limit);
    if (q.exec()) {
        while (q.next())
            out.append(readRow(q));
    }
    return out;
}

SalesStats CashRepository::salesStats(int sessionId)
{
    SalesStats st;
    QSqlQuery q(m_db);
    // Los importes cuentan todas las ventas de la caja (tambien las anuladas despues): lo devuelto
    // se resta aparte, en la caja donde se devolvio. Solo la cantidad excluye las anuladas.
    q.prepare(QStringLiteral("SELECT COALESCE(SUM(CASE WHEN status = 'completed' THEN 1 ELSE 0 END), 0), "
                             "COALESCE(SUM(total_cents), 0), COALESCE(SUM(change_cents), 0) "
                             "FROM sales WHERE session_id = :s"));
    q.bindValue(QStringLiteral(":s"), sessionId);
    if (q.exec() && q.next()) {
        st.count = q.value(0).toInt();
        st.total = q.value(1).toLongLong();
        st.change = q.value(2).toLongLong();
    }
    q.prepare(QStringLiteral("SELECT COALESCE(SUM((i.qty_milli * i.cost_cents + 500) / 1000), 0) "
                             "FROM sale_items i JOIN sales s ON s.id = i.sale_id "
                             "WHERE s.session_id = :s"));
    q.bindValue(QStringLiteral(":s"), sessionId);
    if (q.exec() && q.next())
        st.cost = q.value(0).toLongLong();
    return st;
}

QVector<PaymentTotal> CashRepository::paymentTotals(int sessionId)
{
    QVector<PaymentTotal> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT p.method, p.currency_code, SUM(p.amount_cents), SUM(p.base_cents) "
        "FROM payments p JOIN sales s ON s.id = p.sale_id "
        "WHERE s.session_id = :s GROUP BY p.method, p.currency_code"));
    q.bindValue(QStringLiteral(":s"), sessionId);
    if (q.exec()) {
        while (q.next()) {
            PaymentTotal t;
            t.method = q.value(0).toString();
            t.currency = q.value(1).toString();
            t.amountCents = q.value(2).toLongLong();
            t.baseCents = q.value(3).toLongLong();
            out.append(t);
        }
    }
    return out;
}

void CashRepository::movementTotals(int sessionId, qint64 *in, qint64 *out)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT COALESCE(SUM(CASE WHEN amount_cents > 0 THEN amount_cents END), 0), "
        "COALESCE(SUM(CASE WHEN amount_cents < 0 THEN -amount_cents END), 0) "
        "FROM cash_movements WHERE session_id = :s"));
    q.bindValue(QStringLiteral(":s"), sessionId);
    qint64 i = 0, o = 0;
    if (q.exec() && q.next()) {
        i = q.value(0).toLongLong();
        o = q.value(1).toLongLong();
    }
    if (in)
        *in = i;
    if (out)
        *out = o;
}
