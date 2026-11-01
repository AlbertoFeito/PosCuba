#include "CurrencyService.h"

#include <QCoreApplication>
#include <QSqlQuery>
#include <QVariant>

#include "data/Transaction.h"

namespace {
QString tr(const char *s) { return QCoreApplication::translate("CurrencyService", s); }
void setErr(QString *error, const QString &msg) { if (error) *error = msg; }
constexpr qint64 kMaxRateMilli = 1000000000; // 1 000 000 por unidad
} // namespace

CurrencyService::CurrencyService(QSqlDatabase db)
    : m_db(db)
    , m_repo(db)
{
}

QString CurrencyService::validate(Currency &c)
{
    c.code = c.code.trimmed().toUpper();
    c.name = c.name.simplified();
    c.symbol = c.symbol.trimmed();
    if (c.code.size() != 3)
        return tr("El código debe tener 3 letras (ej.: USD, MLC, EUR).");
    for (const QChar ch : c.code) {
        if (ch < QLatin1Char('A') || ch > QLatin1Char('Z'))
            return tr("El código debe tener 3 letras (ej.: USD, MLC, EUR).");
    }
    if (c.name.isEmpty() || c.name.size() > 40)
        return tr("El nombre es obligatorio (máximo 40 caracteres).");
    if (c.symbol.isEmpty() || c.symbol.size() > 5)
        return tr("El símbolo es obligatorio (máximo 5 caracteres).");
    if (!c.isBase && (c.rateMilli <= 0 || c.rateMilli > kMaxRateMilli))
        return tr("La tasa debe ser mayor que cero.");
    return QString();
}

bool CurrencyService::add(Currency c, QString *error)
{
    c.isBase = false;
    const QString invalid = validate(c);
    if (!invalid.isEmpty()) {
        setErr(error, invalid);
        return false;
    }
    if (m_repo.get(c.code, nullptr)) {
        setErr(error, tr("La moneda %1 ya existe.").arg(c.code));
        return false;
    }
    if (!m_repo.insert(c)) {
        setErr(error, tr("No se pudo guardar la moneda: %1").arg(m_repo.lastError()));
        return false;
    }
    return true;
}

bool CurrencyService::update(Currency c, QString *error)
{
    Currency current;
    if (!m_repo.get(c.code.trimmed().toUpper(), &current)) {
        setErr(error, tr("La moneda no existe."));
        return false;
    }
    c.isBase = current.isBase;
    if (c.isBase) {
        c.rateMilli = 1000;
        if (!c.active) {
            setErr(error, tr("La moneda base no se puede desactivar."));
            return false;
        }
    }
    const QString invalid = validate(c);
    if (!invalid.isEmpty()) {
        setErr(error, invalid);
        return false;
    }
    if (!m_repo.update(c)) {
        setErr(error, tr("No se pudo guardar la moneda: %1").arg(m_repo.lastError()));
        return false;
    }
    return true;
}

bool CurrencyService::hasSales()
{
    QSqlQuery q(m_db);
    return q.exec(QStringLiteral("SELECT 1 FROM sales LIMIT 1")) && q.next();
}

bool CurrencyService::setBase(const QString &rawCode, QString *error)
{
    const QString code = rawCode.trimmed().toUpper();
    Currency c;
    if (!m_repo.get(code, &c)) {
        setErr(error, tr("La moneda no existe."));
        return false;
    }
    if (c.isBase)
        return true;
    if (hasSales()) {
        setErr(error, tr("No se puede cambiar la moneda base cuando ya hay ventas registradas."));
        return false;
    }
    Transaction tx(m_db);
    if (!m_repo.setBase(code) || !tx.commit()) {
        setErr(error, tr("No se pudo cambiar la moneda base: %1").arg(m_repo.lastError()));
        return false;
    }
    return true;
}
