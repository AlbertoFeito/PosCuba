#include "PricingService.h"

#include <QCoreApplication>
#include <cmath>

#include "data/Transaction.h"

namespace {
QString tr(const char *s) { return QCoreApplication::translate("PricingService", s); }
} // namespace

PricingService::PricingService(QSqlDatabase db)
    : m_db(db)
    , m_products(db)
{
}

bool PricingService::parsePercent(const QString &text, int *basisPoints)
{
    QString t = text.trimmed();
    t.remove(QLatin1Char('%'));
    t.replace(QLatin1Char(','), QLatin1Char('.'));
    t = t.trimmed();
    if (t.startsWith(QLatin1Char('+')))
        t = t.mid(1);
    bool ok = false;
    const double v = t.toDouble(&ok);
    if (!ok || t.isEmpty() || v <= -100.0 || v > 1000.0)
        return false;
    // Como mucho dos decimales
    const int dot = t.indexOf(QLatin1Char('.'));
    if (dot >= 0 && t.size() - dot - 1 > 2)
        return false;
    if (basisPoints)
        *basisPoints = int(std::llround(v * 100.0));
    return true;
}

qint64 PricingService::newPrice(qint64 oldPrice, qint64 cost, const PriceChangeRequest &req)
{
    const qint64 base = req.mode == PriceChangeRequest::MarginOnCost ? cost : oldPrice;
    const long double raw = (long double)base * (10000 + req.basisPoints) / 10000.0L;
    qint64 cents = 0;
    if (req.stepCents > 0) {
        const long double steps = raw / req.stepCents;
        // Tolerancia: 120.00 * 1.10 no debe redondear a 135 por un error de coma flotante.
        const long double eps = 1e-9L;
        const qint64 n = req.rounding == PriceChangeRequest::RoundUp ? qint64(std::ceil((double)(steps - eps)))
                                                                     : qint64(std::llround((double)steps));
        cents = n * req.stepCents;
    } else {
        cents = qint64(std::llround((double)raw));
    }
    return qMax<qint64>(0, cents);
}

PricePreview PricingService::preview(const PriceChangeRequest &req)
{
    PricePreview out;
    ProductFilter f;
    f.categoryId = req.categoryId;
    f.includeInactive = req.includeInactive;
    f.limit = 100000;
    for (const Product &p : m_products.find(f)) {
        if (req.mode == PriceChangeRequest::MarginOnCost && p.costCents <= 0) {
            ++out.skippedNoCost;
            continue;
        }
        const qint64 np = newPrice(p.priceCents, p.costCents, req);
        if (np == p.priceCents) {
            ++out.unchanged;
            continue;
        }
        PriceChange c;
        c.productId = p.id;
        c.code = p.code;
        c.name = p.name;
        c.costCents = p.costCents;
        c.oldPrice = p.priceCents;
        c.newPrice = np;
        out.changes.append(c);
    }
    return out;
}

int PricingService::apply(const PriceChangeRequest &req, QString *error)
{
    Transaction tx(m_db);
    if (!tx.isActive()) {
        if (error)
            *error = tr("No se pudo iniciar la transacción.");
        return -1;
    }
    const PricePreview pv = preview(req);
    for (const PriceChange &c : pv.changes) {
        if (!m_products.setPrice(c.productId, c.newPrice)) {
            if (error)
                *error = tr("No se pudo cambiar el precio de «%1»: %2").arg(c.name, m_products.lastError());
            return -1;
        }
    }
    if (!tx.commit()) {
        if (error)
            *error = tr("No se pudo confirmar el cambio de precios.");
        return -1;
    }
    return pv.changes.size();
}
