#include "ExpenseController.h"

#include <QDir>
#include <QStandardPaths>

#include "core/CsvWriter.h"
#include "core/Money.h"

namespace {
QVariantMap result(bool ok, const QString &error)
{
    return QVariantMap{{QStringLiteral("ok"), ok}, {QStringLiteral("error"), error}};
}

QDate parseDate(const QString &text)
{
    const QString t = text.trimmed();
    for (const char *fmt : {"dd/MM/yyyy", "d/M/yyyy", "dd-MM-yyyy", "yyyy-MM-dd"}) {
        const QDate d = QDate::fromString(t, QString::fromLatin1(fmt));
        if (d.isValid())
            return d;
    }
    return QDate();
}
} // namespace

ExpenseController::ExpenseController(QSqlDatabase db, const Session *session, QObject *parent)
    : QObject(parent)
    , m_service(db)
    , m_session(session)
{
    setPreset(QStringLiteral("month"));
    loadCategories();
}

QString ExpenseController::totalText() const
{
    return Money::formatCents(m_total);
}

void ExpenseController::setCategoryFilter(int id)
{
    if (id == m_categoryFilter)
        return;
    m_categoryFilter = id;
    refresh();
}

void ExpenseController::setPreset(const QString &key)
{
    const QDate today = QDate::currentDate();
    const QDate monthStart(today.year(), today.month(), 1);
    if (key == QLatin1String("lastMonth")) {
        m_from = monthStart.addMonths(-1);
        m_to = monthStart.addDays(-1);
    } else if (key == QLatin1String("last30")) {
        m_from = today.addDays(-29);
        m_to = today;
    } else if (key == QLatin1String("year")) {
        m_from = QDate(today.year(), 1, 1);
        m_to = today;
    } else {
        m_from = monthStart;
        m_to = today;
    }
    m_preset = key == QLatin1String("lastMonth") || key == QLatin1String("last30") || key == QLatin1String("year")
                   ? key : QStringLiteral("month");
    refresh();
}

QVariantMap ExpenseController::setRange(const QString &fromText, const QString &toText)
{
    const QDate from = parseDate(fromText);
    const QDate to = parseDate(toText);
    if (!from.isValid() || !to.isValid())
        return result(false, tr("Fecha no válida. Usa dd/mm/aaaa."));
    if (from > to)
        return result(false, tr("La fecha inicial es posterior a la final."));
    m_from = from;
    m_to = to;
    m_preset = QStringLiteral("custom");
    refresh();
    return result(true, QString());
}

void ExpenseController::reload()
{
    m_service.generateDue(QDate::currentDate());
    loadCategories();
    refresh();
}

void ExpenseController::loadCategories()
{
    m_categories.clear();
    for (const ExpenseCategory &c : m_service.expenses().categories())
        m_categories.append(QVariantMap{{QStringLiteral("id"), c.id}, {QStringLiteral("name"), c.name}});
    emit categoriesChanged();
}

void ExpenseController::refresh()
{
    m_expenses.clear();
    m_totals.clear();
    m_recurring.clear();
    m_total = 0;
    for (const Expense &e : m_service.expenses().list(m_from, m_to, m_categoryFilter)) {
        m_expenses.append(QVariantMap{
            {QStringLiteral("id"), e.id},
            {QStringLiteral("dateText"), e.date.toString(QStringLiteral("dd/MM/yyyy"))},
            {QStringLiteral("categoryName"), e.categoryName},
            {QStringLiteral("description"), e.description},
            {QStringLiteral("amountText"), Money::formatCents(e.amountCents)},
            {QStringLiteral("fromCash"), e.fromCash},
            {QStringLiteral("recurring"), e.recurringId > 0},
            {QStringLiteral("userName"), e.userName},
        });
    }
    const QVector<ExpenseTotal> totals = m_service.expenses().totals(m_from, m_to);
    for (const ExpenseTotal &t : totals)
        m_total += t.amountCents;
    for (const ExpenseTotal &t : totals) {
        m_totals.append(QVariantMap{
            {QStringLiteral("categoryId"), t.categoryId},
            {QStringLiteral("name"), t.categoryName},
            {QStringLiteral("amountText"), Money::formatCents(t.amountCents)},
            {QStringLiteral("count"), t.count},
            {QStringLiteral("share"), m_total > 0 ? double(t.amountCents) / double(m_total) : 0.0},
        });
    }
    for (const RecurringExpense &r : m_service.expenses().recurring(true)) {
        QString last = QStringLiteral("—");
        if (!r.lastMonth.isEmpty()) {
            const QDate d = QDate::fromString(r.lastMonth + QStringLiteral("-01"), QStringLiteral("yyyy-MM-dd"));
            last = d.isValid() ? d.toString(QStringLiteral("MM/yyyy")) : r.lastMonth;
        }
        m_recurring.append(QVariantMap{
            {QStringLiteral("id"), r.id},
            {QStringLiteral("categoryId"), r.categoryId},
            {QStringLiteral("categoryName"), r.categoryName},
            {QStringLiteral("description"), r.description},
            {QStringLiteral("amountText"), Money::formatCents(r.amountCents)},
            {QStringLiteral("day"), r.dayOfMonth},
            {QStringLiteral("active"), r.active},
            {QStringLiteral("lastText"), last},
        });
    }
    emit changed();
}

QVariantMap ExpenseController::add(const QVariantMap &form)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede registrar gastos."));
    Expense e;
    e.date = parseDate(form.value(QStringLiteral("dateText")).toString());
    e.categoryId = form.value(QStringLiteral("categoryId")).toInt();
    e.description = form.value(QStringLiteral("description")).toString();
    e.fromCash = form.value(QStringLiteral("fromCash")).toBool();
    if (!Money::parseCents(form.value(QStringLiteral("amount")).toString().trimmed(), &e.amountCents))
        return result(false, tr("Importe no válido."));
    QString error;
    if (!m_service.add(e, m_session->userId, &error))
        return result(false, error);
    refresh();
    emit expensesChanged();
    if (e.fromCash)
        emit cashChanged();
    return result(true, QString());
}

QVariantMap ExpenseController::remove(int id)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede borrar gastos."));
    Expense e;
    const bool fromCash = m_service.expenses().get(id, &e) && e.fromCash;
    QString error;
    if (!m_service.remove(id, &error))
        return result(false, error);
    refresh();
    emit expensesChanged();
    if (fromCash)
        emit cashChanged();
    return result(true, QString());
}

QVariantMap ExpenseController::addCategory(const QString &name)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede crear categorías."));
    QString error;
    const int id = m_service.ensureCategory(name, &error);
    if (id <= 0)
        return result(false, error);
    loadCategories();
    QVariantMap r = result(true, QString());
    r.insert(QStringLiteral("id"), id);
    return r;
}

QVariantMap ExpenseController::saveRecurring(const QVariantMap &form)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede cambiar los gastos fijos."));
    RecurringExpense r;
    r.id = form.value(QStringLiteral("id")).toInt();
    r.categoryId = form.value(QStringLiteral("categoryId")).toInt();
    r.description = form.value(QStringLiteral("description")).toString();
    r.dayOfMonth = form.value(QStringLiteral("day")).toInt();
    if (!Money::parseCents(form.value(QStringLiteral("amount")).toString().trimmed(), &r.amountCents))
        return result(false, tr("Importe no válido."));
    QString error;
    if (!m_service.saveRecurring(r, QDate::currentDate(), &error))
        return result(false, error);
    const int before = m_service.expenses().list(m_from, m_to, 0).size();
    reload();
    if (m_service.expenses().list(m_from, m_to, 0).size() != before)
        emit expensesChanged();
    return result(true, QString());
}

QVariantMap ExpenseController::setRecurringActive(int id, bool active)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede cambiar los gastos fijos."));
    QString error;
    if (!m_service.setRecurringActive(id, active, &error))
        return result(false, error);
    reload();
    emit expensesChanged();
    return result(true, QString());
}

QVariantMap ExpenseController::exportCsv(const QString &format)
{
    if (!isAdmin())
        return result(false, tr("Solo un administrador puede exportar."));
    const QString folder = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                               .filePath(QStringLiteral("PosCuba/Reportes"));
    const QString path = QDir(folder).filePath(QStringLiteral("gastos_%1_a_%2.csv")
                                                   .arg(m_from.toString(QStringLiteral("yyyy-MM-dd")),
                                                        m_to.toString(QStringLiteral("yyyy-MM-dd"))));
    const CsvWriter csv(format == QLatin1String("std") ? CsvWriter::Standard : CsvWriter::Spanish);
    QVector<QStringList> rows;
    for (const Expense &e : m_service.expenses().list(m_from, m_to, m_categoryFilter)) {
        rows.append({e.date.toString(QStringLiteral("yyyy-MM-dd")), e.categoryName, e.description,
                     csv.number(Money::formatCents(e.amountCents)),
                     e.fromCash ? tr("Caja") : tr("Fuera de caja"), e.recurringId > 0 ? tr("sí") : tr("no"), e.userName});
    }
    QString error;
    if (!QDir().mkpath(folder)
        || !csv.write(path, {tr("Fecha"), tr("Categoría"), tr("Descripción"), tr("Importe"), tr("Pagado de"),
                             tr("Gasto fijo"), tr("Usuario")},
                      rows, &error))
        return result(false, error.isEmpty() ? tr("No se pudo crear la carpeta.") : error);
    QVariantMap r = result(true, QString());
    r.insert(QStringLiteral("path"), QDir::toNativeSeparators(path));
    r.insert(QStringLiteral("count"), rows.size());
    return r;
}
