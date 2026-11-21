#include "ExpenseService.h"

#include <QCoreApplication>

#include "core/Money.h"
#include "data/Transaction.h"

namespace {
QString tr(const char *s) { return QCoreApplication::translate("ExpenseService", s); }
void setErr(QString *error, const QString &msg) { if (error) *error = msg; }

QString monthKey(const QDate &d) { return d.toString(QStringLiteral("yyyy-MM")); }
QDate monthStart(const QString &key) { return QDate::fromString(key + QStringLiteral("-01"), QStringLiteral("yyyy-MM-dd")); }
} // namespace

ExpenseService::ExpenseService(QSqlDatabase db)
    : m_db(db)
    , m_expenses(db)
    , m_cashRepo(db)
    , m_cash(db)
{
}

QString ExpenseService::validate(const Expense &e, const QDate &today)
{
    if (!e.date.isValid())
        return tr("Fecha no válida.");
    if (e.date > today)
        return tr("La fecha del gasto no puede ser futura.");
    if (e.categoryId <= 0)
        return tr("Elige la categoría del gasto.");
    if (e.description.trimmed().isEmpty())
        return tr("Escribe en qué se gastó.");
    if (e.description.trimmed().size() > 120)
        return tr("La descripción no puede tener más de 120 caracteres.");
    if (e.amountCents <= 0)
        return tr("El importe debe ser mayor que cero.");
    if (e.amountCents > 10000000000LL)
        return tr("El importe es demasiado grande.");
    if (e.fromCash && e.date != today)
        return tr("Un gasto pagado con dinero de la caja tiene que ser de hoy.");
    return QString();
}

int ExpenseService::ensureCategory(const QString &name, QString *error)
{
    const QString n = name.simplified();
    if (n.isEmpty() || n.size() > 60) {
        setErr(error, tr("El nombre de la categoría debe tener entre 1 y 60 caracteres."));
        return -1;
    }
    const int existing = m_expenses.categoryId(n);
    if (existing > 0)
        return existing;
    const int id = m_expenses.insertCategory(n);
    if (id <= 0)
        setErr(error, tr("No se pudo crear la categoría: %1").arg(m_expenses.lastError()));
    return id;
}

bool ExpenseService::add(Expense &e, int userId, QString *error)
{
    e.description = e.description.simplified();
    const QString invalid = validate(e, QDate::currentDate());
    if (!invalid.isEmpty()) {
        setErr(error, invalid);
        return false;
    }
    Transaction tx(m_db);
    if (!tx.isActive()) {
        setErr(error, tr("No se pudo iniciar la transacción."));
        return false;
    }
    e.cashMovementId = 0;
    if (e.fromCash) {
        CashSession s;
        if (!m_cashRepo.current(&s)) {
            setErr(error, tr("No hay caja abierta: desmarca «Pagado con dinero de la caja» o abre la caja."));
            return false;
        }
        const qint64 available = m_cash.summary(s).expected.first().expectedCents;
        if (e.amountCents > available) {
            setErr(error, tr("No hay suficiente efectivo en caja (hay %1).").arg(Money::formatCents(available)));
            return false;
        }
        QString category = e.categoryName;
        for (const ExpenseCategory &c : m_expenses.categories(true)) {
            if (c.id == e.categoryId)
                category = c.name;
        }
        e.cashMovementId = m_cashRepo.insertMovement(s.id, userId, -e.amountCents,
                                                     tr("Gasto · %1: %2").arg(category, e.description));
        if (e.cashMovementId <= 0) {
            setErr(error, tr("No se pudo registrar el retiro: %1").arg(m_cashRepo.lastError()));
            return false;
        }
    }
    e.id = m_expenses.insert(e, userId);
    if (e.id <= 0) {
        setErr(error, tr("No se pudo guardar el gasto: %1").arg(m_expenses.lastError()));
        return false;
    }
    if (!tx.commit()) {
        setErr(error, tr("No se pudo confirmar el gasto."));
        return false;
    }
    return true;
}

bool ExpenseService::remove(int id, QString *error)
{
    Transaction tx(m_db);
    Expense e;
    if (!m_expenses.get(id, &e)) {
        setErr(error, tr("No se encontró el gasto."));
        return false;
    }
    if (e.fromCash && e.cashMovementId > 0) {
        CashSession s;
        const int session = m_expenses.movementSession(e.cashMovementId);
        if (!m_cashRepo.current(&s) || s.id != session) {
            setErr(error, tr("Este gasto salió de una caja que ya se cerró: no se puede borrar. "
                             "Si fue un error, registra un ingreso en la caja."));
            return false;
        }
    }
    // Primero el gasto (referencia al retiro), luego el retiro.
    if (!m_expenses.remove(id)) {
        setErr(error, tr("No se pudo borrar el gasto: %1").arg(m_expenses.lastError()));
        return false;
    }
    if (e.fromCash && e.cashMovementId > 0 && !m_cashRepo.deleteMovement(e.cashMovementId)) {
        setErr(error, tr("No se pudo borrar el retiro de la caja: %1").arg(m_cashRepo.lastError()));
        return false;
    }
    if (!tx.commit()) {
        setErr(error, tr("No se pudo confirmar."));
        return false;
    }
    return true;
}

QString ExpenseService::validateRecurring(const RecurringExpense &r)
{
    if (r.categoryId <= 0)
        return tr("Elige la categoría del gasto.");
    if (r.description.trimmed().isEmpty())
        return tr("Escribe en qué se gasta (por ejemplo «Alquiler del local»).");
    if (r.description.trimmed().size() > 120)
        return tr("La descripción no puede tener más de 120 caracteres.");
    if (r.amountCents <= 0)
        return tr("El importe debe ser mayor que cero.");
    if (r.dayOfMonth < 1 || r.dayOfMonth > 31)
        return tr("El día del mes debe estar entre 1 y 31.");
    return QString();
}

bool ExpenseService::saveRecurring(RecurringExpense &r, const QDate &today, QString *error)
{
    r.description = r.description.simplified();
    const QString invalid = validateRecurring(r);
    if (!invalid.isEmpty()) {
        setErr(error, invalid);
        return false;
    }
    if (r.id > 0) {
        if (!m_expenses.updateRecurring(r)) {
            setErr(error, tr("No se pudo guardar: %1").arg(m_expenses.lastError()));
            return false;
        }
        return true;
    }
    r.startMonth = monthKey(today);
    r.active = true;
    r.id = m_expenses.insertRecurring(r);
    if (r.id <= 0) {
        setErr(error, tr("No se pudo guardar: %1").arg(m_expenses.lastError()));
        return false;
    }
    return true;
}

bool ExpenseService::setRecurringActive(int id, bool active, QString *error, const QDate &today)
{
    RecurringExpense r;
    if (!m_expenses.getRecurring(id, &r)) {
        setErr(error, tr("No se encontró el gasto fijo."));
        return false;
    }
    if (active && !r.active) {
        const QString previous = monthKey(QDate(today.year(), today.month(), 1).addMonths(-1));
        if (r.lastMonth.isEmpty() ? r.startMonth <= previous : r.lastMonth < previous)
            m_expenses.setRecurringLastMonth(id, previous);
    }
    if (!m_expenses.setRecurringActive(id, active)) {
        setErr(error, tr("No se pudo cambiar el gasto fijo."));
        return false;
    }
    return true;
}

int ExpenseService::generateDue(const QDate &today, QString *error)
{
    Transaction tx(m_db);
    if (!tx.isActive()) {
        setErr(error, tr("No se pudo iniciar la transacción."));
        return -1;
    }
    int created = 0;
    const QString current = monthKey(today);
    for (const RecurringExpense &r : m_expenses.recurring(false)) {
        QDate month = r.lastMonth.isEmpty() ? monthStart(r.startMonth) : monthStart(r.lastMonth).addMonths(1);
        while (month.isValid() && monthKey(month) <= current) {
            const QDate due(month.year(), month.month(), qMin(r.dayOfMonth, month.daysInMonth()));
            if (due > today)
                break;
            Expense e;
            e.date = due;
            e.categoryId = r.categoryId;
            e.description = r.description;
            e.amountCents = r.amountCents;
            e.recurringId = r.id;
            if (m_expenses.insert(e, 0) <= 0 || !m_expenses.setRecurringLastMonth(r.id, monthKey(month))) {
                setErr(error, tr("No se pudo registrar el gasto fijo «%1»: %2").arg(r.description, m_expenses.lastError()));
                return -1;
            }
            ++created;
            month = month.addMonths(1);
        }
    }
    if (!tx.commit()) {
        setErr(error, tr("No se pudo confirmar los gastos fijos."));
        return -1;
    }
    return created;
}
