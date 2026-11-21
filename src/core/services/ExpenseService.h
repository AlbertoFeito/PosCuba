#pragma once

#include <QDate>
#include <QSqlDatabase>
#include <QString>

#include "core/Expense.h"
#include "core/services/CashService.h"
#include "data/repositories/CashRepository.h"
#include "data/repositories/ExpenseRepository.h"

// Gastos del negocio. Un gasto pagado con dinero de la caja genera un retiro en la caja abierta
// (y solo puede ser de hoy). Los gastos fijos mensuales se registran solos con generateDue().
class ExpenseService
{
public:
    explicit ExpenseService(QSqlDatabase db);

    ExpenseRepository &expenses() { return m_expenses; }

    static QString validate(const Expense &e, const QDate &today);
    int ensureCategory(const QString &name, QString *error);

    bool add(Expense &e, int userId, QString *error);
    // Un gasto de caja solo se borra mientras su caja siga abierta (se borra tambien el retiro).
    bool remove(int id, QString *error);

    static QString validateRecurring(const RecurringExpense &r);
    bool saveRecurring(RecurringExpense &r, const QDate &today, QString *error);
    // Al reactivar no se recuperan los meses en pausa: sigue desde el mes de "today".
    bool setRecurringActive(int id, bool active, QString *error, const QDate &today = QDate::currentDate());

    // Registra los gastos fijos que tocan hasta "today". Devuelve cuantos registro (o -1 si fallo).
    int generateDue(const QDate &today, QString *error = nullptr);

private:
    QSqlDatabase m_db;
    ExpenseRepository m_expenses;
    CashRepository m_cashRepo;
    CashService m_cash;
};
