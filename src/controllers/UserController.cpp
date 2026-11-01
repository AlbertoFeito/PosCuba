#include "UserController.h"

#include <QDateTime>

#include "core/services/UserService.h"

namespace {
QVariantMap result(bool ok, const QString &error)
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("error"), error}};
}
} // namespace

UserController::UserController(UserService *service, const Session *session, QObject *parent)
    : QObject(parent)
    , m_service(service)
    , m_session(session)
{
}

bool UserController::allowed(QVariantMap *denied) const
{
    if (m_service && m_session && m_session->isAdmin())
        return true;
    *denied = result(false, tr("Solo un administrador puede gestionar usuarios."));
    return false;
}

void UserController::refresh()
{
    m_users.clear();
    if (m_service && m_session && m_session->isAdmin()) {
        for (const User &u : m_service->list(true)) {
            const QDateTime last = QDateTime::fromString(u.lastLoginAt, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
            m_users.append(QVariantMap{
                {QStringLiteral("id"), u.id},
                {QStringLiteral("name"), u.name},
                {QStringLiteral("role"), u.role},
                {QStringLiteral("roleText"), u.role == Role::admin() ? tr("Administrador") : tr("Cajero")},
                {QStringLiteral("active"), u.active},
                {QStringLiteral("hasPin"), u.hasPin},
                {QStringLiteral("lastLoginText"),
                 last.isValid() ? last.toString(QStringLiteral("dd/MM/yyyy HH:mm")) : tr("nunca")},
                {QStringLiteral("isSelf"), u.id == m_session->userId},
            });
        }
    }
    emit usersChanged();
}

QVariantMap UserController::create(const QString &name, const QString &role, const QString &pin, const QString &confirm)
{
    QVariantMap denied;
    if (!allowed(&denied))
        return denied;
    if (pin != confirm)
        return result(false, tr("Los PIN no coinciden."));
    QString error;
    if (m_service->create(name, role, pin, &error) <= 0)
        return result(false, error);
    refresh();
    return result(true, QString());
}

QVariantMap UserController::update(int id, const QString &name, const QString &role)
{
    QVariantMap denied;
    if (!allowed(&denied))
        return denied;
    QString error;
    if (!m_service->update(id, name, role, &error))
        return result(false, error);
    refresh();
    return result(true, QString());
}

QVariantMap UserController::resetPin(int id, const QString &pin, const QString &confirm)
{
    QVariantMap denied;
    if (!allowed(&denied))
        return denied;
    if (pin != confirm)
        return result(false, tr("Los PIN no coinciden."));
    QString error;
    if (!m_service->resetPin(id, pin, &error))
        return result(false, error);
    refresh();
    return result(true, QString());
}

QVariantMap UserController::setActive(int id, bool active)
{
    QVariantMap denied;
    if (!allowed(&denied))
        return denied;
    QString error;
    if (!m_service->setActive(id, active, m_session->userId, &error))
        return result(false, error);
    refresh();
    return result(true, QString());
}
