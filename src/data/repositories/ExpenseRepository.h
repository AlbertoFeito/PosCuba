#pragma once

#include <QDate>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "core/Expense.h"

class ExpenseRepository
{
public:
    explicit ExpenseRepository(QSqlDatabase db);

    QVector<ExpenseCategory> categories(bool includeInactive = false);
    int categoryId(const QString &name);          // 0 si no existe
    int insertCategory(const QString &name);       // id o -1

    // categoryId 0 = todas. Mas reciente primero.
    QVector<Expense> list(const QDate &from, const QDate &to, int categoryId);
    QVector<ExpenseTotal> totals(const QDate &from, const QDate &to);
    bool get(int id, Expense *out);
    int insert(const Expense &e, int userId);      // id o -1
    bool remove(int id);
    // Sesion de caja del retiro de un gasto pagado de la caja (0 si no hay).
    int movementSession(int movementId);

    QVector<RecurringExpense> recurring(bool includeInactive = true);
    bool getRecurring(int id, RecurringExpense *out);
    int insertRecurring(const RecurringExpense &r);   // id o -1
    bool updateRecurring(const RecurringExpense &r);
    bool setRecurringActive(int id, bool active);
    bool setRecurringLastMonth(int id, const QString &month);

    QString lastError() const { return m_lastError; }

private:
    QSqlDatabase m_db;
    QString m_lastError;
};
