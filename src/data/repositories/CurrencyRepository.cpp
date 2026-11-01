#include "CurrencyRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
Currency readRow(const QSqlQuery &q)
{
    Currency c;
    c.code = q.value(0).toString();
    c.name = q.value(1).toString();
    c.symbol = q.value(2).toString();
    c.rateMilli = q.value(3).toLongLong();
    c.isBase = q.value(4).toBool();
    c.active = q.value(5).toBool();
    return c;
}
const char *kSelect = "SELECT code, name, symbol, rate_milli, is_base, active FROM currencies ";
} // namespace

CurrencyRepository::CurrencyRepository(QSqlDatabase db)
    : m_db(db)
{
}

QVector<Currency> CurrencyRepository::active()
{
    QVector<Currency> out;
    QSqlQuery q(m_db);
    if (q.exec(QString::fromLatin1(kSelect) + QStringLiteral("WHERE active = 1 ORDER BY is_base DESC, code"))) {
        while (q.next())
            out.append(readRow(q));
    }
    return out;
}

bool CurrencyRepository::base(Currency *out)
{
    QSqlQuery q(m_db);
    if (!q.exec(QString::fromLatin1(kSelect) + QStringLiteral("WHERE is_base = 1")) || !q.next())
        return false;
    if (out)
        *out = readRow(q);
    return true;
}

bool CurrencyRepository::get(const QString &code, Currency *out)
{
    QSqlQuery q(m_db);
    q.prepare(QString::fromLatin1(kSelect) + QStringLiteral("WHERE code = :c"));
    q.bindValue(QStringLiteral(":c"), code);
    if (!q.exec() || !q.next())
        return false;
    if (out)
        *out = readRow(q);
    return true;
}

QVector<Currency> CurrencyRepository::all()
{
    QVector<Currency> out;
    QSqlQuery q(m_db);
    if (q.exec(QString::fromLatin1(kSelect) + QStringLiteral("ORDER BY is_base DESC, active DESC, code"))) {
        while (q.next())
            out.append(readRow(q));
    }
    return out;
}

bool CurrencyRepository::insert(const Currency &c)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO currencies (code, name, symbol, rate_milli, is_base, active) "
                             "VALUES (:c, :n, :s, :r, 0, :a)"));
    q.bindValue(QStringLiteral(":c"), c.code);
    q.bindValue(QStringLiteral(":n"), c.name);
    q.bindValue(QStringLiteral(":s"), c.symbol);
    q.bindValue(QStringLiteral(":r"), c.rateMilli);
    q.bindValue(QStringLiteral(":a"), c.active ? 1 : 0);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool CurrencyRepository::update(const Currency &c)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE currencies SET name = :n, symbol = :s, rate_milli = :r, active = :a "
                             "WHERE code = :c"));
    q.bindValue(QStringLiteral(":n"), c.name);
    q.bindValue(QStringLiteral(":s"), c.symbol);
    q.bindValue(QStringLiteral(":r"), c.rateMilli);
    q.bindValue(QStringLiteral(":a"), c.active ? 1 : 0);
    q.bindValue(QStringLiteral(":c"), c.code);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

bool CurrencyRepository::setBase(const QString &code)
{
    QSqlQuery q(m_db);
    // El indice unico parcial exige quitar la base anterior antes de marcar la nueva.
    if (!q.exec(QStringLiteral("UPDATE currencies SET is_base = 0 WHERE is_base = 1"))) {
        m_lastError = q.lastError().text();
        return false;
    }
    q.prepare(QStringLiteral("UPDATE currencies SET is_base = 1, active = 1, rate_milli = 1000 WHERE code = :c"));
    q.bindValue(QStringLiteral(":c"), code);
    if (!q.exec() || q.numRowsAffected() != 1) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}
