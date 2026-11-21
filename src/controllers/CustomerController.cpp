#include "CustomerController.h"

#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

#include "core/CsvWriter.h"
#include "core/Money.h"
#include "core/Sale.h"

namespace {
QVariantMap result(bool ok, const QString &error)
{
    return QVariantMap{{QStringLiteral("ok"), ok}, {QStringLiteral("error"), error}};
}

QString shortDate(const QString &sqlDate)
{
    const QDateTime dt = QDateTime::fromString(sqlDate, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    return dt.isValid() ? dt.toString(QStringLiteral("dd/MM/yyyy HH:mm")) : sqlDate;
}
} // namespace

CustomerController::CustomerController(QSqlDatabase db, const Session *session, QObject *parent)
    : QObject(parent)
    , m_service(db)
    , m_session(session)
{
    reload();
}

void CustomerController::setSearch(const QString &s)
{
    if (s == m_search)
        return;
    m_search = s;
    reload();
}

void CustomerController::setOnlyDebt(bool on)
{
    if (on == m_onlyDebt)
        return;
    m_onlyDebt = on;
    reload();
}

void CustomerController::setIncludeInactive(bool on)
{
    if (on == m_includeInactive)
        return;
    m_includeInactive = on;
    reload();
}

QString CustomerController::receivablesText() const
{
    return Money::formatCents(m_receivables);
}

QVariantMap CustomerController::toMap(const Customer &c) const
{
    return QVariantMap{
        {QStringLiteral("id"), c.id},
        {QStringLiteral("name"), c.name},
        {QStringLiteral("phone"), c.phone},
        {QStringLiteral("note"), c.note},
        {QStringLiteral("balance"), double(c.balanceCents)},
        {QStringLiteral("balanceText"), Money::formatCents(c.balanceCents)},
        {QStringLiteral("limitText"), c.creditLimitCents > 0 ? Money::formatCents(c.creditLimitCents) : tr("sin límite")},
        {QStringLiteral("limitValue"), c.creditLimitCents > 0 ? Money::formatCents(c.creditLimitCents) : QString()},
        {QStringLiteral("hasLimit"), c.creditLimitCents > 0},
        {QStringLiteral("overLimit"), c.creditLimitCents > 0 && c.balanceCents > c.creditLimitCents},
        {QStringLiteral("active"), c.active},
        {QStringLiteral("lastText"), c.lastActivity.isEmpty() ? QStringLiteral("—") : shortDate(c.lastActivity).left(10)},
    };
}

void CustomerController::reload()
{
    m_list.clear();
    for (const Customer &c : m_service.customers().list(m_search, m_includeInactive, m_onlyDebt))
        m_list.append(toMap(c));
    m_receivables = m_service.customers().receivables(&m_debtors);
    emit changed();
}

QVariantMap CustomerController::get(int id)
{
    Customer c;
    if (!m_service.customers().get(id, &c))
        return result(false, tr("No se encontró el cliente."));
    QVariantMap m = toMap(c);
    m.insert(QStringLiteral("ok"), true);
    return m;
}

QVariantMap CustomerController::save(const QVariantMap &form)
{
    if (!m_session || !m_session->isValid())
        return result(false, tr("No hay un usuario activo."));
    Customer c;
    const int id = form.value(QStringLiteral("id")).toInt();
    if (id > 0 && !m_service.customers().get(id, &c))
        return result(false, tr("No se encontró el cliente."));
    c.name = form.value(QStringLiteral("name")).toString();
    c.phone = form.value(QStringLiteral("phone")).toString();
    c.note = form.value(QStringLiteral("note")).toString();
    if (isAdmin()) {
        const QString limitText = form.value(QStringLiteral("limit")).toString().trimmed();
        qint64 limit = 0;
        if (!limitText.isEmpty() && !Money::parseCents(limitText, &limit))
            return result(false, tr("Límite de crédito no válido."));
        c.creditLimitCents = limit;
    }
    QString error;
    if (!m_service.save(c, &error))
        return result(false, error);
    reload();
    QVariantMap r = result(true, QString());
    r.insert(QStringLiteral("id"), c.id);
    return r;
}

QVariantMap CustomerController::setActive(int id, bool active)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede desactivar clientes."));
    QString error;
    if (!m_service.setActive(id, active, &error))
        return result(false, error);
    reload();
    return result(true, QString());
}

QVariantList CustomerController::entries(int id)
{
    QVariantList out;
    for (const CustomerEntry &e : m_service.customers().entries(id)) {
        QString kindText;
        QString detail;
        if (e.kind == CustomerEntryKind::charge()) {
            kindText = tr("Compra a crédito");
            detail = e.saleNumber > 0 ? tr("Venta #%1").arg(e.saleNumber) : e.note;
        } else if (e.kind == CustomerEntryKind::payment()) {
            kindText = tr("Abono");
            detail = e.method == PaymentMethod::transfer() ? tr("Transferencia") : tr("Efectivo");
            if (!e.note.isEmpty())
                detail += QStringLiteral(" · ") + e.note;
        } else {
            kindText = tr("Devolución");
            detail = e.note;
        }
        if (!e.userName.isEmpty())
            detail += QStringLiteral(" · ") + e.userName;
        out.append(QVariantMap{
            {QStringLiteral("dateText"), shortDate(e.createdAt)},
            {QStringLiteral("kind"), e.kind},
            {QStringLiteral("kindText"), kindText},
            {QStringLiteral("amountText"), (e.kind == CustomerEntryKind::charge() ? QStringLiteral("+") : QStringLiteral("−"))
                                               + Money::formatCents(e.amountCents)},
            {QStringLiteral("detail"), detail},
            {QStringLiteral("balanceText"), Money::formatCents(e.balanceAfter)},
        });
    }
    return out;
}

QVariantMap CustomerController::registerPayment(int id, const QString &amountText, const QString &method,
                                                const QString &note)
{
    if (!m_session || !m_session->isValid())
        return result(false, tr("No hay un usuario activo."));
    qint64 amount = 0;
    if (!Money::parseCents(amountText.trimmed(), &amount))
        return result(false, tr("Importe no válido."));
    QString error;
    if (!m_service.registerPayment(id, amount, method, note, m_session->userId, &error))
        return result(false, error);
    reload();
    emit paymentRegistered();
    QVariantMap r = result(true, QString());
    r.insert(QStringLiteral("balanceText"), Money::formatCents(m_service.customers().balance(id)));
    r.insert(QStringLiteral("amountText"), Money::formatCents(amount));
    return r;
}

QVariantList CustomerController::pick(const QString &text)
{
    QVariantList out;
    for (const Customer &c : m_service.customers().list(text, false, false)) {
        if (out.size() >= 50)
            break;
        out.append(toMap(c));
    }
    return out;
}

QVariantMap CustomerController::exportReceivables(const QString &format)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede exportar."));
    const QString folder = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                               .filePath(QStringLiteral("PosCuba/Reportes"));
    const QString path = QDir(folder).filePath(
        QStringLiteral("cuentas_por_cobrar_%1.csv").arg(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"))));
    const CsvWriter csv(format == QLatin1String("std") ? CsvWriter::Standard : CsvWriter::Spanish);
    QVector<QStringList> rows;
    for (const Customer &c : m_service.customers().list(QString(), true, true)) {
        rows.append({c.name, c.phone, csv.number(Money::formatCents(c.balanceCents)),
                     c.creditLimitCents > 0 ? csv.number(Money::formatCents(c.creditLimitCents)) : QString(),
                     c.lastActivity.left(10), c.note});
    }
    QString error;
    if (!QDir().mkpath(folder)
        || !csv.write(path, {tr("Cliente"), tr("Teléfono"), tr("Debe"), tr("Límite"), tr("Último movimiento"), tr("Nota")},
                      rows, &error))
        return result(false, error.isEmpty() ? tr("No se pudo crear la carpeta %1.").arg(QDir::toNativeSeparators(folder))
                                             : error);
    QVariantMap r = result(true, QString());
    r.insert(QStringLiteral("path"), QDir::toNativeSeparators(path));
    r.insert(QStringLiteral("count"), rows.size());
    return r;
}
