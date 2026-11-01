#include "UserRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
const char *kSelect = "SELECT id, name, role, active, pin_hash <> '', created_at, last_login_at FROM users ";

User readRow(const QSqlQuery &q)
{
    User u;
    u.id = q.value(0).toInt();
    u.name = q.value(1).toString();
    u.role = q.value(2).toString();
    u.active = q.value(3).toBool();
    u.hasPin = q.value(4).toBool();
    u.createdAt = q.value(5).toString();
    u.lastLoginAt = q.value(6).toString();
    return u;
}
} // namespace

UserRepository::UserRepository(QSqlDatabase db)
    : m_db(db)
{
}

bool UserRepository::firstAdmin(Session *out)
{
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT id, name, role FROM users WHERE role = 'admin' AND active = 1 "
                               "ORDER BY id LIMIT 1"))
        || !q.next())
        return false;
    if (out) {
        out->userId = q.value(0).toInt();
        out->userName = q.value(1).toString();
        out->role = q.value(2).toString();
    }
    return true;
}

QVector<User> UserRepository::list(bool includeInactive)
{
    QVector<User> out;
    QSqlQuery q(m_db);
    const QString where = includeInactive ? QString() : QStringLiteral("WHERE active = 1 ");
    if (!q.exec(QString::fromLatin1(kSelect) + where
                + QStringLiteral("ORDER BY active DESC, role = 'admin' DESC, name COLLATE NOCASE"))) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next())
        out.append(readRow(q));
    return out;
}

bool UserRepository::get(int id, User *out)
{
    QSqlQuery q(m_db);
    q.prepare(QString::fromLatin1(kSelect) + QStringLiteral("WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec() || !q.next())
        return false;
    if (out)
        *out = readRow(q);
    return true;
}

bool UserRepository::credentials(int id, QString *hash, QString *salt)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT pin_hash, pin_salt FROM users WHERE id = :id AND active = 1"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec() || !q.next())
        return false;
    *hash = q.value(0).toString();
    *salt = q.value(1).toString();
    return true;
}

bool UserRepository::nameExists(const QString &name, int excludeId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT 1 FROM users WHERE name = :n COLLATE NOCASE AND id <> :id"));
    q.bindValue(QStringLiteral(":n"), name);
    q.bindValue(QStringLiteral(":id"), excludeId);
    return q.exec() && q.next();
}

int UserRepository::countActiveAdmins()
{
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT COUNT(*) FROM users WHERE role = 'admin' AND active = 1")) && q.next())
        return q.value(0).toInt();
    return 0;
}

int UserRepository::insert(const QString &name, const QString &role, const QString &hash, const QString &salt)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO users (name, pin_hash, pin_salt, role) VALUES (:n, :h, :s, :r)"));
    q.bindValue(QStringLiteral(":n"), name);
    q.bindValue(QStringLiteral(":h"), hash);
    q.bindValue(QStringLiteral(":s"), salt);
    q.bindValue(QStringLiteral(":r"), role);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool UserRepository::update(int id, const QString &name, const QString &role)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE users SET name = :n, role = :r WHERE id = :id"));
    q.bindValue(QStringLiteral(":n"), name);
    q.bindValue(QStringLiteral(":r"), role);
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

bool UserRepository::setPin(int id, const QString &hash, const QString &salt)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE users SET pin_hash = :h, pin_salt = :s WHERE id = :id"));
    q.bindValue(QStringLiteral(":h"), hash);
    q.bindValue(QStringLiteral(":s"), salt);
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

bool UserRepository::setActive(int id, bool active)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE users SET active = :a WHERE id = :id"));
    q.bindValue(QStringLiteral(":a"), active ? 1 : 0);
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

bool UserRepository::touchLogin(int id)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE users SET last_login_at = datetime('now','localtime') WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec();
}
