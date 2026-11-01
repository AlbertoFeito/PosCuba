#pragma once

#include <QMap>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "core/Sale.h"
#include "data/repositories/CashRepository.h"
#include "data/repositories/CurrencyRepository.h"
#include "data/repositories/RefundRepository.h"

class CashService
{
public:
    explicit CashService(QSqlDatabase db);

    bool open(qint64 openingCents, int userId, int *sessionId, QString *error);
    bool current(CashSession *out);
    CashSummary summary(const CashSession &session);

    // amountCents > 0 ingreso, < 0 retiro. Siempre en moneda base.
    bool addMovement(qint64 amountCents, const QString &reason, int userId, QString *error);

    // counted: moneda -> centavos contados. Debe incluir cada moneda con efectivo esperado.
    bool close(const QMap<QString, qint64> &counted, int userId, CashSession *closed, QString *error);

    QVector<CashSession> recent(int limit) { return m_cash.recent(limit); }
    QVector<CurrencyAmount> counts(int sessionId) { return m_cash.counts(sessionId); }

private:
    QSqlDatabase m_db;
    CashRepository m_cash;
    CurrencyRepository m_currencies;
    RefundRepository m_refunds;
};
