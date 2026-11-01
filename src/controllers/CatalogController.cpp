#include "CatalogController.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QUrl>

#include <algorithm>

#include "core/CsvReader.h"
#include "core/Money.h"

namespace {
QVariantMap result(bool ok, const QString &error)
{
    return QVariantMap{{QStringLiteral("ok"), ok}, {QStringLiteral("error"), error}};
}

CsvWriter::Format formatOf(const QString &f)
{
    return f == QLatin1String("std") ? CsvWriter::Standard : CsvWriter::Spanish;
}

bool skipMount(const QStorageInfo &s)
{
#ifdef Q_OS_WIN
    Q_UNUSED(s)
    return false;
#else
    const QString root = s.rootPath();
    return root == QLatin1String("/") || root.startsWith(QLatin1String("/proc")) || root.startsWith(QLatin1String("/sys"))
           || root.startsWith(QLatin1String("/run")) || root.startsWith(QLatin1String("/dev"))
           || root.startsWith(QLatin1String("/snap")) || root.startsWith(QLatin1String("/boot"))
           || s.fileSystemType() == "tmpfs" || s.fileSystemType() == "overlay";
#endif
}
} // namespace

CatalogController::CatalogController(QSqlDatabase db, const Session *session, QObject *parent)
    : QObject(parent)
    , m_pricing(db)
    , m_import(db)
    , m_session(session)
{
}

QString CatalogController::folder() const
{
    const QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return QDir::toNativeSeparators(QDir(docs).filePath(QStringLiteral("PosCuba/Productos")));
}

bool CatalogController::readRequest(const QVariantMap &form, PriceChangeRequest *req, QString *error) const
{
    req->categoryId = form.value(QStringLiteral("categoryId")).toInt();
    req->includeInactive = form.value(QStringLiteral("includeInactive")).toBool();
    req->mode = form.value(QStringLiteral("mode")).toString() == QLatin1String("margin")
                    ? PriceChangeRequest::MarginOnCost : PriceChangeRequest::PercentOfPrice;
    req->stepCents = form.value(QStringLiteral("step")).toLongLong();
    req->rounding = form.value(QStringLiteral("rounding")).toString() == QLatin1String("nearest")
                        ? PriceChangeRequest::RoundNearest : PriceChangeRequest::RoundUp;
    if (!PricingService::parsePercent(form.value(QStringLiteral("percent")).toString(), &req->basisPoints)) {
        *error = tr("Porcentaje no válido. Ejemplos: 10, -5, 12,5 (entre -99,99 y 1000).");
        return false;
    }
    if (req->mode == PriceChangeRequest::PercentOfPrice && req->basisPoints == 0 && req->stepCents == 0) {
        *error = tr("Escribe el porcentaje del cambio (por ejemplo 10 para subir un 10 %).");
        return false;
    }
    return true;
}

QVariantMap CatalogController::pricePreview(const QVariantMap &form)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede cambiar precios."));
    PriceChangeRequest req;
    QString error;
    if (!readRequest(form, &req, &error))
        return result(false, error);
    const PricePreview pv = m_pricing.preview(req);
    QVariantList rows;
    for (const PriceChange &c : pv.changes) {
        if (rows.size() >= 500)
            break;
        const qint64 d = c.newPrice - c.oldPrice;
        rows.append(QVariantMap{
            {QStringLiteral("code"), c.code},
            {QStringLiteral("name"), c.name},
            {QStringLiteral("oldText"), Money::formatCents(c.oldPrice)},
            {QStringLiteral("newText"), Money::formatCents(c.newPrice)},
            {QStringLiteral("diffText"), (d > 0 ? QStringLiteral("+") : QStringLiteral("−")) + Money::formatCents(qAbs(d))},
            {QStringLiteral("up"), d > 0},
            {QStringLiteral("belowCost"), c.costCents > 0 && c.newPrice < c.costCents},
        });
    }
    QVariantMap r = result(true, QString());
    r.insert(QStringLiteral("count"), pv.changes.size());
    r.insert(QStringLiteral("unchanged"), pv.unchanged);
    r.insert(QStringLiteral("skipped"), pv.skippedNoCost);
    r.insert(QStringLiteral("rows"), rows);
    return r;
}

QVariantMap CatalogController::applyPrices(const QVariantMap &form)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede cambiar precios."));
    PriceChangeRequest req;
    QString error;
    if (!readRequest(form, &req, &error))
        return result(false, error);
    const int n = m_pricing.apply(req, &error);
    if (n < 0)
        return result(false, error);
    if (n > 0)
        emit catalogChanged();
    QVariantMap r = result(true, QString());
    r.insert(QStringLiteral("count"), n);
    return r;
}

QVariantMap CatalogController::exportProducts(const QString &format, bool includeInactive)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede exportar productos."));
    const QString path = QDir(folder()).filePath(
        QStringLiteral("productos_%1.csv").arg(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"))));
    QString error;
    int count = 0;
    if (!QDir().mkpath(folder()) || !m_import.exportTo(path, formatOf(format), includeInactive, &count, &error))
        return result(false, error.isEmpty() ? tr("No se pudo crear la carpeta %1.").arg(folder()) : error);
    QVariantMap r = result(true, QString());
    r.insert(QStringLiteral("path"), QDir::toNativeSeparators(path));
    r.insert(QStringLiteral("count"), count);
    return r;
}

QVariantMap CatalogController::exportTemplate(const QString &format)
{
    const QString path = QDir(folder()).filePath(QStringLiteral("plantilla_productos.csv"));
    QString error;
    if (!QDir().mkpath(folder()) || !ProductImportService::writeTemplate(path, formatOf(format), &error))
        return result(false, error.isEmpty() ? tr("No se pudo crear la carpeta %1.").arg(folder()) : error);
    QVariantMap r = result(true, QString());
    r.insert(QStringLiteral("path"), QDir::toNativeSeparators(path));
    return r;
}

QVariantList CatalogController::importCandidates() const
{
    QStringList folders;
    const QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    folders << QDir(docs).filePath(QStringLiteral("PosCuba/Productos")) << docs
            << QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)
            << QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    const QString home = QDir::homePath();
    for (const QStorageInfo &s : QStorageInfo::mountedVolumes()) {
        if (!s.isValid() || !s.isReady() || skipMount(s) || home.startsWith(s.rootPath()))
            continue;
        folders << s.rootPath();
        const QStringList subs = QDir(s.rootPath()).entryList({QStringLiteral("PosCuba*")}, QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &sub : subs)
            folders << QDir(s.rootPath()).filePath(sub);
    }

    QFileInfoList found;
    QStringList seen;
    for (const QString &f : folders) {
        if (f.isEmpty())
            continue;
        const QString canon = QDir(f).canonicalPath();
        if (canon.isEmpty() || seen.contains(canon))
            continue;
        seen << canon;
        found << QDir(f).entryInfoList({QStringLiteral("*.csv"), QStringLiteral("*.CSV")}, QDir::Files, QDir::Time);
    }
    std::sort(found.begin(), found.end(), [](const QFileInfo &a, const QFileInfo &b) {
        return a.lastModified() > b.lastModified();
    });
    QVariantList out;
    for (const QFileInfo &fi : found) {
        if (out.size() >= 60)
            break;
        out.append(QVariantMap{
            {QStringLiteral("path"), QDir::toNativeSeparators(fi.absoluteFilePath())},
            {QStringLiteral("name"), fi.fileName()},
            {QStringLiteral("folder"), QDir::toNativeSeparators(fi.absolutePath())},
            {QStringLiteral("dateText"), fi.lastModified().toString(QStringLiteral("dd/MM/yyyy HH:mm"))},
        });
    }
    return out;
}

QString CatalogController::localPath(const QString &pathOrUrl)
{
    QString p = pathOrUrl.trimmed();
    if (p.startsWith(QLatin1Char('"')) && p.endsWith(QLatin1Char('"')) && p.size() > 1)
        p = p.mid(1, p.size() - 2);
    if (p.startsWith(QLatin1String("file:")))
        p = QUrl(p).toLocalFile();
    return QDir::fromNativeSeparators(p);
}

bool CatalogController::readFile(const QString &path, QString *text, QString *error)
{
    QFile f(path);
    if (path.isEmpty() || !f.exists()) {
        *error = tr("No existe el archivo «%1».").arg(QDir::toNativeSeparators(path));
        return false;
    }
    if (f.size() > 20 * 1024 * 1024) {
        *error = tr("El archivo es demasiado grande.");
        return false;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        *error = tr("No se pudo abrir «%1». ¿Está abierto en Excel?").arg(QDir::toNativeSeparators(path));
        return false;
    }
    *text = CsvReader::decode(f.readAll());
    return true;
}

QVariantMap CatalogController::importPlan(const QString &pathOrUrl, bool updateStock)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede importar productos."));
    QString text, error;
    if (!readFile(localPath(pathOrUrl), &text, &error))
        return result(false, error);
    const ImportPlan pl = m_import.plan(text, updateStock);
    if (!pl.usable())
        return result(false, pl.error);

    QVariantList rows;
    for (const ImportRow &r : pl.rows) {
        QString status;
        QString key;
        switch (r.status) {
        case ImportRow::New: status = tr("Nuevo"); key = QStringLiteral("new"); break;
        case ImportRow::Update: status = tr("Cambia"); key = QStringLiteral("update"); break;
        case ImportRow::Unchanged: status = tr("Igual"); key = QStringLiteral("same"); break;
        default: status = tr("Error"); key = QStringLiteral("error"); break;
        }
        rows.append(QVariantMap{
            {QStringLiteral("line"), r.line},
            {QStringLiteral("status"), key},
            {QStringLiteral("statusText"), status},
            {QStringLiteral("code"), r.autoCode ? tr("(auto)") : r.product.code},
            {QStringLiteral("name"), r.product.name},
            {QStringLiteral("detail"), r.status == ImportRow::Error ? r.error : r.changes.join(QStringLiteral(" · "))},
        });
    }
    QVariantMap out = result(true, QString());
    out.insert(QStringLiteral("newCount"), pl.newCount);
    out.insert(QStringLiteral("updateCount"), pl.updateCount);
    out.insert(QStringLiteral("unchangedCount"), pl.unchangedCount);
    out.insert(QStringLiteral("errorCount"), pl.errorCount);
    out.insert(QStringLiteral("columns"), pl.columns.join(QStringLiteral(", ")));
    out.insert(QStringLiteral("ignored"), pl.ignored.join(QStringLiteral(", ")));
    out.insert(QStringLiteral("rows"), rows);
    return out;
}

QVariantMap CatalogController::importApply(const QString &pathOrUrl, bool updateStock)
{
    if (!isAdmin() || !m_session)
        return result(false, tr("Solo un administrador puede importar productos."));
    QString text, error;
    if (!readFile(localPath(pathOrUrl), &text, &error))
        return result(false, error);
    const ImportResult r = m_import.apply(text, updateStock, m_session->userId);
    if (!r.ok)
        return result(false, r.error);
    if (r.created + r.updated > 0)
        emit catalogChanged();
    QVariantMap out = result(true, QString());
    out.insert(QStringLiteral("created"), r.created);
    out.insert(QStringLiteral("updated"), r.updated);
    out.insert(QStringLiteral("categories"), r.categoriesCreated);
    return out;
}

void CatalogController::openFolder() const
{
    QDir().mkpath(folder());
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder()));
}
