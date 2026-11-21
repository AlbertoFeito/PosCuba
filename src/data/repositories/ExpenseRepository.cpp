#include "ExpenseRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
QString day(const QDate &d) { return d.toString(QStringLiteral("yyyy-MM-dd")); }

Expense readExpense(const QSqlQuery &q)
{
    Expense e;
    e.id = q.value(0).toInt();
    e.date = QDate::fromString(q.value(1).toString(), QStringLiteral("yyyy-MM-dd"));
    e.categoryId = q.value(2).toInt();
    e.categoryName = q.value(3).toString();
    e.description = q.value(4).toString();
    e.amountCents = q.value(5).toLongLong();
    e.fromCash = q.value(6).toInt() != 0;
    e.cashMovementId = q.value(7).toInt();
    e.recurringId = q.value(8).toInt();
    e.userName = q.value(9).toString();
    return e;
}

const char *const kExpenseSelect =
    "SELECT e.id, e.expense_date, e.category_id, c.name, e.description, e.amount_cents, e.from_cash, "
    "COALESCE(e.cash_movement_id, 0), COALESCE(e.recurring_id, 0), COALESCE(u.name, '') "
    "FROM expenses e JOIN expense_categories c ON c.id = e.category_id LEFT JOIN users u ON u.id = e.user_id ";

RecurringExpense readRecurring(const QSqlQuery &q)
{
    RecurringExpense r;
    r.id = q.value(0).toInt();
    r.categoryId = q.value(1).toInt();
    r.categoryName = q.value(2).toString();
    r.description = q.value(3).toString();
    r.amountCents = q.value(4).toLongLong();
    r.dayOfMonth = q.value(5).toInt();
    r.startMonth = q.value(6).toString();
    r.lastMonth = q.value(7).toString();
    r.active = q.value(8).toInt() != 0;
    return r;
}

const char *const kRecurringSelect =
    "SELECT r.id, r.category_id, c.name, r.description, r.amount_cents, r.day_of_month, r.start_month, "
    "COALESCE(r.last_month, ''), r.active FROM expense_recurring r JOIN expense_categories c ON c.id = r.category_id ";
} // namespace

ExpenseRepository::ExpenseRepository(QSqlDatabase db)
    : m_db(db)
{
}

QVector<ExpenseCategory> ExpenseRepository::categories(bool includeInactive)
{
    QVector<ExpenseCategory> out;
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT id, name, active FROM expense_categories %1 ORDER BY name COLLATE NOCASE")
                    .arg(includeInactive ? QString() : QStringLiteral("WHERE active = 1"))))
        return out;
    while (q.next()) {
        ExpenseCategory c;
        c.id = q.value(0).toInt();
        c.name = q.value(1).toString();
        c.active = q.value(2).toInt() != 0;
        out.append(c);
    }
    return out;
}

int ExpenseRepository::categoryId(const QString &name)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id FROM expense_categories WHERE name = :n COLLATE NOCASE"));
    q.bindValue(QStringLiteral(":n"), name.simplified());
    return q.exec() && q.next() ? q.value(0).toInt() : 0;
}

int ExpenseRepository::insertCategory(const QString &name)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO expense_categories (name) VALUES (:n)"));
    q.bindValue(QStringLiteral(":n"), name.simplified());
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

QVector<Expense> ExpenseRepository::list(const QDate &from, const QDate &to, int categoryId)
{
    QVector<Expense> out;
    QSqlQuery q(m_db);
    q.prepare(QString::fromLatin1(kExpenseSelect)
              + QStringLiteral("WHERE e.expense_date >= :f AND e.expense_date <= :t %1 "
                               "ORDER BY e.expense_date DESC, e.id DESC")
                    .arg(categoryId > 0 ? QStringLiteral("AND e.category_id = :c") : QString()));
    q.bindValue(QStringLiteral(":f"), day(from));
    q.bindValue(QStringLiteral(":t"), day(to));
    if (categoryId > 0)
        q.bindValue(QStringLiteral(":c"), categoryId);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next())
        out.append(readExpense(q));
    return out;
}

QVector<ExpenseTotal> ExpenseRepository::totals(const QDate &from, const QDate &to)
{
    QVector<ExpenseTotal> out;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT c.id, c.name, SUM(e.amount_cents), COUNT(*) FROM expenses e "
                             "JOIN expense_categories c ON c.id = e.category_id "
                             "WHERE e.expense_date >= :f AND e.expense_date <= :t "
                             "GROUP BY c.id ORDER BY SUM(e.amount_cents) DESC"));
    q.bindValue(QStringLiteral(":f"), day(from));
    q.bindValue(QStringLiteral(":t"), day(to));
    if (q.exec()) {
        while (q.next()) {
            ExpenseTotal t;
            t.categoryId = q.value(0).toInt();
            t.categoryName = q.value(1).toString();
            t.amountCents = q.value(2).toLongLong();
            t.count = q.value(3).toInt();
            out.append(t);
        }
    }
    return out;
}

bool ExpenseRepository::get(int id, Expense *out)
{
    QSqlQuery q(m_db);
    q.prepare(QString::fromLatin1(kExpenseSelect) + QStringLiteral("WHERE e.id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec() || !q.next())
        return false;
    if (out)
        *out = readExpense(q);
    return true;
}

int ExpenseRepository::insert(const Expense &e, int userId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO expenses (expense_date, category_id, description, amount_cents, from_cash, cash_movement_id, "
        "recurring_id, user_id) VALUES (:d, :c, :de, :a, :fc, :m, :r, :u)"));
    q.bindValue(QStringLiteral(":d"), day(e.date));
    q.bindValue(QStringLiteral(":c"), e.categoryId);
    q.bindValue(QStringLiteral(":de"), e.description);
    q.bindValue(QStringLiteral(":a"), e.amountCents);
    q.bindValue(QStringLiteral(":fc"), e.fromCash ? 1 : 0);
    q.bindValue(QStringLiteral(":m"), e.cashMovementId > 0 ? QVariant(e.cashMovementId) : QVariant());
    q.bindValue(QStringLiteral(":r"), e.recurringId > 0 ? QVariant(e.recurringId) : QVariant());
    q.bindValue(QStringLiteral(":u"), userId > 0 ? QVariant(userId) : QVariant());
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool ExpenseRepository::remove(int id)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM expenses WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

int ExpenseRepository::movementSession(int movementId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT session_id FROM cash_movements WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), movementId);
    return q.exec() && q.next() ? q.value(0).toInt() : 0;
}

QVector<RecurringExpense> ExpenseRepository::recurring(bool includeInactive)
{
    QVector<RecurringExpense> out;
    QSqlQuery q(m_db);
    if (!q.exec(QString::fromLatin1(kRecurringSelect)
                + (includeInactive ? QString() : QStringLiteral("WHERE r.active = 1 "))
                + QStringLiteral("ORDER BY r.active DESC, r.day_of_month, r.description")))
        return out;
    while (q.next())
        out.append(readRecurring(q));
    return out;
}

bool ExpenseRepository::getRecurring(int id, RecurringExpense *out)
{
    QSqlQuery q(m_db);
    q.prepare(QString::fromLatin1(kRecurringSelect) + QStringLiteral("WHERE r.id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec() || !q.next())
        return false;
    if (out)
        *out = readRecurring(q);
    return true;
}

int ExpenseRepository::insertRecurring(const RecurringExpense &r)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO expense_recurring (category_id, description, amount_cents, day_of_month, "
                             "start_month, active) VALUES (:c, :d, :a, :dm, :s, :ac)"));
    q.bindValue(QStringLiteral(":c"), r.categoryId);
    q.bindValue(QStringLiteral(":d"), r.description);
    q.bindValue(QStringLiteral(":a"), r.amountCents);
    q.bindValue(QStringLiteral(":dm"), r.dayOfMonth);
    q.bindValue(QStringLiteral(":s"), r.startMonth);
    q.bindValue(QStringLiteral(":ac"), r.active ? 1 : 0);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool ExpenseRepository::updateRecurring(const RecurringExpense &r)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE expense_recurring SET category_id = :c, description = :d, amount_cents = :a, "
                             "day_of_month = :dm WHERE id = :id"));
    q.bindValue(QStringLiteral(":c"), r.categoryId);
    q.bindValue(QStringLiteral(":d"), r.description);
    q.bindValue(QStringLiteral(":a"), r.amountCents);
    q.bindValue(QStringLiteral(":dm"), r.dayOfMonth);
    q.bindValue(QStringLiteral(":id"), r.id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

bool ExpenseRepository::setRecurringActive(int id, bool active)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE expense_recurring SET active = :a WHERE id = :id"));
    q.bindValue(QStringLiteral(":a"), active ? 1 : 0);
    q.bindValue(QStringLiteral(":id"), id);
    return q.exec() && q.numRowsAffected() == 1;
}

bool ExpenseRepository::setRecurringLastMonth(int id, const QString &month)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE expense_recurring SET last_month = :m WHERE id = :id"));
    q.bindValue(QStringLiteral(":m"), month);
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}
