#include "CashController.h"

#include <QDateTime>

#include "core/Money.h"

namespace {
QVariantMap result(bool ok, const QString &error)
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("error"), error}};
}

QString shortDate(const QString &sqlDate)
{
    if (sqlDate.isEmpty())
        return QString();
    const QDateTime dt = QDateTime::fromString(sqlDate, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    return dt.isValid() ? dt.toString(QStringLiteral("dd/MM/yyyy HH:mm")) : sqlDate;
}

QString signedMoney(qint64 cents)
{
    if (cents > 0)
        return QStringLiteral("+") + Money::formatCents(cents);
    return Money::formatCents(cents);
}
} // namespace

CashController::CashController(QSqlDatabase db, const Session *session, QObject *parent)
    : QObject(parent)
    , m_service(db)
    , m_session(session)
{
}

void CashController::refresh()
{
    CashSession s;
    m_isOpen = m_service.current(&s);
    m_summary.clear();
    m_expected.clear();

    if (m_isOpen) {
        const CashSummary sum = m_service.summary(s);
        QVariantList expected;
        for (const CurrencyAmount &ca : sum.expected) {
            m_expected.insert(ca.currency, ca.expectedCents);
            expected.append(QVariantMap{
                {QStringLiteral("currency"), ca.currency},
                {QStringLiteral("expectedText"), Money::formatCents(ca.expectedCents)},
                {QStringLiteral("isBase"), ca.rateMilli == 1000 && expected.isEmpty()},
            });
        }
        m_summary = {
            {QStringLiteral("openedAt"), shortDate(s.openedAt)},
            {QStringLiteral("openedBy"), s.openedByName},
            {QStringLiteral("openingText"), Money::formatCents(s.openingCents)},
            {QStringLiteral("salesCount"), sum.salesCount},
            {QStringLiteral("salesTotalText"), Money::formatCents(sum.salesTotal)},
            {QStringLiteral("cashSalesText"), Money::formatCents(sum.cashSales)},
            {QStringLiteral("transferText"), Money::formatCents(sum.transferSales)},
            {QStringLiteral("movementsInText"), Money::formatCents(sum.movementsIn)},
            {QStringLiteral("movementsOutText"), Money::formatCents(sum.movementsOut)},
            {QStringLiteral("profitText"), Money::formatCents(sum.profit)},
            {QStringLiteral("expected"), expected},
        };
    }

    m_history.clear();
    for (const CashSession &h : m_service.recent(15)) {
        if (h.status != QLatin1String("closed"))
            continue;
        m_history.append(QVariantMap{
            {QStringLiteral("id"), h.id},
            {QStringLiteral("openedAt"), shortDate(h.openedAt)},
            {QStringLiteral("closedAt"), shortDate(h.closedAt)},
            {QStringLiteral("openedBy"), h.openedByName},
            {QStringLiteral("expectedText"), Money::formatCents(h.expectedCents)},
            {QStringLiteral("countedText"), Money::formatCents(h.countedCents)},
            {QStringLiteral("differenceText"), signedMoney(h.differenceCents)},
            {QStringLiteral("differenceCents"), static_cast<double>(h.differenceCents)},
        });
    }
    emit changed();
}

QVariantMap CashController::open(const QString &openingText)
{
    qint64 opening = 0;
    if (!openingText.trimmed().isEmpty() && !Money::parseCents(openingText, &opening))
        return result(false, tr("Fondo inicial no válido (ej.: 500 o 500.00)."));
    QString error;
    if (!m_service.open(opening, m_session ? m_session->userId : 0, nullptr, &error))
        return result(false, error);
    refresh();
    emit sessionOpenedOrClosed();
    return result(true, QString());
}

QVariantMap CashController::addMovement(const QString &amountText, bool withdrawal, const QString &reason)
{
    qint64 amount = 0;
    if (!Money::parseCents(amountText, &amount) || amount <= 0)
        return result(false, tr("Importe no válido."));
    QString error;
    if (!m_service.addMovement(withdrawal ? -amount : amount, reason, m_session ? m_session->userId : 0, &error))
        return result(false, error);
    refresh();
    return result(true, QString());
}

QVariantMap CashController::previewDifference(const QString &currency, const QString &countedText) const
{
    qint64 counted = 0;
    if (!Money::parseCents(countedText, &counted))
        return {{QStringLiteral("valid"), false}};
    const qint64 diff = counted - m_expected.value(currency);
    return {{QStringLiteral("valid"), true},
            {QStringLiteral("differenceText"), signedMoney(diff)},
            {QStringLiteral("differenceCents"), static_cast<double>(diff)}};
}

QVariantMap CashController::close(const QVariantMap &countedForm)
{
    QMap<QString, qint64> counted;
    for (auto it = countedForm.constBegin(); it != countedForm.constEnd(); ++it) {
        qint64 v = 0;
        const QString text = it.value().toString().trimmed();
        if (!Money::parseCents(text.isEmpty() ? QStringLiteral("0") : text, &v))
            return result(false, tr("Conteo de %1 no válido.").arg(it.key()));
        counted.insert(it.key(), v);
    }
    QString error;
    CashSession closed;
    if (!m_service.close(counted, m_session ? m_session->userId : 0, &closed, &error))
        return result(false, error);

    refresh();
    emit sessionOpenedOrClosed();
    emit sessionClosed();
    QVariantMap r = result(true, QString());
    r.insert(QStringLiteral("expectedText"), Money::formatCents(closed.expectedCents));
    r.insert(QStringLiteral("countedText"), Money::formatCents(closed.countedCents));
    r.insert(QStringLiteral("differenceText"), signedMoney(closed.differenceCents));
    r.insert(QStringLiteral("differenceCents"), static_cast<double>(closed.differenceCents));
    return r;
}
