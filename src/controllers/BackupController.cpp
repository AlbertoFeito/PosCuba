#include "BackupController.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QStorageInfo>
#include <QUrl>

namespace {
QVariantMap result(bool ok, const QString &error, const QString &pathKey = QString(), const QString &path = QString())
{
    QVariantMap r{{QStringLiteral("ok"), ok}, {QStringLiteral("error"), error}};
    if (!pathKey.isEmpty())
        r.insert(pathKey, QDir::toNativeSeparators(path));
    return r;
}

const QString kSubfolder = QStringLiteral("PosCuba_Respaldos");

QString shortDate(const QDateTime &dt)
{
    return dt.isValid() ? dt.toString(QStringLiteral("dd/MM/yyyy HH:mm")) : QString();
}

bool isSystemMount(const QStorageInfo &s)
{
    const QString root = s.rootPath();
#ifdef Q_OS_WIN
    Q_UNUSED(root)
    return false;
#else
    static const QStringList skipFs = {QStringLiteral("tmpfs"), QStringLiteral("devtmpfs"), QStringLiteral("proc"),
                                       QStringLiteral("sysfs"), QStringLiteral("overlay"), QStringLiteral("squashfs"),
                                       QStringLiteral("cgroup"), QStringLiteral("cgroup2")};
    return skipFs.contains(QString::fromLatin1(s.fileSystemType())) || root.startsWith(QLatin1String("/proc"))
           || root.startsWith(QLatin1String("/sys")) || root.startsWith(QLatin1String("/run"))
           || root.startsWith(QLatin1String("/dev")) || root.startsWith(QLatin1String("/snap"))
           || root.startsWith(QLatin1String("/boot"));
#endif
}
} // namespace

BackupController::BackupController(QSqlDatabase db, const Session *session, QObject *parent)
    : QObject(parent)
    , m_service(db)
    , m_settings(db)
    , m_session(session)
    , m_folder(BackupService::defaultFolder())
{
}

bool BackupController::usingDefault() const
{
    return QDir::cleanPath(m_folder) == QDir::cleanPath(BackupService::defaultFolder());
}

bool BackupController::autoEnabled() const
{
    return m_settings.value(QStringLiteral("auto_backup"), QStringLiteral("1")) == QLatin1String("1");
}

int BackupController::keepCount() const
{
    const int n = m_settings.value(QStringLiteral("backup_keep"), QStringLiteral("30")).toInt();
    return n > 0 ? n : 30;
}

QString BackupController::lastAutoText() const
{
    const QDateTime last = QDateTime::fromString(m_settings.value(QStringLiteral("last_auto_backup")), Qt::ISODate);
    return last.isValid() ? shortDate(last) : tr("nunca");
}

QString BackupController::sizeText(qint64 bytes)
{
    if (bytes >= 1024LL * 1024 * 1024)
        return QStringLiteral("%1 GB").arg(double(bytes) / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
    if (bytes >= 1024 * 1024)
        return QStringLiteral("%1 MB").arg(double(bytes) / (1024.0 * 1024.0), 0, 'f', 1);
    return QStringLiteral("%1 KB").arg(qMax<qint64>(1, (bytes + 1023) / 1024));
}

QString BackupController::kindText(const QString &kind)
{
    if (kind == BackupService::kindAuto())
        return tr("Automático");
    if (kind == BackupService::kindPreRestore())
        return tr("Antes de restaurar");
    return tr("Manual");
}

void BackupController::refreshDrives()
{
    m_drives.clear();
    const QString systemRoot = QStorageInfo::root().rootPath();
    for (const QStorageInfo &s : QStorageInfo::mountedVolumes()) {
        if (!s.isValid() || !s.isReady() || s.isReadOnly() || s.bytesTotal() <= 0 || isSystemMount(s))
            continue;
        const bool system = s.rootPath() == systemRoot;
        QString label = s.displayName();
        if (label.isEmpty() || label == s.rootPath())
            label = QDir::toNativeSeparators(s.rootPath());
        else
            label = QStringLiteral("%1 (%2)").arg(label, QDir::toNativeSeparators(s.rootPath()));
        m_drives.append(QVariantMap{
            {QStringLiteral("root"), s.rootPath()},
            {QStringLiteral("folder"), QDir::toNativeSeparators(QDir(s.rootPath()).filePath(kSubfolder))},
            {QStringLiteral("label"), label},
            {QStringLiteral("freeText"), tr("%1 libres").arg(sizeText(s.bytesAvailable()))},
            {QStringLiteral("system"), system},
        });
    }
}

void BackupController::refresh()
{
    refreshDrives();
    m_backups.clear();
    for (const BackupInfo &b : m_service.list(m_folder)) {
        m_backups.append(QVariantMap{
            {QStringLiteral("path"), b.path},
            {QStringLiteral("fileName"), b.fileName},
            {QStringLiteral("kind"), b.kind},
            {QStringLiteral("kindText"), kindText(b.kind)},
            {QStringLiteral("dateText"), shortDate(b.created)},
            {QStringLiteral("sizeText"), sizeText(b.sizeBytes)},
        });
    }
    emit changed();
}

void BackupController::useDefaultFolder()
{
    m_folder = BackupService::defaultFolder();
    refresh();
}

void BackupController::useDrive(const QString &root)
{
    m_folder = QDir(root).filePath(kSubfolder);
    refresh();
}

QVariantMap BackupController::useFolder(const QString &path)
{
    const QString p = QDir::fromNativeSeparators(path.trimmed());
    if (p.isEmpty())
        return result(false, tr("Escribe una carpeta."));
    m_folder = QDir::cleanPath(p);
    refresh();
    return result(true, QString());
}

QVariantMap BackupController::backupNow()
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede hacer respaldos."));
    QString path, error;
    if (!m_service.backup(m_folder, BackupService::kindManual(), &path, &error))
        return result(false, error);
    refresh();
    return result(true, QString(), QStringLiteral("path"), path);
}

QVariantMap BackupController::inspect(const QString &path)
{
    const BackupInfo info = m_service.inspect(path);
    const QDateTime last = QDateTime::fromString(info.lastSaleAt, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    return {
        {QStringLiteral("valid"), info.valid},
        {QStringLiteral("error"), info.error},
        {QStringLiteral("dateText"), shortDate(info.created)},
        {QStringLiteral("sizeText"), sizeText(info.sizeBytes)},
        {QStringLiteral("products"), info.products},
        {QStringLiteral("sales"), info.sales},
        {QStringLiteral("lastSaleText"), last.isValid() ? shortDate(last) : tr("sin ventas")},
        {QStringLiteral("schemaVersion"), info.schemaVersion},
    };
}

QVariantMap BackupController::restore(const QString &path)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede restaurar."));
    QString safety, error;
    if (!m_service.restore(path, BackupService::defaultFolder(), &safety, &error))
        return result(false, error);
    refresh();
    emit dataRestored();
    return result(true, QString(), QStringLiteral("safetyPath"), safety);
}

QVariantMap BackupController::saveAutoSettings(bool enabled, int keep)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede cambiar esto."));
    if (keep < 1 || keep > 365)
        return result(false, tr("Conservar entre 1 y 365 respaldos automáticos."));
    m_settings.setValue(QStringLiteral("auto_backup"), enabled ? QStringLiteral("1") : QStringLiteral("0"));
    m_settings.setValue(QStringLiteral("backup_keep"), QString::number(keep));
    m_service.prune(BackupService::defaultFolder(), keep);
    refresh();
    return result(true, QString());
}

void BackupController::autoBackupIfDue()
{
    const QDateTime last = QDateTime::fromString(m_settings.value(QStringLiteral("last_auto_backup")), Qt::ISODate);
    if (autoEnabled() && BackupService::isAutoDue(last, QDateTime::currentDateTime()))
        autoBackupNow();
}

void BackupController::autoBackupNow()
{
    if (!autoEnabled())
        return;
    QString path, error;
    if (m_service.backup(BackupService::defaultFolder(), BackupService::kindAuto(), &path, &error)) {
        m_settings.setValue(QStringLiteral("last_auto_backup"), QDateTime::currentDateTime().toString(Qt::ISODate));
        m_service.prune(BackupService::defaultFolder(), keepCount());
    }
    refresh();
}

void BackupController::openFolder() const
{
    QDir().mkpath(m_folder);
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_folder));
}
