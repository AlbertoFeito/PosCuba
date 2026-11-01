#include "ReportController.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>

#include "core/CsvWriter.h"
#include "core/Money.h"

namespace {
QVariantMap result(bool ok, const QString &error, const QString &path = QString())
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("error"), error}, {QStringLiteral("path"), path}};
}
const QString kDateFormat = QStringLiteral("dd/MM/yyyy");
} // namespace

ReportController::ReportController(QSqlDatabase db, const Session *session, QObject *parent)
    : QObject(parent)
    , m_service(db)
    , m_session(session)
    , m_settings(db)
{
    const QDate today = QDate::currentDate();
    m_from = QDate(today.year(), today.month(), 1);
    m_to = today;
}

QString ReportController::fromText() const { return m_from.toString(kDateFormat); }
QString ReportController::toText() const { return m_to.toString(kDateFormat); }

QString ReportController::groupKey(ReportGroup g)
{
    switch (g) {
    case ReportGroup::Week: return QStringLiteral("week");
    case ReportGroup::Month: return QStringLiteral("month");
    default: return QStringLiteral("day");
    }
}

TopOrder ReportController::order() const
{
    if (m_topOrder == QLatin1String("revenue"))
        return TopOrder::Revenue;
    if (m_topOrder == QLatin1String("profit"))
        return TopOrder::Profit;
    return TopOrder::Quantity;
}

QString ReportController::exportFolder() const
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath(QStringLiteral("PosCuba/Reportes"));
}

void ReportController::setPreset(const QString &key)
{
    const QDate today = QDate::currentDate();
    QDate from = today, to = today;
    ReportGroup group = ReportGroup::Day;
    if (key == QLatin1String("yesterday")) {
        from = to = today.addDays(-1);
    } else if (key == QLatin1String("week")) {
        from = ReportService::periodStart(today, ReportGroup::Week);
    } else if (key == QLatin1String("month")) {
        from = QDate(today.year(), today.month(), 1);
    } else if (key == QLatin1String("lastMonth")) {
        const QDate first = QDate(today.year(), today.month(), 1);
        from = first.addMonths(-1);
        to = first.addDays(-1);
    } else if (key == QLatin1String("last30")) {
        from = today.addDays(-29);
    } else if (key == QLatin1String("year")) {
        from = QDate(today.year(), 1, 1);
        group = ReportGroup::Month;
    } else if (key != QLatin1String("today")) {
        return;
    }
    m_preset = key;
    m_from = from;
    m_to = to;
    m_group = group;
    refresh();
}

QVariantMap ReportController::setRange(const QString &fromText, const QString &toText)
{
    const QDate from = QDate::fromString(fromText.trimmed(), kDateFormat);
    const QDate to = QDate::fromString(toText.trimmed(), kDateFormat);
    if (!from.isValid() || !to.isValid())
        return result(false, tr("Fecha no válida. Usa el formato dd/mm/aaaa."));
    if (from > to)
        return result(false, tr("La fecha inicial es posterior a la final."));
    if (from.daysTo(to) > 3660)
        return result(false, tr("El rango no puede superar 10 años."));
    m_preset = QStringLiteral("custom");
    m_from = from;
    m_to = to;
    refresh();
    return result(true, QString());
}

void ReportController::setGroupBy(const QString &key)
{
    m_group = key == QLatin1String("week") ? ReportGroup::Week
              : key == QLatin1String("month") ? ReportGroup::Month : ReportGroup::Day;
    refresh();
}

void ReportController::setTopOrder(const QString &key)
{
    if (key != QLatin1String("qty") && key != QLatin1String("revenue") && key != QLatin1String("profit"))
        return;
    m_topOrder = key;
    refresh();
}

void ReportController::setCsvFormat(const QString &key)
{
    m_csvFormat = key == QLatin1String("std") ? QStringLiteral("std") : QStringLiteral("es");
    m_settings.setValue(QStringLiteral("report_csv_format"), m_csvFormat);
    emit changed();
}

void ReportController::refresh()
{
    if (!isAdmin()) {
        m_summary.clear();
        m_periods.clear();
        m_products.clear();
        m_maxPeriodTotal = 0;
        emit changed();
        return;
    }
    m_csvFormat = m_settings.value(QStringLiteral("report_csv_format"), QStringLiteral("es")) == QLatin1String("std")
                      ? QStringLiteral("std") : QStringLiteral("es");

    // Evita graficos ilegibles: maximo ~62 dias o ~104 semanas por barra.
    const qint64 days = m_from.daysTo(m_to) + 1;
    m_effectiveGroup = m_group;
    if (m_effectiveGroup == ReportGroup::Day && days > 62)
        m_effectiveGroup = ReportGroup::Week;
    if (m_effectiveGroup == ReportGroup::Week && days > 728)
        m_effectiveGroup = ReportGroup::Month;

    const ReportSummary s = m_service.summary(m_from, m_to);
    m_summary = {
        {QStringLiteral("salesCount"), s.salesCount},
        {QStringLiteral("totalText"), Money::formatCents(s.total)},
        {QStringLiteral("costText"), Money::formatCents(s.cost)},
        {QStringLiteral("profitText"), Money::formatCents(s.profit)},
        {QStringLiteral("marginPercent"), s.marginPercent},
        {QStringLiteral("discountText"), Money::formatCents(s.discount)},
        {QStringLiteral("cashText"), Money::formatCents(s.cash)},
        {QStringLiteral("transferText"), Money::formatCents(s.transfer)},
        {QStringLiteral("averageText"), Money::formatCents(s.averageTicket)},
        {QStringLiteral("days"), int(days)},
    };

    m_periods.clear();
    m_maxPeriodTotal = 0;
    for (const PeriodRow &r : m_service.byPeriod(m_from, m_to, m_effectiveGroup)) {
        m_maxPeriodTotal = qMax(m_maxPeriodTotal, double(r.total));
        m_periods.append(QVariantMap{
            {QStringLiteral("label"), r.label},
            {QStringLiteral("salesCount"), r.salesCount},
            {QStringLiteral("total"), double(r.total)},
            {QStringLiteral("totalText"), Money::formatCents(r.total)},
            {QStringLiteral("profitText"), Money::formatCents(r.profit)},
            {QStringLiteral("profitNegative"), r.profit < 0},
        });
    }

    m_products.clear();
    for (const ProductRow &r : m_service.topProducts(m_from, m_to, order(), 50)) {
        m_products.append(QVariantMap{
            {QStringLiteral("name"), r.name},
            {QStringLiteral("qtyText"), Money::formatMilli(r.qtyMilli)},
            {QStringLiteral("unit"), r.unit},
            {QStringLiteral("revenueText"), Money::formatCents(r.revenue)},
            {QStringLiteral("profitText"), Money::formatCents(r.profit)},
            {QStringLiteral("profitNegative"), r.profit < 0},
        });
    }
    emit changed();
}

QString ReportController::fileName(const QString &prefix) const
{
    return QDir(exportFolder()).filePath(QStringLiteral("%1_%2_a_%3.csv")
                                             .arg(prefix, m_from.toString(QStringLiteral("yyyy-MM-dd")),
                                                  m_to.toString(QStringLiteral("yyyy-MM-dd"))));
}

QVariantMap ReportController::exportPeriods()
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede hacer esto."));
    const CsvWriter csv(m_csvFormat == QLatin1String("std") ? CsvWriter::Standard : CsvWriter::Spanish);
    QVector<QStringList> rows;
    for (const PeriodRow &r : m_service.byPeriod(m_from, m_to, m_effectiveGroup)) {
        rows.append({r.start.toString(QStringLiteral("yyyy-MM-dd")), r.label, QString::number(r.salesCount),
                     csv.number(Money::formatCents(r.total)), csv.number(Money::formatCents(r.discount)),
                     csv.number(Money::formatCents(r.cost)), csv.number(Money::formatCents(r.profit))});
    }
    const QString prefix = m_effectiveGroup == ReportGroup::Week ? QStringLiteral("ventas_por_semana")
                           : m_effectiveGroup == ReportGroup::Month ? QStringLiteral("ventas_por_mes")
                                                                    : QStringLiteral("ventas_por_dia");
    const QString path = fileName(prefix);
    QString error;
    if (!csv.write(path, {tr("Inicio"), tr("Período"), tr("Ventas"), tr("Total"), tr("Descuentos"), tr("Costo"),
                          tr("Ganancia")}, rows, &error))
        return result(false, error);
    return result(true, QString(), QDir::toNativeSeparators(path));
}

QVariantMap ReportController::exportProducts()
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede hacer esto."));
    const CsvWriter csv(m_csvFormat == QLatin1String("std") ? CsvWriter::Standard : CsvWriter::Spanish);
    QVector<QStringList> rows;
    for (const ProductRow &r : m_service.topProducts(m_from, m_to, order(), 100000)) {
        rows.append({r.name, csv.number(Money::formatMilli(r.qtyMilli)), r.unit,
                     csv.number(Money::formatCents(r.revenue)), csv.number(Money::formatCents(r.cost)),
                     csv.number(Money::formatCents(r.profit))});
    }
    const QString path = fileName(QStringLiteral("productos_vendidos"));
    QString error;
    if (!csv.write(path, {tr("Producto"), tr("Cantidad"), tr("Unidad"), tr("Importe"), tr("Costo"), tr("Ganancia")},
                   rows, &error))
        return result(false, error);
    return result(true, QString(), QDir::toNativeSeparators(path));
}

QVariantMap ReportController::exportSales()
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede hacer esto."));
    const CsvWriter csv(m_csvFormat == QLatin1String("std") ? CsvWriter::Standard : CsvWriter::Spanish);
    QVector<QStringList> rows;
    for (const SaleReportRow &r : m_service.sales(m_from, m_to)) {
        const QDateTime dt = QDateTime::fromString(r.createdAt, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        rows.append({QString::number(r.number),
                     dt.isValid() ? dt.toString(QStringLiteral("dd/MM/yyyy HH:mm")) : r.createdAt, r.userName,
                     csv.number(Money::formatCents(r.subtotal)), csv.number(Money::formatCents(r.discount)),
                     csv.number(Money::formatCents(r.total)), csv.number(Money::formatCents(r.cost)),
                     csv.number(Money::formatCents(r.total - r.cost)), csv.number(Money::formatCents(r.cash)),
                     csv.number(Money::formatCents(r.transfer))});
    }
    const QString path = fileName(QStringLiteral("detalle_ventas"));
    QString error;
    if (!csv.write(path, {tr("Venta"), tr("Fecha"), tr("Cajero"), tr("Subtotal"), tr("Descuento"), tr("Total"),
                          tr("Costo"), tr("Ganancia"), tr("Efectivo"), tr("Transferencia")}, rows, &error))
        return result(false, error);
    return result(true, QString(), QDir::toNativeSeparators(path));
}

void ReportController::openFolder() const
{
    QDir().mkpath(exportFolder());
    QDesktopServices::openUrl(QUrl::fromLocalFile(exportFolder()));
}
