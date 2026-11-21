#pragma once

#include <QDate>
#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <QVariantMap>

#include "core/Session.h"
#include "core/services/ExpenseService.h"

// Gastos del negocio (solo administradores), expuesto a QML como "expenseCtrl".
class ExpenseController : public QObject
{
    Q_OBJECT
    // month, lastMonth, last30, year, custom
    Q_PROPERTY(QString preset READ preset NOTIFY changed)
    Q_PROPERTY(QString fromText READ fromText NOTIFY changed)
    Q_PROPERTY(QString toText READ toText NOTIFY changed)
    Q_PROPERTY(int categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY changed)
    // [{ id, dateText, categoryName, description, amountText, fromCash, recurring, userName }]
    Q_PROPERTY(QVariantList expenses READ expenses NOTIFY changed)
    // [{ categoryId, name, amountText, amount, count, share }]
    Q_PROPERTY(QVariantList totals READ totals NOTIFY changed)
    Q_PROPERTY(QString totalText READ totalText NOTIFY changed)
    // [{ id, name }]
    Q_PROPERTY(QVariantList categories READ categories NOTIFY categoriesChanged)
    // [{ id, categoryId, categoryName, description, amountText, amountValue, day, active, lastText }]
    Q_PROPERTY(QVariantList recurring READ recurring NOTIFY changed)

public:
    ExpenseController(QSqlDatabase db, const Session *session, QObject *parent = nullptr);

    QString preset() const { return m_preset; }
    QString fromText() const { return m_from.toString(QStringLiteral("dd/MM/yyyy")); }
    QString toText() const { return m_to.toString(QStringLiteral("dd/MM/yyyy")); }
    int categoryFilter() const { return m_categoryFilter; }
    void setCategoryFilter(int id);
    QVariantList expenses() const { return m_expenses; }
    QVariantList totals() const { return m_totals; }
    QString totalText() const;
    QVariantList categories() const { return m_categories; }
    QVariantList recurring() const { return m_recurring; }

    Q_INVOKABLE void setPreset(const QString &key);
    // Salida: { ok, error }
    Q_INVOKABLE QVariantMap setRange(const QString &fromText, const QString &toText);
    // form: { dateText, categoryId, description, amount, fromCash }. Salida: { ok, error }
    Q_INVOKABLE QVariantMap add(const QVariantMap &form);
    Q_INVOKABLE QVariantMap remove(int id);
    // Salida: { ok, error, id }
    Q_INVOKABLE QVariantMap addCategory(const QString &name);
    // form: { id, categoryId, description, amount, day }
    Q_INVOKABLE QVariantMap saveRecurring(const QVariantMap &form);
    Q_INVOKABLE QVariantMap setRecurringActive(int id, bool active);
    Q_INVOKABLE QString todayText() const { return QDate::currentDate().toString(QStringLiteral("dd/MM/yyyy")); }
    // CSV en Documentos\PosCuba\Reportes. Salida: { ok, error, path }
    Q_INVOKABLE QVariantMap exportCsv(const QString &format);

public slots:
    // Registra los gastos fijos que tocan y relee.
    void reload();

signals:
    void changed();
    void categoriesChanged();
    // Un gasto se pago de la caja (o se borro uno de caja): la caja cambio.
    void cashChanged();
    // Cambiaron los gastos: los reportes (ganancia neta) cambian.
    void expensesChanged();

private:
    bool isAdmin() const { return m_session && m_session->isAdmin(); }
    void refresh();
    void loadCategories();

    ExpenseService m_service;
    const Session *m_session;
    QString m_preset = QStringLiteral("month");
    QDate m_from;
    QDate m_to;
    int m_categoryFilter = 0;
    qint64 m_total = 0;
    QVariantList m_expenses;
    QVariantList m_totals;
    QVariantList m_categories;
    QVariantList m_recurring;
};
