#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <QVariantMap>

#include "core/Session.h"
#include "core/services/CurrencyService.h"

class CurrencyController : public QObject
{
    Q_OBJECT
    // [{ code, name, symbol, rateText, isBase, active }]
    Q_PROPERTY(QVariantList currencies READ currencies NOTIFY changed)
    Q_PROPERTY(QString baseCode READ baseCode NOTIFY changed)
    Q_PROPERTY(bool canChangeBase READ canChangeBase NOTIFY changed)

public:
    CurrencyController(QSqlDatabase db, const Session *session, QObject *parent = nullptr);

    QVariantList currencies() const { return m_list; }
    QString baseCode() const { return m_baseCode; }
    bool canChangeBase() const { return m_canChangeBase; }

    // form: code, name, symbol, rate (texto), active. Salida: { ok, error }
    Q_INVOKABLE QVariantMap add(const QVariantMap &form);
    Q_INVOKABLE QVariantMap update(const QVariantMap &form);
    Q_INVOKABLE QVariantMap setBase(const QString &code);

public slots:
    void refresh();

signals:
    void changed();
    void currenciesEdited();   // las demas pantallas deben releer monedas y tasas

private:
    bool fromForm(const QVariantMap &form, Currency *c, QString *error) const;
    bool isAdmin() const { return m_session && m_session->isAdmin(); }

    CurrencyService m_service;
    const Session *m_session;
    QVariantList m_list;
    QString m_baseCode;
    bool m_canChangeBase = false;
};
