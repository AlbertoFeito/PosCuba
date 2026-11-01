#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <QVariantMap>

#include "core/Session.h"
#include "core/services/CashService.h"

class CashController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isOpen READ isOpen NOTIFY changed)
    // Resumen de la caja abierta: openedAt, openedBy, openingText, salesCount, salesTotalText,
    // cashSalesText, transferText, movementsInText, movementsOutText, profitText,
    // expected: [{ currency, expectedText, isBase }]
    Q_PROPERTY(QVariantMap summary READ summary NOTIFY changed)
    // Ultimas cajas: [{ id, openedAt, closedAt, openedBy, expectedText, countedText, differenceText,
    //                   differenceCents, status }]
    Q_PROPERTY(QVariantList history READ history NOTIFY changed)

public:
    CashController(QSqlDatabase db, const Session *session, QObject *parent = nullptr);

    bool isOpen() const { return m_isOpen; }
    QVariantMap summary() const { return m_summary; }
    QVariantList history() const { return m_history; }

    // Salida de todas: { ok, error, ... }
    Q_INVOKABLE QVariantMap open(const QString &openingText);
    Q_INVOKABLE QVariantMap addMovement(const QString &amountText, bool withdrawal, const QString &reason);
    // counted: { "CUP": "1250.00", "USD": "20" }. Salida extra: expectedText, countedText, differenceText
    Q_INVOKABLE QVariantMap close(const QVariantMap &counted);
    // Diferencia contada - esperada para una moneda (para mostrar mientras se escribe).
    Q_INVOKABLE QVariantMap previewDifference(const QString &currency, const QString &countedText) const;

public slots:
    void refresh();

signals:
    void changed();
    void sessionOpenedOrClosed();
    void sessionClosed();

private:
    CashService m_service;
    const Session *m_session;
    bool m_isOpen = false;
    QVariantMap m_summary;
    QVariantList m_history;
    QMap<QString, qint64> m_expected; // moneda -> esperado (caja abierta)
};
