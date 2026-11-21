#pragma once

#include <QString>

// Cliente con cuenta de credito (fiado). Importes en centavos de la moneda base.
struct Customer
{
    int id = 0;
    QString name;
    QString phone;
    QString note;
    qint64 creditLimitCents = 0;   // 0 = sin limite
    bool active = true;
    qint64 balanceCents = 0;       // lo que debe (negativo = saldo a favor)
    QString lastActivity;          // fecha del ultimo movimiento de su cuenta
};

namespace CustomerEntryKind {
inline QString charge() { return QStringLiteral("charge"); }     // venta a credito
inline QString payment() { return QStringLiteral("payment"); }   // abono
inline QString refund() { return QStringLiteral("refund"); }     // devolucion a la cuenta
}

struct CustomerEntry
{
    int id = 0;
    QString kind;
    qint64 amountCents = 0;
    int saleNumber = 0;
    int refundNumber = 0;
    QString method;
    QString note;
    QString userName;
    QString createdAt;
    qint64 balanceAfter = 0;
};
