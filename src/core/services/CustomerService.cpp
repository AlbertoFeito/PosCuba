#include "CustomerService.h"

#include <QCoreApplication>

#include "core/Money.h"
#include "core/Sale.h"
#include "data/Transaction.h"

namespace {
QString tr(const char *s) { return QCoreApplication::translate("CustomerService", s); }
void setErr(QString *error, const QString &msg) { if (error) *error = msg; }
} // namespace

CustomerService::CustomerService(QSqlDatabase db)
    : m_db(db)
    , m_customers(db)
    , m_cash(db)
{
}

QString CustomerService::validate(const Customer &c)
{
    const QString name = c.name.trimmed();
    if (name.isEmpty())
        return tr("Escribe el nombre del cliente.");
    if (name.size() > 80)
        return tr("El nombre no puede tener más de 80 caracteres.");
    if (c.phone.trimmed().size() > 30)
        return tr("El teléfono no puede tener más de 30 caracteres.");
    if (c.note.trimmed().size() > 200)
        return tr("La nota no puede tener más de 200 caracteres.");
    if (c.creditLimitCents < 0)
        return tr("El límite de crédito no puede ser negativo.");
    return QString();
}

bool CustomerService::save(Customer &c, QString *error)
{
    c.name = c.name.simplified();
    c.phone = c.phone.trimmed();
    c.note = c.note.trimmed();
    const QString invalid = validate(c);
    if (!invalid.isEmpty()) {
        setErr(error, invalid);
        return false;
    }
    if (m_customers.nameExists(c.name, c.id)) {
        setErr(error, tr("Ya existe un cliente llamado «%1».").arg(c.name));
        return false;
    }
    if (c.id > 0) {
        if (!m_customers.update(c)) {
            setErr(error, tr("No se pudo guardar el cliente: %1").arg(m_customers.lastError()));
            return false;
        }
        return true;
    }
    const int id = m_customers.insert(c);
    if (id <= 0) {
        setErr(error, tr("No se pudo guardar el cliente: %1").arg(m_customers.lastError()));
        return false;
    }
    c.id = id;
    return true;
}

bool CustomerService::setActive(int id, bool active, QString *error)
{
    if (!active && m_customers.balance(id) > 0) {
        setErr(error, tr("No se puede desactivar: el cliente todavía debe %1.").arg(Money::formatCents(m_customers.balance(id))));
        return false;
    }
    if (!m_customers.setActive(id, active)) {
        setErr(error, tr("No se pudo cambiar el cliente: %1").arg(m_customers.lastError()));
        return false;
    }
    return true;
}

bool CustomerService::canCharge(int customerId, qint64 amount, QString *error)
{
    Customer c;
    if (customerId <= 0 || !m_customers.get(customerId, &c)) {
        setErr(error, tr("Elige el cliente que se lleva la mercancía a crédito."));
        return false;
    }
    if (!c.active) {
        setErr(error, tr("El cliente «%1» está desactivado.").arg(c.name));
        return false;
    }
    if (c.creditLimitCents > 0 && c.balanceCents + amount > c.creditLimitCents) {
        setErr(error, tr("Supera el límite de crédito de %1 (%2): ya debe %3 y quedaría debiendo %4.")
                          .arg(c.name, Money::formatCents(c.creditLimitCents), Money::formatCents(c.balanceCents),
                               Money::formatCents(c.balanceCents + amount)));
        return false;
    }
    return true;
}

bool CustomerService::registerPayment(int customerId, qint64 amount, const QString &method, const QString &note,
                                      int userId, QString *error)
{
    if (amount <= 0) {
        setErr(error, tr("El abono debe ser mayor que cero."));
        return false;
    }
    if (method != PaymentMethod::cash() && method != PaymentMethod::transfer()) {
        setErr(error, tr("Forma de pago no válida."));
        return false;
    }
    if (userId <= 0) {
        setErr(error, tr("No hay un usuario activo."));
        return false;
    }
    Transaction tx(m_db);
    if (!tx.isActive()) {
        setErr(error, tr("No se pudo iniciar la transacción."));
        return false;
    }
    Customer c;
    if (!m_customers.get(customerId, &c)) {
        setErr(error, tr("No se encontró el cliente."));
        return false;
    }
    if (amount > c.balanceCents) {
        setErr(error, c.balanceCents > 0
                          ? tr("El abono no puede ser mayor que la deuda (%1).").arg(Money::formatCents(c.balanceCents))
                          : tr("%1 no debe nada.").arg(c.name));
        return false;
    }
    CashSession session;
    if (!m_cash.current(&session)) {
        setErr(error, tr("Abre la caja para registrar abonos: el dinero entra a la caja abierta."));
        return false;
    }
    if (!m_customers.insertEntry(customerId, CustomerEntryKind::payment(), amount, 0, 0, session.id, method, note,
                                 userId)) {
        setErr(error, tr("No se pudo registrar el abono: %1").arg(m_customers.lastError()));
        return false;
    }
    if (!tx.commit()) {
        setErr(error, tr("No se pudo confirmar el abono."));
        return false;
    }
    return true;
}
