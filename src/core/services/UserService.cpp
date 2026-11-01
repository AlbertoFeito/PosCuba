#include "UserService.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QRandomGenerator>

namespace {
QString tr(const char *s) { return QCoreApplication::translate("UserService", s); }
void setErr(QString *error, const QString &msg) { if (error) *error = msg; }
bool validRole(const QString &r) { return r == Role::admin() || r == Role::cashier(); }
} // namespace

UserService::UserService(QSqlDatabase db)
    : m_users(db)
{
}

bool UserService::isValidPin(const QString &pin)
{
    if (pin.size() < 4 || pin.size() > 8)
        return false;
    for (const QChar c : pin) {
        if (c < QLatin1Char('0') || c > QLatin1Char('9'))
            return false;
    }
    return true;
}

QString UserService::newSalt()
{
    QByteArray bytes(16, Qt::Uninitialized);
    for (int i = 0; i < bytes.size(); ++i)
        bytes[i] = char(QRandomGenerator::system()->bounded(256));
    return QString::fromLatin1(bytes.toHex());
}

QString UserService::hashPin(const QString &pin, const QString &salt)
{
    return QString::fromLatin1(
        QCryptographicHash::hash((salt + pin).toUtf8(), QCryptographicHash::Sha256).toHex());
}

QString UserService::validateName(const QString &rawName, int excludeId)
{
    const QString name = rawName.simplified();
    if (name.isEmpty())
        return tr("El nombre es obligatorio.");
    if (name.size() > 40)
        return tr("El nombre no puede tener más de 40 caracteres.");
    if (m_users.nameExists(name, excludeId))
        return tr("Ya existe un usuario llamado «%1».").arg(name);
    return QString();
}

bool UserService::needsSetup(User *admin)
{
    bool anyPin = false;
    User first;
    for (const User &u : m_users.list(false)) {
        if (u.role != Role::admin())
            continue;
        if (u.hasPin)
            anyPin = true;
        else if (first.id == 0)
            first = u;
    }
    if (anyPin || first.id == 0)
        return false;
    if (admin)
        *admin = first;
    return true;
}

bool UserService::setupAdmin(const QString &pin, const QString &confirm, Session *session, QString *error)
{
    User admin;
    if (!needsSetup(&admin)) {
        setErr(error, tr("La configuración inicial ya se hizo."));
        return false;
    }
    if (!isValidPin(pin)) {
        setErr(error, tr("El PIN debe tener entre 4 y 8 dígitos."));
        return false;
    }
    if (pin != confirm) {
        setErr(error, tr("Los PIN no coinciden."));
        return false;
    }
    const QString salt = newSalt();
    if (!m_users.setPin(admin.id, hashPin(pin, salt), salt)) {
        setErr(error, tr("No se pudo guardar el PIN."));
        return false;
    }
    return authenticate(admin.id, pin, session, error);
}

int UserService::secondsLocked(int userId) const
{
    const auto it = m_attempts.constFind(userId);
    if (it == m_attempts.constEnd() || !it->lockedUntil.isValid())
        return 0;
    return int(qMax<qint64>(0, QDateTime::currentDateTime().secsTo(it->lockedUntil)));
}

bool UserService::checkPin(int userId, const QString &pin)
{
    QString hash, salt;
    if (!m_users.credentials(userId, &hash, &salt) || hash.isEmpty())
        return false;
    return hashPin(pin, salt) == hash;
}

bool UserService::checkWithLockout(int userId, const QString &pin, User *user, QString *error)
{
    const int locked = secondsLocked(userId);
    if (locked > 0) {
        setErr(error, tr("Demasiados intentos. Espera %1 segundos.").arg(locked));
        return false;
    }

    User u;
    if (!m_users.get(userId, &u) || !u.active) {
        setErr(error, tr("El usuario no existe o está desactivado."));
        return false;
    }
    if (!u.hasPin) {
        setErr(error, tr("Este usuario no tiene PIN. Pide a un administrador que lo asigne."));
        return false;
    }

    if (!checkPin(userId, pin)) {
        Attempts &a = m_attempts[userId];
        if (++a.failures >= MaxAttempts) {
            a.failures = 0;
            a.lockedUntil = QDateTime::currentDateTime().addSecs(LockSeconds);
            setErr(error, tr("PIN incorrecto. Usuario bloqueado %1 segundos.").arg(LockSeconds));
        } else {
            setErr(error, tr("PIN incorrecto. Te quedan %1 intento(s).").arg(MaxAttempts - a.failures));
        }
        return false;
    }
    m_attempts.remove(userId);
    if (user)
        *user = u;
    return true;
}

bool UserService::authenticate(int userId, const QString &pin, Session *session, QString *error)
{
    User u;
    if (!checkWithLockout(userId, pin, &u, error))
        return false;
    m_users.touchLogin(userId);
    if (session) {
        session->userId = u.id;
        session->userName = u.name;
        session->role = u.role;
    }
    return true;
}

bool UserService::verifyAdmin(int userId, const QString &pin, QString *error)
{
    User u;
    if (!m_users.get(userId, &u) || u.role != Role::admin()) {
        setErr(error, tr("Elige un administrador."));
        return false;
    }
    return checkWithLockout(userId, pin, nullptr, error);
}

int UserService::create(const QString &name, const QString &role, const QString &pin, QString *error)
{
    const QString nameError = validateName(name, 0);
    if (!nameError.isEmpty()) {
        setErr(error, nameError);
        return -1;
    }
    if (!validRole(role)) {
        setErr(error, tr("Rol no válido."));
        return -1;
    }
    if (!isValidPin(pin)) {
        setErr(error, tr("El PIN debe tener entre 4 y 8 dígitos."));
        return -1;
    }
    const QString salt = newSalt();
    const int id = m_users.insert(name.simplified(), role, hashPin(pin, salt), salt);
    if (id <= 0)
        setErr(error, tr("No se pudo crear el usuario: %1").arg(m_users.lastError()));
    return id;
}

bool UserService::update(int id, const QString &name, const QString &role, QString *error)
{
    User u;
    if (!m_users.get(id, &u)) {
        setErr(error, tr("El usuario no existe."));
        return false;
    }
    const QString nameError = validateName(name, id);
    if (!nameError.isEmpty()) {
        setErr(error, nameError);
        return false;
    }
    if (!validRole(role)) {
        setErr(error, tr("Rol no válido."));
        return false;
    }
    if (u.active && u.role == Role::admin() && role != Role::admin() && m_users.countActiveAdmins() <= 1) {
        setErr(error, tr("Debe quedar al menos un administrador activo."));
        return false;
    }
    if (!m_users.update(id, name.simplified(), role)) {
        setErr(error, tr("No se pudo guardar el usuario."));
        return false;
    }
    return true;
}

bool UserService::resetPin(int id, const QString &pin, QString *error)
{
    if (!isValidPin(pin)) {
        setErr(error, tr("El PIN debe tener entre 4 y 8 dígitos."));
        return false;
    }
    const QString salt = newSalt();
    if (!m_users.setPin(id, hashPin(pin, salt), salt)) {
        setErr(error, tr("No se pudo cambiar el PIN."));
        return false;
    }
    m_attempts.remove(id);
    return true;
}

bool UserService::changeOwnPin(int id, const QString &oldPin, const QString &newPin, QString *error)
{
    if (!checkPin(id, oldPin)) {
        setErr(error, tr("El PIN actual no es correcto."));
        return false;
    }
    return resetPin(id, newPin, error);
}

bool UserService::setActive(int id, bool active, int actingUserId, QString *error)
{
    User u;
    if (!m_users.get(id, &u)) {
        setErr(error, tr("El usuario no existe."));
        return false;
    }
    if (!active && id == actingUserId) {
        setErr(error, tr("No puedes desactivar tu propio usuario."));
        return false;
    }
    if (!active && u.active && u.role == Role::admin() && m_users.countActiveAdmins() <= 1) {
        setErr(error, tr("Debe quedar al menos un administrador activo."));
        return false;
    }
    if (!m_users.setActive(id, active)) {
        setErr(error, tr("No se pudo cambiar el estado del usuario."));
        return false;
    }
    return true;
}
