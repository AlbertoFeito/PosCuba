#pragma once

#include <QDateTime>
#include <QHash>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "core/Session.h"
#include "core/User.h"
#include "data/repositories/UserRepository.h"

// Usuarios y autenticacion con PIN.
// PIN: 4 a 8 digitos. Se guarda SHA-256(sal + PIN) con una sal aleatoria por usuario.
// Tras MaxAttempts intentos fallidos el usuario queda bloqueado LockSeconds segundos.
class UserService
{
public:
    static constexpr int MaxAttempts = 5;
    static constexpr int LockSeconds = 30;

    explicit UserService(QSqlDatabase db);

    static bool isValidPin(const QString &pin);
    static QString newSalt();
    static QString hashPin(const QString &pin, const QString &salt);

    // Primer inicio: ningun administrador activo tiene PIN.
    bool needsSetup(User *admin);
    bool setupAdmin(const QString &pin, const QString &confirm, Session *session, QString *error);

    bool authenticate(int userId, const QString &pin, Session *session, QString *error);
    // Autorizacion de un administrador sin iniciar sesion (devoluciones). Cuenta para el bloqueo.
    bool verifyAdmin(int userId, const QString &pin, QString *error);
    int secondsLocked(int userId) const;
    void clearLockouts() { m_attempts.clear(); }

    QVector<User> list(bool includeInactive) { return m_users.list(includeInactive); }
    int create(const QString &name, const QString &role, const QString &pin, QString *error);
    bool update(int id, const QString &name, const QString &role, QString *error);
    bool resetPin(int id, const QString &pin, QString *error);
    bool changeOwnPin(int id, const QString &oldPin, const QString &newPin, QString *error);
    bool setActive(int id, bool active, int actingUserId, QString *error);

private:
    bool checkWithLockout(int userId, const QString &pin, User *user, QString *error);
    struct Attempts
    {
        int failures = 0;
        QDateTime lockedUntil;
    };

    bool checkPin(int userId, const QString &pin);
    QString validateName(const QString &name, int excludeId);

    UserRepository m_users;
    QHash<int, Attempts> m_attempts;
};
