#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <QVariantMap>

#include "core/Session.h"
#include "core/services/BackupService.h"
#include "data/repositories/SettingsRepository.h"

class BackupController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString folder READ folder NOTIFY changed)
    Q_PROPERTY(QString defaultFolder READ defaultFolder CONSTANT)
    Q_PROPERTY(bool usingDefault READ usingDefault NOTIFY changed)
    // [{ root, folder, label, freeText, system }]
    Q_PROPERTY(QVariantList drives READ drives NOTIFY changed)
    // [{ path, fileName, kind, kindText, dateText, sizeText }]
    Q_PROPERTY(QVariantList backups READ backups NOTIFY changed)
    Q_PROPERTY(bool autoEnabled READ autoEnabled NOTIFY changed)
    Q_PROPERTY(int keepCount READ keepCount NOTIFY changed)
    Q_PROPERTY(QString lastAutoText READ lastAutoText NOTIFY changed)

public:
    BackupController(QSqlDatabase db, const Session *session, QObject *parent = nullptr);

    QString folder() const { return m_folder; }
    QString defaultFolder() const { return BackupService::defaultFolder(); }
    bool usingDefault() const;
    QVariantList drives() const { return m_drives; }
    QVariantList backups() const { return m_backups; }
    bool autoEnabled() const;
    int keepCount() const;
    QString lastAutoText() const;

    Q_INVOKABLE void useDefaultFolder();
    Q_INVOKABLE void useDrive(const QString &root);
    Q_INVOKABLE QVariantMap useFolder(const QString &path);

    // Salida: { ok, error, path }
    Q_INVOKABLE QVariantMap backupNow();
    // Salida: { valid, error, dateText, sizeText, products, sales, lastSaleText, schemaVersion }
    Q_INVOKABLE QVariantMap inspect(const QString &path);
    // Salida: { ok, error, safetyPath }
    Q_INVOKABLE QVariantMap restore(const QString &path);
    Q_INVOKABLE QVariantMap saveAutoSettings(bool enabled, int keep);
    Q_INVOKABLE void openFolder() const;

public slots:
    void refresh();
    // Respaldo automatico si esta activado y hoy aun no se hizo (al iniciar).
    void autoBackupIfDue();
    // Respaldo automatico incondicional si esta activado (al cerrar caja).
    void autoBackupNow();

signals:
    void changed();
    void dataRestored();

private:
    bool isAdmin() const { return m_session && m_session->isAdmin(); }
    void refreshDrives();
    static QString sizeText(qint64 bytes);
    static QString kindText(const QString &kind);

    BackupService m_service;
    mutable SettingsRepository m_settings;
    const Session *m_session;
    QString m_folder;
    QVariantList m_drives;
    QVariantList m_backups;
};
