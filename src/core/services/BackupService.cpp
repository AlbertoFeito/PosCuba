#include "BackupService.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringList>
#include <QVariant>

#include "data/Migrator.h"

namespace {

QString tr(const char *s) { return QCoreApplication::translate("BackupService", s); }
void setErr(QString *error, const QString &msg) { if (error) *error = msg; }

const QString kPrefix = QStringLiteral("PosCuba_");
const QString kDateFormat = QStringLiteral("yyyy-MM-dd_HHmmss");

QString kindPrefix(const QString &kind)
{
    if (kind == BackupService::kindAuto())
        return kPrefix + QStringLiteral("auto_");
    if (kind == BackupService::kindPreRestore())
        return kPrefix + QStringLiteral("antes_de_restaurar_");
    return kPrefix + QStringLiteral("respaldo_");
}

QString kindFromName(const QString &name)
{
    if (name.startsWith(kindPrefix(BackupService::kindAuto())))
        return BackupService::kindAuto();
    if (name.startsWith(kindPrefix(BackupService::kindPreRestore())))
        return BackupService::kindPreRestore();
    return BackupService::kindManual();
}

QString uniqueConnectionName(const QString &base)
{
    return base + QString::number(QRandomGenerator::global()->generate());
}

// Abre una base aparte (para inspeccionar o migrar un archivo) y la cierra al salir del ambito.
class ScopedConnection
{
public:
    ScopedConnection(const QString &path, bool readOnly)
        : m_name(uniqueConnectionName(QStringLiteral("pos_backup_")))
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_name);
        db.setDatabaseName(path);
        if (readOnly)
            db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        m_open = db.open();
        if (!m_open)
            m_error = db.lastError().text();
    }
    ~ScopedConnection()
    {
        {
            QSqlDatabase db = QSqlDatabase::database(m_name, false);
            db.close();
        }
        QSqlDatabase::removeDatabase(m_name);
    }
    bool isOpen() const { return m_open; }
    QString error() const { return m_error; }
    QSqlDatabase db() const { return QSqlDatabase::database(m_name, false); }

private:
    QString m_name;
    bool m_open = false;
    QString m_error;
};

QString sqlPath(const QString &path)
{
    QString p = QDir::toNativeSeparators(path);
    p.replace(QLatin1Char('\''), QStringLiteral("''"));
    return p;
}

} // namespace

BackupService::BackupService(QSqlDatabase db)
    : m_db(db)
{
}

QString BackupService::defaultFolder()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath(QStringLiteral("PosCuba/Respaldos"));
}

QString BackupService::fileNameFor(const QString &kind, const QDateTime &when)
{
    return kindPrefix(kind) + when.toString(kDateFormat) + QStringLiteral(".db");
}

bool BackupService::isAutoDue(const QDateTime &lastAuto, const QDateTime &now)
{
    return !lastAuto.isValid() || lastAuto.date() < now.date();
}

bool BackupService::backup(const QString &folder, const QString &kind, QString *outPath, QString *error)
{
    if (!QDir().mkpath(folder)) {
        setErr(error, tr("No se pudo crear la carpeta %1.").arg(QDir::toNativeSeparators(folder)));
        return false;
    }
    QString path = QDir(folder).filePath(fileNameFor(kind, QDateTime::currentDateTime()));
    for (int i = 2; QFileInfo::exists(path); ++i)   // dos respaldos en el mismo segundo
        path = QDir(folder).filePath(fileNameFor(kind, QDateTime::currentDateTime()).replace(
            QStringLiteral(".db"), QStringLiteral("_%1.db").arg(i)));

    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("VACUUM INTO '%1'").arg(sqlPath(path)))) {
        // SQLite < 3.27 no tiene VACUUM INTO: volcar el WAL y copiar el archivo.
        const QString vacuumError = q.lastError().text();
        QFile::remove(path);
        q.exec(QStringLiteral("PRAGMA wal_checkpoint(TRUNCATE)"));
        if (m_db.databaseName() == QLatin1String(":memory:") || !QFile::copy(m_db.databaseName(), path)) {
            setErr(error, tr("No se pudo crear el respaldo: %1").arg(vacuumError));
            return false;
        }
    }

    const BackupInfo info = inspect(path);
    if (!info.valid) {
        QFile::remove(path);
        setErr(error, tr("El respaldo creado no pasó la verificación: %1").arg(info.error));
        return false;
    }
    if (outPath)
        *outPath = path;
    return true;
}

BackupInfo BackupService::inspect(const QString &path)
{
    BackupInfo info;
    const QFileInfo fi(path);
    info.path = fi.absoluteFilePath();
    info.fileName = fi.fileName();
    info.kind = kindFromName(info.fileName);
    info.created = fi.lastModified();
    info.sizeBytes = fi.size();
    info.inspected = true;

    if (!fi.exists() || !fi.isFile()) {
        info.error = tr("El archivo no existe.");
        return info;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly) || f.read(16) != QByteArray("SQLite format 3\0", 16)) {
        info.error = tr("No es un respaldo de PosCuba (no es una base SQLite).");
        return info;
    }
    f.close();

    ScopedConnection conn(path, true);
    if (!conn.isOpen()) {
        info.error = tr("No se pudo abrir: %1").arg(conn.error());
        return info;
    }
    QSqlQuery q(conn.db());
    if (!q.exec(QStringLiteral("PRAGMA quick_check")) || !q.next() || q.value(0).toString() != QLatin1String("ok")) {
        info.error = tr("El archivo está dañado.");
        return info;
    }
    if (q.exec(QStringLiteral("PRAGMA user_version")) && q.next())
        info.schemaVersion = q.value(0).toInt();
    if (!q.exec(QStringLiteral("SELECT (SELECT COUNT(*) FROM products), (SELECT COUNT(*) FROM sales), "
                               "(SELECT MAX(created_at) FROM sales)"))
        || !q.next()) {
        info.error = tr("No es un respaldo de PosCuba (faltan tablas).");
        return info;
    }
    info.products = q.value(0).toInt();
    info.sales = q.value(1).toInt();
    info.lastSaleAt = q.value(2).toString();
    if (info.schemaVersion < 1) {
        info.error = tr("No es un respaldo de PosCuba (sin versión de esquema).");
        return info;
    }
    if (info.schemaVersion > Migrator::latestVersion()) {
        info.error = tr("El respaldo es de una versión más nueva del programa (esquema %1). Actualiza PosCuba.")
                         .arg(info.schemaVersion);
        return info;
    }
    info.valid = true;
    return info;
}

QVector<BackupInfo> BackupService::list(const QString &folder) const
{
    QVector<BackupInfo> out;
    const QFileInfoList files = QDir(folder).entryInfoList({kPrefix + QStringLiteral("*.db")}, QDir::Files,
                                                           QDir::Time);
    for (const QFileInfo &fi : files) {
        BackupInfo b;
        b.path = fi.absoluteFilePath();
        b.fileName = fi.fileName();
        b.kind = kindFromName(b.fileName);
        b.created = fi.lastModified();
        b.sizeBytes = fi.size();
        out.append(b);
    }
    return out;
}

int BackupService::prune(const QString &folder, int keep) const
{
    int removed = 0;
    int seen = 0;
    for (const BackupInfo &b : list(folder)) {   // mas recientes primero
        if (b.kind != kindAuto())
            continue;
        if (++seen > qMax(1, keep) && QFile::remove(b.path))
            ++removed;
    }
    return removed;
}

bool BackupService::restore(const QString &path, const QString &safetyFolder, QString *safetyPath, QString *error)
{
    const BackupInfo info = inspect(path);
    if (!info.valid) {
        setErr(error, info.error);
        return false;
    }

    // 1) Copia temporal del respaldo, migrada al esquema actual (el original no se toca).
    const QString temp = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                             .filePath(uniqueConnectionName(QStringLiteral("poscuba_restaurar_")) + QStringLiteral(".db"));
    if (!QFile::copy(path, temp)) {
        setErr(error, tr("No se pudo leer el respaldo."));
        return false;
    }
    QFile::setPermissions(temp, QFile::ReadOwner | QFile::WriteOwner);
    struct TempGuard {
        QString p;
        ~TempGuard() { QFile::remove(p); }
    } guard{temp};

    {
        ScopedConnection conn(temp, false);
        QString migrateError;
        if (!conn.isOpen() || !Migrator::run(conn.db(), &migrateError)) {
            setErr(error, tr("No se pudo preparar el respaldo: %1").arg(conn.isOpen() ? migrateError : conn.error()));
            return false;
        }
    }

    // 2) Copia de seguridad de los datos actuales.
    if (!backup(safetyFolder, kindPreRestore(), safetyPath, error))
        return false;

    // 3) Reemplazo tabla por tabla en una transaccion.
    QSqlQuery q(m_db);
    q.exec(QStringLiteral("PRAGMA foreign_keys = OFF"));
    q.prepare(QStringLiteral("ATTACH DATABASE :p AS bk"));
    q.bindValue(QStringLiteral(":p"), temp);
    if (!q.exec()) {
        q.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
        setErr(error, tr("No se pudo abrir el respaldo: %1").arg(q.lastError().text()));
        return false;
    }

    bool ok = m_db.transaction();
    QString failure = ok ? QString() : m_db.lastError().text();
    if (ok) {
        QStringList tables;
        if (q.exec(QStringLiteral("SELECT name FROM main.sqlite_master WHERE type = 'table' "
                                  "AND name NOT LIKE 'sqlite_%' ORDER BY name"))) {
            while (q.next())
                tables << q.value(0).toString();
        }
        for (const QString &t : tables) {
            const QString quoted = QLatin1Char('"') + QString(t).replace(QLatin1Char('"'), QStringLiteral("\"\""))
                                   + QLatin1Char('"');
            if (!q.exec(QStringLiteral("DELETE FROM main.%1").arg(quoted))
                || !q.exec(QStringLiteral("INSERT INTO main.%1 SELECT * FROM bk.%1").arg(quoted))) {
                ok = false;
                failure = QStringLiteral("%1: %2").arg(t, q.lastError().text());
                break;
            }
        }
        if (ok && q.exec(QStringLiteral("PRAGMA main.foreign_key_check")) && q.next()) {
            ok = false;
            failure = tr("el respaldo tiene referencias rotas (tabla %1).").arg(q.value(0).toString());
        }
        if (ok && !m_db.commit()) {
            ok = false;
            failure = m_db.lastError().text();
        }
        if (!ok)
            m_db.rollback();
    }

    q.exec(QStringLiteral("DETACH DATABASE bk"));
    q.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    if (!ok) {
        setErr(error, tr("No se pudo restaurar; los datos actuales no cambiaron. Detalle: %1").arg(failure));
        return false;
    }
    return true;
}
