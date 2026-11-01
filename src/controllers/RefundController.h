#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <QVariantMap>

#include "core/Session.h"
#include "core/services/RefundService.h"

class UserService;

// Devoluciones y anulaciones, expuesto a QML como "refundCtrl".
// Un cajero necesita que un administrador autorice con su PIN; un administrador autoriza solo.
class RefundController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int lastSaleId READ lastSaleId NOTIFY refunded)

public:
    RefundController(QSqlDatabase db, const Session *session, UserService *users, QObject *parent = nullptr);

    int lastSaleId() const { return m_lastSaleId; }

    // { ok, error, saleId, number, dateText, totalText, refundedText, remainingText, voided,
    //   items: [{ id, name, unit, soldText, returnedText, available, availableText, priceText }] }
    Q_INVOKABLE QVariantMap load(int saleId);
    // qtys: { "<id linea>": "texto cantidad" }. Salida: { ok, error, totalText, completes }
    Q_INVOKABLE QVariantMap preview(int saleId, const QVariantMap &qtys, bool all);
    // form: { saleId, qtys, all, method ("cash"|"transfer"), reason, restock, adminId, adminPin }
    // Salida: { ok, error, number, totalText, voided }
    Q_INVOKABLE QVariantMap refund(const QVariantMap &form);
    // Administradores activos con PIN: [{ id, name }]
    Q_INVOKABLE QVariantList admins() const;

signals:
    void refunded();

private:
    bool parseQtys(const QVariantMap &qtys, QMap<int, qint64> *out, QString *error) const;

    RefundService m_service;
    const Session *m_session;
    UserService *m_users;
    int m_lastSaleId = 0;
};
