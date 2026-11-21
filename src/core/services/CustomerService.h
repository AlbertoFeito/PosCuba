#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "core/Customer.h"
#include "data/repositories/CashRepository.h"
#include "data/repositories/CustomerRepository.h"

// Clientes y su cuenta de credito (fiado).
class CustomerService
{
public:
    explicit CustomerService(QSqlDatabase db);

    CustomerRepository &customers() { return m_customers; }

    static QString validate(const Customer &c);
    // Crea o actualiza (id > 0). No toca el saldo.
    bool save(Customer &c, QString *error);
    // No se desactiva un cliente que debe dinero.
    bool setActive(int id, bool active, QString *error);

    // Comprueba que el cliente pueda llevarse "amount" a credito (activo, dentro de su limite).
    bool canCharge(int customerId, qint64 amount, QString *error);

    // Abono: el cliente paga parte o toda su deuda. Necesita la caja abierta (el dinero entra a ella).
    // method: PaymentMethod::cash() o transfer(). Se registra en una transaccion.
    bool registerPayment(int customerId, qint64 amount, const QString &method, const QString &note, int userId,
                         QString *error);

private:
    QSqlDatabase m_db;
    CustomerRepository m_customers;
    CashRepository m_cash;
};
