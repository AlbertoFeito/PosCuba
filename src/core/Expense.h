#pragma once

#include <QDate>
#include <QString>

struct ExpenseCategory
{
    int id = 0;
    QString name;
    bool active = true;
};

struct Expense
{
    int id = 0;
    QDate date;
    int categoryId = 0;
    QString categoryName;
    QString description;
    qint64 amountCents = 0;
    bool fromCash = false;        // se pago con dinero de la caja (genera un retiro)
    int cashMovementId = 0;
    int recurringId = 0;          // generado por un gasto fijo
    QString userName;
};

// Gasto fijo mensual: se registra solo cada mes el dia indicado (o el ultimo dia si el mes es mas corto).
struct RecurringExpense
{
    int id = 0;
    int categoryId = 0;
    QString categoryName;
    QString description;
    qint64 amountCents = 0;
    int dayOfMonth = 1;
    QString startMonth;           // "yyyy-MM"
    QString lastMonth;            // ultimo mes generado ("" = ninguno)
    bool active = true;
};

struct ExpenseTotal
{
    int categoryId = 0;
    QString categoryName;
    qint64 amountCents = 0;
    int count = 0;
};
