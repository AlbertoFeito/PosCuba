#include "RefundController.h"

#include <QDateTime>

#include "core/Money.h"
#include "core/User.h"
#include "core/services/UserService.h"

namespace {
QVariantMap result(bool ok, const QString &error)
{
    return QVariantMap{{QStringLiteral("ok"), ok}, {QStringLiteral("error"), error}};
}

QString shortDate(const QString &sqlDate)
{
    const QDateTime dt = QDateTime::fromString(sqlDate, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    return dt.isValid() ? dt.toString(QStringLiteral("dd/MM/yyyy HH:mm")) : sqlDate;
}
} // namespace

RefundController::RefundController(QSqlDatabase db, const Session *session, UserService *users, QObject *parent)
    : QObject(parent)
    , m_service(db)
    , m_session(session)
    , m_users(users)
{
}

QVariantMap RefundController::load(int saleId)
{
    SaleRefundState st;
    if (!m_service.state(saleId, &st))
        return result(false, tr("No se encontró la venta."));
    QVariantList items;
    for (const RefundableItem &it : st.items) {
        items.append(QVariantMap{
            {QStringLiteral("id"), it.saleItemId},
            {QStringLiteral("name"), it.name},
            {QStringLiteral("unit"), it.unit},
            {QStringLiteral("soldText"), Money::formatMilli(it.soldMilli)},
            {QStringLiteral("returnedText"), Money::formatMilli(it.returnedMilli)},
            {QStringLiteral("available"), double(it.availableMilli())},
            {QStringLiteral("availableText"), Money::formatMilli(it.availableMilli())},
            {QStringLiteral("priceText"), Money::formatCents(it.priceCents)},
        });
    }
    QVariantMap out = result(true, QString());
    out.insert(QStringLiteral("saleId"), st.saleId);
    out.insert(QStringLiteral("number"), st.number);
    out.insert(QStringLiteral("dateText"), shortDate(st.createdAt));
    out.insert(QStringLiteral("totalText"), Money::formatCents(st.total));
    out.insert(QStringLiteral("refundedText"), Money::formatCents(st.refunded));
    out.insert(QStringLiteral("hasRefunds"), st.refunded > 0);
    out.insert(QStringLiteral("remainingText"), Money::formatCents(qMax<qint64>(0, st.total - st.refunded)));
    out.insert(QStringLiteral("voided"), st.voided());
    out.insert(QStringLiteral("items"), items);
    return out;
}

bool RefundController::parseQtys(const QVariantMap &qtys, QMap<int, qint64> *out, QString *error) const
{
    for (auto it = qtys.constBegin(); it != qtys.constEnd(); ++it) {
        const QString text = it.value().toString().trimmed();
        if (text.isEmpty())
            continue;
        qint64 milli = 0;
        if (!Money::parseMilli(text, &milli) || milli < 0) {
            if (error)
                *error = tr("Cantidad no válida: «%1».").arg(text);
            return false;
        }
        if (milli > 0)
            out->insert(it.key().toInt(), milli);
    }
    return true;
}

QVariantMap RefundController::preview(int saleId, const QVariantMap &qtys, bool all)
{
    SaleRefundState st;
    if (!m_service.state(saleId, &st))
        return result(false, tr("No se encontró la venta."));
    QMap<int, qint64> qty;
    QString error;
    if (!all && !parseQtys(qtys, &qty, &error))
        return result(false, error);
    RefundCalc calc;
    if (!RefundService::calculate(st, qty, all, &calc, &error)) {
        QVariantMap r = result(false, error);
        r.insert(QStringLiteral("totalText"), Money::formatCents(0));
        return r;
    }
    QVariantMap r = result(true, QString());
    r.insert(QStringLiteral("totalText"), Money::formatCents(calc.total));
    r.insert(QStringLiteral("completes"), calc.completesSale);
    return r;
}

QVariantMap RefundController::refund(const QVariantMap &form)
{
    if (!m_session || !m_session->isValid())
        return result(false, tr("No hay un usuario activo."));

    RefundRequest req;
    req.saleId = form.value(QStringLiteral("saleId")).toInt();
    req.all = form.value(QStringLiteral("all")).toBool();
    req.method = form.value(QStringLiteral("method")).toString();
    req.reason = form.value(QStringLiteral("reason")).toString();
    req.restock = form.value(QStringLiteral("restock"), true).toBool();
    req.userId = m_session->userId;
    QString error;
    if (!req.all && !parseQtys(form.value(QStringLiteral("qtys")).toMap(), &req.qtyByItem, &error))
        return result(false, error);
    if (req.reason.trimmed().isEmpty())
        return result(false, tr("Indica el motivo de la devolución."));

    // Autorizacion
    if (m_session->isAdmin()) {
        req.authorizedBy = m_session->userId;
    } else {
        const int adminId = form.value(QStringLiteral("adminId")).toInt();
        if (!m_users || !m_users->verifyAdmin(adminId, form.value(QStringLiteral("adminPin")).toString(), &error))
            return result(false, error.isEmpty() ? tr("Falta la autorización de un administrador.") : error);
        req.authorizedBy = adminId;
    }

    const RefundResult r = m_service.refund(req);
    if (!r.ok)
        return result(false, r.error);
    m_lastSaleId = req.saleId;
    emit refunded();
    QVariantMap out = result(true, QString());
    out.insert(QStringLiteral("number"), r.number);
    out.insert(QStringLiteral("totalText"), Money::formatCents(r.total));
    out.insert(QStringLiteral("voided"), r.voided);
    out.insert(QStringLiteral("method"), req.method);
    return out;
}

QVariantList RefundController::admins() const
{
    QVariantList out;
    if (!m_users)
        return out;
    for (const User &u : m_users->list(false)) {
        if (u.role == Role::admin() && u.hasPin)
            out.append(QVariantMap{{QStringLiteral("id"), u.id}, {QStringLiteral("name"), u.name}});
    }
    return out;
}
