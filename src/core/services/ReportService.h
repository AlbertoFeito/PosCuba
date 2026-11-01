#pragma once

#include <QDate>
#include <QSqlDatabase>
#include <QVector>

#include "core/Report.h"

// Consultas de reportes sobre ventas completadas. Rango de fechas inclusivo [from, to].
class ReportService
{
public:
    explicit ReportService(QSqlDatabase db);

    ReportSummary summary(const QDate &from, const QDate &to);
    // Incluye periodos sin ventas (en cero) para que el grafico sea continuo.
    QVector<PeriodRow> byPeriod(const QDate &from, const QDate &to, ReportGroup group);
    QVector<ProductRow> topProducts(const QDate &from, const QDate &to, TopOrder order, int limit);
    QVector<SaleReportRow> sales(const QDate &from, const QDate &to);

    static QDate periodStart(const QDate &d, ReportGroup group);
    static QString periodLabel(const QDate &start, ReportGroup group);

private:
    QSqlDatabase m_db;
};
