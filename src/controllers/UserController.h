#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

#include "core/Session.h"

class UserService;

// Gestion de usuarios (solo administradores).
class UserController : public QObject
{
    Q_OBJECT
    // [{ id, name, role, roleText, active, hasPin, lastLoginText, isSelf }]
    Q_PROPERTY(QVariantList users READ users NOTIFY usersChanged)

public:
    UserController(UserService *service, const Session *session, QObject *parent = nullptr);

    QVariantList users() const { return m_users; }

    // Salida: { ok, error }
    Q_INVOKABLE QVariantMap create(const QString &name, const QString &role, const QString &pin, const QString &confirm);
    Q_INVOKABLE QVariantMap update(int id, const QString &name, const QString &role);
    Q_INVOKABLE QVariantMap resetPin(int id, const QString &pin, const QString &confirm);
    Q_INVOKABLE QVariantMap setActive(int id, bool active);

public slots:
    void refresh();

signals:
    void usersChanged();

private:
    bool allowed(QVariantMap *denied) const;

    UserService *m_service;
    const Session *m_session;
    QVariantList m_users;
};
