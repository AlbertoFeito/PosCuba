#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "core/Session.h"
#include "core/User.h"

class UserRepository
{
public:
    explicit UserRepository(QSqlDatabase db);

    // Primer administrador activo.
    bool firstAdmin(Session *out);

    QVector<User> list(bool includeInactive);
    bool get(int id, User *out);
    bool credentials(int id, QString *hash, QString *salt);
    bool nameExists(const QString &name, int excludeId);
    int countActiveAdmins();
    int insert(const QString &name, const QString &role, const QString &hash, const QString &salt); // id o -1
    bool update(int id, const QString &name, const QString &role);
    bool setPin(int id, const QString &hash, const QString &salt);
    bool setActive(int id, bool active);
    bool touchLogin(int id);

    QString lastError() const { return m_lastError; }

private:
    QSqlDatabase m_db;
    QString m_lastError;
};
