#pragma once

#include <QDate>
#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <QVariantMap>

#include "core/Session.h"
#include "core/services/ReportService.h"
#include "data/repositories/SettingsRepository.h"

class ReportController : public QObject
{
    Q_OBJECT
    // today, yesterday, week, month, lastMonth, last30, year, custom
    Q_PROPERTY(QString preset READ preset NOTIFY changed)
    Q_PROPERTY(QString fromText READ fromText NOTIFY changed)
    Q_PROPERTY(QString toText READ toText NOTIFY changed)
    // day, week, month (lo elegido) y la agrupacion realmente usada (puede subir si el rango es largo)
    Q_PROPERTY(QString groupBy READ groupBy NOTIFY changed)
    Q_PROPERTY(QString effectiveGroup READ effectiveGroup NOTIFY changed)
    Q_PROPERTY(QString topOrder READ topOrder NOTIFY changed)       // qty, revenue, profit
    Q_PROPERTY(QString csvFormat READ csvFormat NOTIFY changed)     // es, std
    Q_PROPERTY(QVariantMap summary READ summary NOTIFY changed)
    Q_PROPERTY(QVariantList periods READ periods NOTIFY changed)
    Q_PROPERTY(double maxPeriodTotal READ maxPeriodTotal NOTIFY changed)
    Q_PROPERTY(QVariantList products READ products NOTIFY changed)
    Q_PROPERTY(QString exportFolder READ exportFolder CONSTANT)

public:
    ReportController(QSqlDatabase db, const Session *session, QObject *parent = nullptr);

    QString preset() const { return m_preset; }
    QString fromText() const;
    QString toText() const;
    QString groupBy() const { return groupKey(m_group); }
    QString effectiveGroup() const { return groupKey(m_effectiveGroup); }
    QString topOrder() const { return m_topOrder; }
    QString csvFormat() const { return m_csvFormat; }
    QVariantMap summary() const { return m_summary; }
    QVariantList periods() const { return m_periods; }
    double maxPeriodTotal() const { return m_maxPeriodTotal; }
    QVariantList products() const { return m_products; }
    QString exportFolder() const;

    Q_INVOKABLE void setPreset(const QString &key);
    // Fechas "dd/MM/yyyy". Salida: { ok, error }
    Q_INVOKABLE QVariantMap setRange(const QString &from, const QString &to);
    Q_INVOKABLE void setGroupBy(const QString &key);
    Q_INVOKABLE void setTopOrder(const QString &key);
    Q_INVOKABLE void setCsvFormat(const QString &key);

    // Salida: { ok, error, path }
    Q_INVOKABLE QVariantMap exportPeriods();
    Q_INVOKABLE QVariantMap exportProducts();
    Q_INVOKABLE QVariantMap exportSales();
    Q_INVOKABLE void openFolder() const;

public slots:
    void refresh();

signals:
    void changed();

private:
    static QString groupKey(ReportGroup g);
    QString fileName(const QString &prefix) const;
    TopOrder order() const;

    bool isAdmin() const { return m_session && m_session->isAdmin(); }

    ReportService m_service;
    const Session *m_session;
    SettingsRepository m_settings;
    QString m_preset = QStringLiteral("month");
    QDate m_from;
    QDate m_to;
    ReportGroup m_group = ReportGroup::Day;
    ReportGroup m_effectiveGroup = ReportGroup::Day;
    QString m_topOrder = QStringLiteral("qty");
    QString m_csvFormat = QStringLiteral("es");
    QVariantMap m_summary;
    QVariantList m_periods;
    double m_maxPeriodTotal = 0;
    QVariantList m_products;
};
