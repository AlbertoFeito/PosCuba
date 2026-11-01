#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "core/Sale.h"

struct PaymentTotal
{
    QString method;
    QString currency;
    qint64 amountCents = 0; // en la moneda del pago
    qint64 baseCents = 0;
};

struct SalesStats
{
    int count = 0;
    qint64 total = 0;
    qint64 change = 0;
    qint64 cost = 0;
};

class CashRepository
{
public:
    explicit CashRepository(QSqlDatabase db);

    int open(int userId, qint64 openingCents);                 // id o -1
    bool current(CashSession *out);                            // sesion abierta
    bool get(int id, CashSession *out);
    bool close(int id, int userId, qint64 expected, qint64 counted);
    bool insertCount(int sessionId, const CurrencyAmount &c);
    QVector<CurrencyAmount> counts(int sessionId);
    int insertMovement(int sessionId, int userId, qint64 amountCents, const QString &reason);
    QVector<CashSession> recent(int limit);

    SalesStats salesStats(int sessionId);
    QVector<PaymentTotal> paymentTotals(int sessionId);
    void movementTotals(int sessionId, qint64 *in, qint64 *out);

    QString lastError() const { return m_lastError; }

private:
    QSqlDatabase m_db;
    QString m_lastError;
};
