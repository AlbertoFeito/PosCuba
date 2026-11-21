#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <QVariantMap>

#include "core/Session.h"
#include "core/services/CustomerService.h"

// Clientes y fiado, expuesto a QML como "customerCtrl".
// Cajeros: ver, crear clientes y registrar abonos. Solo un administrador cambia el limite o desactiva.
class CustomerController : public QObject
{
    Q_OBJECT
    // [{ id, name, phone, note, balance, balanceText, limitText, hasLimit, active, overLimit, lastText }]
    Q_PROPERTY(QVariantList customers READ customers NOTIFY changed)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY changed)
    Q_PROPERTY(bool onlyDebt READ onlyDebt WRITE setOnlyDebt NOTIFY changed)
    Q_PROPERTY(bool includeInactive READ includeInactive WRITE setIncludeInactive NOTIFY changed)
    Q_PROPERTY(QString receivablesText READ receivablesText NOTIFY changed)
    Q_PROPERTY(int debtorsCount READ debtorsCount NOTIFY changed)

public:
    CustomerController(QSqlDatabase db, const Session *session, QObject *parent = nullptr);

    QVariantList customers() const { return m_list; }
    QString search() const { return m_search; }
    void setSearch(const QString &s);
    bool onlyDebt() const { return m_onlyDebt; }
    void setOnlyDebt(bool on);
    bool includeInactive() const { return m_includeInactive; }
    void setIncludeInactive(bool on);
    QString receivablesText() const;
    int debtorsCount() const { return m_debtors; }

    Q_INVOKABLE QVariantMap get(int id);
    // form: { id, name, phone, note, limit }. Salida: { ok, error, id }
    Q_INVOKABLE QVariantMap save(const QVariantMap &form);
    Q_INVOKABLE QVariantMap setActive(int id, bool active);
    // [{ dateText, kindText, kind, amountText, detail, balanceText }]
    Q_INVOKABLE QVariantList entries(int id);
    // Salida: { ok, error, balanceText }
    Q_INVOKABLE QVariantMap registerPayment(int id, const QString &amountText, const QString &method, const QString &note);
    // Para elegir cliente al cobrar: activos que coinciden. [{ id, name, phone, balanceText, limitText }]
    Q_INVOKABLE QVariantList pick(const QString &text);
    // Cuentas por cobrar en CSV (Documentos\PosCuba\Reportes). format "es" | "std". Salida: { ok, error, path }
    Q_INVOKABLE QVariantMap exportReceivables(const QString &format);

public slots:
    void reload();

signals:
    void changed();
    void paymentRegistered();

private:
    bool isAdmin() const { return m_session && m_session->isAdmin(); }
    QVariantMap toMap(const Customer &c) const;

    CustomerService m_service;
    const Session *m_session;
    QVariantList m_list;
    QString m_search;
    bool m_onlyDebt = false;
    bool m_includeInactive = false;
    qint64 m_receivables = 0;
    int m_debtors = 0;
};
