#include "ReportService.h"

#include <QCoreApplication>
#include <QMap>
#include <QSqlQuery>
#include <QVariant>

namespace {

QString tr(const char *s) { return QCoreApplication::translate("ReportService", s); }

// Limites como texto comparable con created_at ("YYYY-MM-DD HH:MM:SS").
QString lower(const QDate &d) { return d.toString(QStringLiteral("yyyy-MM-dd")) + QStringLiteral(" 00:00:00"); }
QString upper(const QDate &d) { return d.addDays(1).toString(QStringLiteral("yyyy-MM-dd")) + QStringLiteral(" 00:00:00"); }

// Costo de cada venta (subconsulta reutilizable).
const char *kSaleCost =
    "(SELECT COALESCE(SUM((i.qty_milli * i.cost_cents + 500) / 1000), 0) FROM sale_items i WHERE i.sale_id = s.id)";

void bindRange(QSqlQuery &q, const QDate &from, const QDate &to)
{
    q.bindValue(QStringLiteral(":from"), lower(from));
    q.bindValue(QStringLiteral(":to"), upper(to));
}

} // namespace

ReportService::ReportService(QSqlDatabase db)
    : m_db(db)
{
}

QDate ReportService::periodStart(const QDate &d, ReportGroup group)
{
    switch (group) {
    case ReportGroup::Week: return d.addDays(1 - d.dayOfWeek()); // lunes
    case ReportGroup::Month: return QDate(d.year(), d.month(), 1);
    default: return d;
    }
}

QString ReportService::periodLabel(const QDate &start, ReportGroup group)
{
    static const char *months[] = {QT_TRANSLATE_NOOP("ReportService", "ene"), QT_TRANSLATE_NOOP("ReportService", "feb"),
                                   QT_TRANSLATE_NOOP("ReportService", "mar"), QT_TRANSLATE_NOOP("ReportService", "abr"),
                                   QT_TRANSLATE_NOOP("ReportService", "may"), QT_TRANSLATE_NOOP("ReportService", "jun"),
                                   QT_TRANSLATE_NOOP("ReportService", "jul"), QT_TRANSLATE_NOOP("ReportService", "ago"),
                                   QT_TRANSLATE_NOOP("ReportService", "sep"), QT_TRANSLATE_NOOP("ReportService", "oct"),
                                   QT_TRANSLATE_NOOP("ReportService", "nov"), QT_TRANSLATE_NOOP("ReportService", "dic")};
    switch (group) {
    case ReportGroup::Week:
        return tr("Sem. %1").arg(start.toString(QStringLiteral("dd/MM")));
    case ReportGroup::Month:
        return QStringLiteral("%1 %2").arg(tr(months[start.month() - 1])).arg(start.year());
    default:
        return start.toString(QStringLiteral("dd/MM"));
    }
}

ReportSummary ReportService::summary(const QDate &from, const QDate &to)
{
    ReportSummary s;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT COUNT(*), COALESCE(SUM(s.total_cents), 0), COALESCE(SUM(s.discount_cents), 0), "
                             "COALESCE(SUM(%1), 0), COALESCE(SUM(s.change_cents), 0) "
                             "FROM sales s WHERE s.status = 'completed' AND s.created_at >= :from AND s.created_at < :to")
                  .arg(QString::fromLatin1(kSaleCost)));
    bindRange(q, from, to);
    qint64 change = 0;
    if (q.exec() && q.next()) {
        s.salesCount = q.value(0).toInt();
        s.total = q.value(1).toLongLong();
        s.discount = q.value(2).toLongLong();
        s.cost = q.value(3).toLongLong();
        change = q.value(4).toLongLong();
    }
    s.profit = s.total - s.cost;
    s.averageTicket = s.salesCount > 0 ? (s.total + s.salesCount / 2) / s.salesCount : 0;
    s.marginPercent = s.total > 0 ? int((s.profit * 100 + s.total / 2) / s.total) : 0;

    q.prepare(QStringLiteral("SELECT p.method, COALESCE(SUM(p.base_cents), 0) FROM payments p "
                             "JOIN sales s ON s.id = p.sale_id "
                             "WHERE s.status = 'completed' AND s.created_at >= :from AND s.created_at < :to "
                             "GROUP BY p.method"));
    bindRange(q, from, to);
    if (q.exec()) {
        while (q.next()) {
            if (q.value(0).toString() == QLatin1String("transfer"))
                s.transfer = q.value(1).toLongLong();
            else
                s.cash = q.value(1).toLongLong();
        }
    }
    s.cash -= change;
    return s;
}

QVector<PeriodRow> ReportService::byPeriod(const QDate &from, const QDate &to, ReportGroup group)
{
    // Periodos vacios del rango
    QMap<QDate, PeriodRow> rows;
    for (QDate d = periodStart(from, group); d <= to;) {
        PeriodRow r;
        r.start = d;
        r.label = periodLabel(d, group);
        rows.insert(d, r);
        d = group == ReportGroup::Day ? d.addDays(1) : (group == ReportGroup::Week ? d.addDays(7) : d.addMonths(1));
    }

    // Una fila por dia; la agrupacion por semana/mes se hace aqui (mismo calculo que periodStart).
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT date(s.created_at), COUNT(*), SUM(s.total_cents), SUM(%1), SUM(s.discount_cents) "
                             "FROM sales s WHERE s.status = 'completed' AND s.created_at >= :from AND s.created_at < :to "
                             "GROUP BY date(s.created_at)")
                  .arg(QString::fromLatin1(kSaleCost)));
    bindRange(q, from, to);
    if (q.exec()) {
        while (q.next()) {
            const QDate day = QDate::fromString(q.value(0).toString(), QStringLiteral("yyyy-MM-dd"));
            if (!day.isValid())
                continue;
            PeriodRow &r = rows[periodStart(day, group)];
            r.salesCount += q.value(1).toInt();
            r.total += q.value(2).toLongLong();
            r.cost += q.value(3).toLongLong();
            r.discount += q.value(4).toLongLong();
        }
    }

    QVector<PeriodRow> out;
    out.reserve(rows.size());
    for (PeriodRow r : rows) {
        r.profit = r.total - r.cost;
        out.append(r);
    }
    return out;
}

QVector<ProductRow> ReportService::topProducts(const QDate &from, const QDate &to, TopOrder order, int limit)
{
    QString orderBy;
    switch (order) {
    case TopOrder::Revenue: orderBy = QStringLiteral("revenue DESC"); break;
    case TopOrder::Profit: orderBy = QStringLiteral("(revenue - cost) DESC"); break;
    default: orderBy = QStringLiteral("qty DESC"); break;
    }

    QVector<ProductRow> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT i.product_id, MAX(i.product_name), COALESCE(p.unit, 'u'), SUM(i.qty_milli) AS qty, "
        "SUM(i.line_total_cents) AS revenue, SUM((i.qty_milli * i.cost_cents + 500) / 1000) AS cost "
        "FROM sale_items i JOIN sales s ON s.id = i.sale_id LEFT JOIN products p ON p.id = i.product_id "
        "WHERE s.status = 'completed' AND s.created_at >= :from AND s.created_at < :to "
        "GROUP BY i.product_id ORDER BY %1, MAX(i.product_name) LIMIT :limit").arg(orderBy));
    bindRange(q, from, to);
    q.bindValue(QStringLiteral(":limit"), limit);
    if (q.exec()) {
        while (q.next()) {
            ProductRow r;
            r.productId = q.value(0).toInt();
            r.name = q.value(1).toString();
            r.unit = q.value(2).toString();
            r.qtyMilli = q.value(3).toLongLong();
            r.revenue = q.value(4).toLongLong();
            r.cost = q.value(5).toLongLong();
            r.profit = r.revenue - r.cost;
            out.append(r);
        }
    }
    return out;
}

QVector<SaleReportRow> ReportService::sales(const QDate &from, const QDate &to)
{
    QVector<SaleReportRow> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT s.number, s.created_at, u.name, s.subtotal_cents, s.discount_cents, s.total_cents, %1, "
        "(SELECT COALESCE(SUM(base_cents), 0) FROM payments WHERE sale_id = s.id AND method = 'cash') - s.change_cents, "
        "(SELECT COALESCE(SUM(base_cents), 0) FROM payments WHERE sale_id = s.id AND method = 'transfer') "
        "FROM sales s LEFT JOIN users u ON u.id = s.user_id "
        "WHERE s.status = 'completed' AND s.created_at >= :from AND s.created_at < :to ORDER BY s.id")
                  .arg(QString::fromLatin1(kSaleCost)));
    bindRange(q, from, to);
    if (q.exec()) {
        while (q.next()) {
            SaleReportRow r;
            r.number = q.value(0).toInt();
            r.createdAt = q.value(1).toString();
            r.userName = q.value(2).toString();
            r.subtotal = q.value(3).toLongLong();
            r.discount = q.value(4).toLongLong();
            r.total = q.value(5).toLongLong();
            r.cost = q.value(6).toLongLong();
            r.cash = q.value(7).toLongLong();
            r.transfer = q.value(8).toLongLong();
            out.append(r);
        }
    }
    return out;
}
