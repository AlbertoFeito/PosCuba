#include "CurrencyController.h"

#include "core/Money.h"

namespace {
QVariantMap result(bool ok, const QString &error)
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("error"), error}};
}
} // namespace

CurrencyController::CurrencyController(QSqlDatabase db, const Session *session, QObject *parent)
    : QObject(parent)
    , m_service(db)
    , m_session(session)
{
}

void CurrencyController::refresh()
{
    m_list.clear();
    for (const Currency &c : m_service.all()) {
        if (c.isBase)
            m_baseCode = c.code;
        m_list.append(QVariantMap{
            {QStringLiteral("code"), c.code},
            {QStringLiteral("name"), c.name},
            {QStringLiteral("symbol"), c.symbol},
            {QStringLiteral("rateText"), Money::formatMilli(c.rateMilli)},
            {QStringLiteral("isBase"), c.isBase},
            {QStringLiteral("active"), c.active},
        });
    }
    m_canChangeBase = !m_service.hasSales();
    emit changed();
}

bool CurrencyController::fromForm(const QVariantMap &form, Currency *c, QString *error) const
{
    c->code = form.value(QStringLiteral("code")).toString();
    c->name = form.value(QStringLiteral("name")).toString();
    c->symbol = form.value(QStringLiteral("symbol")).toString();
    c->active = form.value(QStringLiteral("active"), true).toBool();
    const QString rate = form.value(QStringLiteral("rate")).toString().trimmed();
    if (rate.isEmpty() && c->code.trimmed().toUpper() == m_baseCode) {
        c->rateMilli = 1000;
        return true;
    }
    if (!Money::parseMilli(rate, &c->rateMilli) || c->rateMilli <= 0) {
        *error = tr("Tasa no válida. Escribe cuántos %1 vale 1 unidad (ej.: 400 o 412.5).").arg(m_baseCode);
        return false;
    }
    return true;
}

QVariantMap CurrencyController::add(const QVariantMap &form)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede cambiar monedas."));
    Currency c;
    QString error;
    if (!fromForm(form, &c, &error) || !m_service.add(c, &error))
        return result(false, error);
    refresh();
    emit currenciesEdited();
    return result(true, QString());
}

QVariantMap CurrencyController::update(const QVariantMap &form)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede cambiar monedas."));
    Currency c;
    QString error;
    if (!fromForm(form, &c, &error) || !m_service.update(c, &error))
        return result(false, error);
    refresh();
    emit currenciesEdited();
    return result(true, QString());
}

QVariantMap CurrencyController::setBase(const QString &code)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede cambiar monedas."));
    QString error;
    if (!m_service.setBase(code, &error))
        return result(false, error);
    refresh();
    emit currenciesEdited();
    return result(true, QString());
}
