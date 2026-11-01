#include "CashService.h"

#include <QCoreApplication>

#include "core/Money.h"
#include "core/SaleCalculator.h"
#include "data/Transaction.h"

namespace {
QString tr(const char *s) { return QCoreApplication::translate("CashService", s); }
void setErr(QString *error, const QString &msg) { if (error) *error = msg; }
} // namespace

CashService::CashService(QSqlDatabase db)
    : m_db(db)
    , m_cash(db)
    , m_currencies(db)
{
}

bool CashService::open(qint64 openingCents, int userId, int *sessionId, QString *error)
{
    if (openingCents < 0) {
        setErr(error, tr("El fondo inicial no puede ser negativo."));
        return false;
    }
    if (userId <= 0) {
        setErr(error, tr("No hay un usuario activo."));
        return false;
    }
    if (m_cash.current(nullptr)) {
        setErr(error, tr("Ya hay una caja abierta."));
        return false;
    }
    const int id = m_cash.open(userId, openingCents);
    if (id <= 0) {
        setErr(error, tr("No se pudo abrir la caja: %1").arg(m_cash.lastError()));
        return false;
    }
    if (sessionId)
        *sessionId = id;
    return true;
}

bool CashService::current(CashSession *out)
{
    return m_cash.current(out);
}

CashSummary CashService::summary(const CashSession &session)
{
    CashSummary s;
    const SalesStats st = m_cash.salesStats(session.id);
    s.salesCount = st.count;
    s.salesTotal = st.total;
    s.profit = st.total - st.cost;
    m_cash.movementTotals(session.id, &s.movementsIn, &s.movementsOut);

    Currency base;
    m_currencies.base(&base);

    // Efectivo esperado por moneda. La moneda base incluye fondo, vuelto y movimientos manuales.
    CurrencyAmount baseAmount;
    baseAmount.currency = base.code;
    baseAmount.rateMilli = 1000;
    baseAmount.expectedCents = session.openingCents - st.change + s.movementsIn - s.movementsOut;
    s.expected.append(baseAmount);

    qint64 cashBase = 0;
    for (const PaymentTotal &p : m_cash.paymentTotals(session.id)) {
        if (p.method == PaymentMethod::transfer()) {
            s.transferSales += p.baseCents;
            continue;
        }
        cashBase += p.baseCents;
        if (p.currency == base.code) {
            s.expected[0].expectedCents += p.amountCents;
            continue;
        }
        Currency c;
        CurrencyAmount ca;
        ca.currency = p.currency;
        ca.rateMilli = m_currencies.get(p.currency, &c) ? c.rateMilli : 1000;
        ca.expectedCents = p.amountCents;
        s.expected.append(ca);
    }
    s.cashSales = cashBase - st.change;
    return s;
}

bool CashService::addMovement(qint64 amountCents, const QString &reason, int userId, QString *error)
{
    if (amountCents == 0) {
        setErr(error, tr("El importe no puede ser cero."));
        return false;
    }
    if (reason.trimmed().isEmpty()) {
        setErr(error, tr("Indica el motivo."));
        return false;
    }
    CashSession s;
    if (!m_cash.current(&s)) {
        setErr(error, tr("No hay caja abierta."));
        return false;
    }
    if (amountCents < 0) {
        const qint64 available = summary(s).expected.first().expectedCents;
        if (-amountCents > available) {
            setErr(error, tr("No hay suficiente efectivo en caja (hay %1).").arg(Money::formatCents(available)));
            return false;
        }
    }
    if (m_cash.insertMovement(s.id, userId, amountCents, reason.trimmed()) <= 0) {
        setErr(error, tr("No se pudo registrar el movimiento: %1").arg(m_cash.lastError()));
        return false;
    }
    return true;
}

bool CashService::close(const QMap<QString, qint64> &counted, int userId, CashSession *closed, QString *error)
{
    if (userId <= 0) {
        setErr(error, tr("No hay un usuario activo."));
        return false;
    }
    Transaction tx(m_db);
    CashSession s;
    if (!m_cash.current(&s)) {
        setErr(error, tr("No hay caja abierta."));
        return false;
    }

    const CashSummary sum = summary(s);
    qint64 expectedBase = 0;
    qint64 countedBase = 0;
    for (CurrencyAmount ca : sum.expected) {
        if (!counted.contains(ca.currency)) {
            setErr(error, tr("Falta el conteo de %1.").arg(ca.currency));
            return false;
        }
        ca.countedCents = counted.value(ca.currency);
        if (ca.countedCents < 0) {
            setErr(error, tr("El conteo no puede ser negativo."));
            return false;
        }
        expectedBase += SaleCalculator::toBase(ca.expectedCents, ca.rateMilli);
        countedBase += SaleCalculator::toBase(ca.countedCents, ca.rateMilli);
        if (!m_cash.insertCount(s.id, ca)) {
            setErr(error, tr("No se pudo guardar el arqueo: %1").arg(m_cash.lastError()));
            return false;
        }
    }

    if (!m_cash.close(s.id, userId, expectedBase, countedBase)) {
        setErr(error, tr("No se pudo cerrar la caja: %1").arg(m_cash.lastError()));
        return false;
    }
    if (!tx.commit()) {
        setErr(error, tr("No se pudo confirmar el cierre."));
        return false;
    }
    if (closed)
        m_cash.get(s.id, closed);
    return true;
}
