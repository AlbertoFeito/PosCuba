#pragma once

#include <QDateTime>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

struct BackupInfo
{
    QString path;
    QString fileName;
    QString kind;            // manual, auto, pre-restore
    QDateTime created;
    qint64 sizeBytes = 0;
    // Rellenado por inspect()
    bool inspected = false;
    bool valid = false;
    int schemaVersion = 0;
    int products = 0;
    int sales = 0;
    QString lastSaleAt;
    QString error;
};

// Respaldos de la base SQLite.
// - backup(): VACUUM INTO (copia consistente aunque la base este en uso y en modo WAL).
// - restore(): copia el respaldo a un temporal, lo migra al esquema actual y reemplaza
//   los datos tabla por tabla en una transaccion (ATTACH). No hace falta reiniciar la app.
class BackupService
{
public:
    static QString kindManual() { return QStringLiteral("manual"); }
    static QString kindAuto() { return QStringLiteral("auto"); }
    static QString kindPreRestore() { return QStringLiteral("pre-restore"); }

    explicit BackupService(QSqlDatabase db);

    static QString defaultFolder();
    static QString fileNameFor(const QString &kind, const QDateTime &when);
    // Respaldo automatico pendiente: nunca se hizo o el ultimo fue otro dia.
    static bool isAutoDue(const QDateTime &lastAuto, const QDateTime &now);

    bool backup(const QString &folder, const QString &kind, QString *outPath, QString *error);
    BackupInfo inspect(const QString &path);
    QVector<BackupInfo> list(const QString &folder) const;   // mas recientes primero, sin inspeccionar
    // Antes de reemplazar guarda los datos actuales en safetyFolder (kind pre-restore).
    bool restore(const QString &path, const QString &safetyFolder, QString *safetyPath, QString *error);
    // Borra los respaldos automaticos mas viejos, deja "keep". Devuelve cuantos borro.
    int prune(const QString &folder, int keep) const;

private:
    QSqlDatabase m_db;
};
