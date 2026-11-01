#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

#include "core/Session.h"

class UserService;

// Estado global de la app expuesto a QML como "app": base de datos, tema y sesion.
class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dbReady READ dbReady NOTIFY statusChanged)
    Q_PROPERTY(QString dbPath READ dbPath NOTIFY statusChanged)
    Q_PROPERTY(int schemaVersion READ schemaVersion NOTIFY statusChanged)
    Q_PROPERTY(QString sqliteVersion READ sqliteVersion NOTIFY statusChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY statusChanged)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(bool darkTheme READ darkTheme WRITE setDarkTheme NOTIFY darkThemeChanged)

    // Sesion
    Q_PROPERTY(bool loggedIn READ loggedIn NOTIFY sessionChanged)
    Q_PROPERTY(QString userName READ userName NOTIFY sessionChanged)
    Q_PROPERTY(QString userRole READ userRole NOTIFY sessionChanged)
    Q_PROPERTY(bool isAdmin READ isAdmin NOTIFY sessionChanged)
    Q_PROPERTY(bool needsSetup READ needsSetup NOTIFY loginUsersChanged)
    Q_PROPERTY(QString setupUserName READ setupUserName NOTIFY loginUsersChanged)
    // [{ id, name, role, roleText, hasPin }]
    Q_PROPERTY(QVariantList loginUsers READ loginUsers NOTIFY loginUsersChanged)

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    void initialize(const QString &dbPath);
    const Session *session() const { return &m_session; }
    UserService *userService() const { return m_users.get(); }

    bool dbReady() const { return m_dbReady; }
    QString dbPath() const { return m_dbPath; }
    int schemaVersion() const { return m_schemaVersion; }
    QString sqliteVersion() const { return m_sqliteVersion; }
    QString errorMessage() const { return m_errorMessage; }
    QString qtVersion() const;
    QString appVersion() const;
    bool darkTheme() const { return m_darkTheme; }
    void setDarkTheme(bool dark);

    bool loggedIn() const { return m_session.isValid(); }
    QString userName() const { return m_session.userName; }
    QString userRole() const { return m_session.role; }
    bool isAdmin() const { return m_session.isAdmin(); }
    bool needsSetup() const { return m_needsSetup; }
    QString setupUserName() const { return m_setupUserName; }
    QVariantList loginUsers() const { return m_loginUsers; }

    // Salida: { ok, error }
    Q_INVOKABLE QVariantMap login(int userId, const QString &pin);
    Q_INVOKABLE QVariantMap setupAdmin(const QString &pin, const QString &confirm);
    Q_INVOKABLE QVariantMap changeOwnPin(const QString &oldPin, const QString &newPin, const QString &confirm);
    Q_INVOKABLE void logout();

public slots:
    void refreshLoginUsers();
    // El usuario actual pudo cambiar (nombre o rol) desde la pantalla de usuarios.
    void refreshSessionUser();
    // Los datos se reemplazaron desde un respaldo: cerrar sesion y releer ajustes.
    void reloadAfterRestore();

signals:
    void statusChanged();
    void darkThemeChanged();
    void sessionChanged();
    void loginUsersChanged();

private:
    bool m_darkTheme = false;
    Session m_session;
    std::unique_ptr<UserService> m_users;
    bool m_needsSetup = false;
    QString m_setupUserName;
    QVariantList m_loginUsers;
    bool m_dbReady = false;
    QString m_dbPath;
    int m_schemaVersion = 0;
    QString m_sqliteVersion;
    QString m_errorMessage;
};
