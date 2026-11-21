#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "core/Customer.h"

class CustomerRepository
{
public:
    explicit CustomerRepository(QSqlDatabase db);

    // search: nombre o telefono. onlyWithDebt: saldo > 0.
    QVector<Customer> list(const QString &search, bool includeInactive, bool onlyWithDebt);
    bool get(int id, Customer *out);
    bool nameExists(const QString &name, int exceptId);
    int insert(const Customer &c);    // id o -1
    bool update(const Customer &c);
    bool setActive(int id, bool active);
    qint64 balance(int customerId);

    bool insertEntry(int customerId, const QString &kind, qint64 amountCents, int saleId, int refundId,
                     int sessionId, const QString &method, const QString &note, int userId);
    QVector<CustomerEntry> entries(int customerId);   // con saldo acumulado, mas reciente primero

    // Cuentas por cobrar: suma de saldos positivos y cantidad de clientes con deuda.
    qint64 receivables(int *customersWithDebt);

    QString lastError() const { return m_lastError; }

private:
    QSqlDatabase m_db;
    QString m_lastError;
};
