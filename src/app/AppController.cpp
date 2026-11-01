#include "AppController.h"

#include <QCoreApplication>

#include "core/services/UserService.h"
#include "data/Database.h"
#include "data/Migrator.h"
#include "data/repositories/SettingsRepository.h"
#include "data/repositories/UserRepository.h"

namespace {
QVariantMap result(bool ok, const QString &error)
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("error"), error}};
}
} // namespace

AppController::AppController(QObject *parent)
    : QObject(parent)
{
}

AppController::~AppController() = default;

void AppController::initialize(const QString &dbPath)
{
    m_dbPath = dbPath;
    m_errorMessage.clear();

    QString error;
    if (!Database::open(dbPath, &error)) {
        m_errorMessage = tr("No se pudo abrir la base de datos: %1").arg(error);
    } else if (!Migrator::run(Database::connection(), &error)) {
        m_errorMessage = tr("Error al migrar la base de datos: %1").arg(error);
    } else {
        m_dbReady = true;
        SettingsRepository settings(Database::connection());
        m_darkTheme = settings.value(QStringLiteral("theme")) == QLatin1String("dark");
        emit darkThemeChanged();
        m_users.reset(new UserService(Database::connection()));
        refreshLoginUsers();
    }

    if (Database::connection().isOpen()) {
        m_schemaVersion = Migrator::currentVersion(Database::connection());
        m_sqliteVersion = Database::sqliteVersion();
    }

    emit statusChanged();
}

QString AppController::qtVersion() const
{
    return QString::fromLatin1(qVersion());
}

void AppController::setDarkTheme(bool dark)
{
    if (m_darkTheme == dark)
        return;
    m_darkTheme = dark;
    if (m_dbReady) {
        SettingsRepository settings(Database::connection());
        settings.setValue(QStringLiteral("theme"), dark ? QStringLiteral("dark") : QStringLiteral("light"));
    }
    emit darkThemeChanged();
}

QString AppController::appVersion() const
{
    return QCoreApplication::applicationVersion();
}

void AppController::refreshLoginUsers()
{
    m_loginUsers.clear();
    m_needsSetup = false;
    m_setupUserName.clear();
    if (!m_users) {
        emit loginUsersChanged();
        return;
    }
    User admin;
    m_needsSetup = m_users->needsSetup(&admin);
    m_setupUserName = admin.name;
    for (const User &u : m_users->list(false)) {
        m_loginUsers.append(QVariantMap{
            {QStringLiteral("id"), u.id},
            {QStringLiteral("name"), u.name},
            {QStringLiteral("role"), u.role},
            {QStringLiteral("roleText"), u.role == Role::admin() ? tr("Administrador") : tr("Cajero")},
            {QStringLiteral("hasPin"), u.hasPin},
        });
    }
    emit loginUsersChanged();
}

void AppController::refreshSessionUser()
{
    if (!m_session.isValid() || !m_dbReady)
        return;
    User u;
    if (!UserRepository(Database::connection()).get(m_session.userId, &u) || !u.active) {
        logout();
        return;
    }
    // Solo notificar si algo cambio: la pantalla de usuarios refresca al mostrarse y una
    // notificacion innecesaria re-evalua los bindings que la mostraron (bucle).
    if (m_session.userName == u.name && m_session.role == u.role)
        return;
    m_session.userName = u.name;
    m_session.role = u.role;
    emit sessionChanged();
}

QVariantMap AppController::login(int userId, const QString &pin)
{
    if (!m_users)
        return result(false, tr("La base de datos no está disponible."));
    QString error;
    Session s;
    if (!m_users->authenticate(userId, pin, &s, &error))
        return result(false, error);
    m_session = s;
    emit sessionChanged();
    return result(true, QString());
}

QVariantMap AppController::setupAdmin(const QString &pin, const QString &confirm)
{
    if (!m_users)
        return result(false, tr("La base de datos no está disponible."));
    QString error;
    Session s;
    if (!m_users->setupAdmin(pin, confirm, &s, &error))
        return result(false, error);
    m_session = s;
    refreshLoginUsers();
    emit sessionChanged();
    return result(true, QString());
}

QVariantMap AppController::changeOwnPin(const QString &oldPin, const QString &newPin, const QString &confirm)
{
    if (!m_users || !m_session.isValid())
        return result(false, tr("No hay una sesión activa."));
    if (newPin != confirm)
        return result(false, tr("Los PIN nuevos no coinciden."));
    QString error;
    if (!m_users->changeOwnPin(m_session.userId, oldPin, newPin, &error))
        return result(false, error);
    return result(true, QString());
}

void AppController::logout()
{
    if (!m_session.isValid())
        return;
    m_session = Session();
    refreshLoginUsers();
    emit sessionChanged();
}

void AppController::reloadAfterRestore()
{
    if (!m_dbReady)
        return;
    SettingsRepository settings(Database::connection());
    const bool dark = settings.value(QStringLiteral("theme")) == QLatin1String("dark");
    if (dark != m_darkTheme) {
        m_darkTheme = dark;
        emit darkThemeChanged();
    }
    m_session = Session();
    if (m_users)
        m_users->clearLockouts();   // el servicio se conserva: UserController guarda un puntero a el
    refreshLoginUsers();
    emit sessionChanged();
}
