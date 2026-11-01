#include "SaleController.h"

#include "core/Money.h"
#include "core/SaleCalculator.h"

namespace {
QVariantMap addResult(bool ok, int row, const QString &error, bool notFound = false)
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("row"), row},
            {QStringLiteral("error"), error}, {QStringLiteral("notFound"), notFound}};
}
} // namespace

SaleController::SaleController(QSqlDatabase db, const Session *session, QObject *parent)
    : QObject(parent)
    , m_db(db)
    , m_session(session)
    , m_productsRepo(db)
    , m_currencyRepo(db)
    , m_saleService(db)
    , m_cashService(db)
{
    connect(&m_cart, &CartModel::contentChanged, this, &SaleController::totalsChanged);
}

void SaleController::setSearchText(const QString &text)
{
    if (m_searchText == text)
        return;
    m_searchText = text;
    emit searchTextChanged();
    ProductFilter f;
    f.text = text;
    f.limit = 30;
    m_results.setProducts(text.trimmed().isEmpty() ? QVector<Product>() : m_productsRepo.find(f));
}

qint64 SaleController::subtotal() const
{
    return SaleCalculator::subtotal(m_cart.lines());
}

qint64 SaleController::discount() const
{
    const qint64 sub = subtotal();
    const qint64 d = m_discountPercent ? SaleCalculator::percentOf(sub, m_discountValue) : m_discountValue;
    return qBound<qint64>(0, d, sub);
}

qint64 SaleController::total() const
{
    return subtotal() - discount();
}

QString SaleController::subtotalText() const { return Money::formatCents(subtotal()); }
QString SaleController::discountText() const { return Money::formatCents(discount()); }
QString SaleController::totalText() const { return Money::formatCents(total()); }

QString SaleController::discountLabel() const
{
    if (m_discountValue <= 0)
        return QString();
    if (m_discountPercent) {
        QString pct = Money::formatCents(m_discountValue);
        if (pct.endsWith(QLatin1String(".00")))
            pct.chop(3);
        return pct + QStringLiteral(" %");
    }
    return QString();
}

QVariantMap SaleController::addLine(const Product &p, qint64 qtyMilli)
{
    if (qtyMilli <= 0 || qtyMilli > SaleCalculator::MaxQtyMilli)
        return addResult(false, -1, tr("Cantidad no válida."));
    CartLine l;
    l.productId = p.id;
    l.code = p.code;
    l.name = p.name;
    l.unit = p.unit;
    l.qtyMilli = qtyMilli;
    l.priceCents = p.priceCents;
    l.costCents = p.costCents;
    l.stockMilli = p.stockMilli;
    return addResult(true, m_cart.add(l), QString());
}

QVariantMap SaleController::addByCode(const QString &rawText)
{
    QString text = rawText.trimmed();
    qint64 qty = 1000;
    const int star = text.indexOf(QLatin1Char('*'));
    if (star > 0) {
        if (!Money::parseMilli(text.left(star), &qty) || qty <= 0)
            return addResult(false, -1, tr("Cantidad no válida antes de «*»."));
        text = text.mid(star + 1).trimmed();
    }
    Product p;
    if (!m_productsRepo.findExact(text, &p))
        return addResult(false, -1, tr("No existe un producto con el código «%1».").arg(text), true);
    return addLine(p, qty);
}

QVariantMap SaleController::addProduct(int productId, const QString &qtyText)
{
    qint64 qty = 0;
    if (!Money::parseMilli(qtyText, &qty))
        return addResult(false, -1, tr("Cantidad no válida."));
    Product p;
    if (!m_productsRepo.get(productId, &p) || !p.active)
        return addResult(false, -1, tr("El producto no está disponible."));
    return addLine(p, qty);
}

QVariantMap SaleController::setQty(int row, const QString &qtyText)
{
    qint64 qty = 0;
    if (!Money::parseMilli(qtyText, &qty) || qty <= 0)
        return addResult(false, row, tr("Cantidad no válida (mayor que cero, hasta 3 decimales)."));
    if (!m_cart.setQty(row, qty))
        return addResult(false, row, tr("Cantidad demasiado grande."));
    return addResult(true, row, QString());
}

bool SaleController::changeQty(int row, int deltaUnits)
{
    const qint64 next = m_cart.qtyAt(row) + qint64(deltaUnits) * 1000;
    if (next <= 0)
        return false;
    return m_cart.setQty(row, next);
}

QString SaleController::qtyTextAt(int row) const
{
    return Money::formatMilli(m_cart.qtyAt(row));
}

void SaleController::removeLine(int row)
{
    m_cart.remove(row);
    if (m_cart.count() == 0 && m_discountValue != 0) {
        m_discountValue = 0;
        emit totalsChanged();
    }
}

void SaleController::clearCart()
{
    m_cart.clear();
    m_discountValue = 0;
    m_discountPercent = false;
    emit totalsChanged();
}

QVariantMap SaleController::setDiscount(const QString &value, bool percent)
{
    const QString v = value.trimmed();
    qint64 parsed = 0;
    if (!v.isEmpty() && !Money::parseCents(v, &parsed))
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("Valor no válido.")}};
    if (percent && parsed > 10000)
        return {{QStringLiteral("ok"), false}, {QStringLiteral("error"), tr("El porcentaje no puede superar 100.")}};
    if (!percent && parsed > subtotal())
        return {{QStringLiteral("ok"), false},
                {QStringLiteral("error"), tr("El descuento no puede superar el subtotal (%1).").arg(subtotalText())}};
    m_discountPercent = percent;
    m_discountValue = parsed;
    emit totalsChanged();
    return {{QStringLiteral("ok"), true}, {QStringLiteral("error"), QString()}};
}

bool SaleController::buildPayments(const QVariantMap &form, QVector<Payment> *out, QString *error) const
{
    const QString cashText = form.value(QStringLiteral("cash")).toString().trimmed();
    const QString transferText = form.value(QStringLiteral("transfer")).toString().trimmed();
    QString cashCurrency = form.value(QStringLiteral("cashCurrency")).toString();
    if (cashCurrency.isEmpty())
        cashCurrency = m_baseCurrency;

    qint64 cash = 0;
    qint64 transfer = 0;
    if (!cashText.isEmpty() && !Money::parseCents(cashText, &cash)) {
        *error = tr("Importe en efectivo no válido.");
        return false;
    }
    if (!transferText.isEmpty() && !Money::parseCents(transferText, &transfer)) {
        *error = tr("Importe de transferencia no válido.");
        return false;
    }

    Currency cur;
    if (!m_currencyRepo.get(cashCurrency, &cur)) {
        *error = tr("Moneda no disponible.");
        return false;
    }
    if (cash > 0) {
        Payment p;
        p.method = PaymentMethod::cash();
        p.currency = cur.code;
        p.amountCents = cash;
        p.rateMilli = cur.isBase ? 1000 : cur.rateMilli;
        out->append(p);
    }
    if (transfer > 0) {
        Payment p;
        p.method = PaymentMethod::transfer();
        p.currency = m_baseCurrency;
        p.amountCents = transfer;
        p.reference = form.value(QStringLiteral("reference")).toString();
        out->append(p);
    }
    return true;
}

QVariantMap SaleController::previewPayment(const QVariantMap &form) const
{
    QVector<Payment> payments;
    QString error;
    QVariantMap r;
    if (!buildPayments(form, &payments, &error)) {
        r.insert(QStringLiteral("valid"), false);
        r.insert(QStringLiteral("error"), error);
        return r;
    }
    const SaleTotals t = SaleCalculator::compute(m_cart.lines(), discount(), payments);
    error = SaleCalculator::validatePayments(t, payments);
    r.insert(QStringLiteral("valid"), error.isEmpty());
    r.insert(QStringLiteral("error"), error);
    r.insert(QStringLiteral("paidText"), Money::formatCents(t.paid));
    r.insert(QStringLiteral("changeText"), Money::formatCents(t.change));
    r.insert(QStringLiteral("remainingText"), Money::formatCents(t.remaining));
    r.insert(QStringLiteral("hasChange"), t.change > 0);
    r.insert(QStringLiteral("hasRemaining"), t.remaining > 0);
    return r;
}

QVariantMap SaleController::checkout(const QVariantMap &form)
{
    QVariantMap r;
    QVector<Payment> payments;
    QString error;
    if (!buildPayments(form, &payments, &error)) {
        r.insert(QStringLiteral("ok"), false);
        r.insert(QStringLiteral("error"), error);
        return r;
    }

    const CheckoutResult res = m_saleService.checkout(m_cart.lines(), discount(), payments,
                                                      m_session ? m_session->userId : 0,
                                                      form.value(QStringLiteral("allowNegative")).toBool());
    r.insert(QStringLiteral("ok"), res.ok);
    r.insert(QStringLiteral("error"), res.error);
    r.insert(QStringLiteral("stockIssues"), QVariant(res.stockIssues));
    r.insert(QStringLiteral("number"), res.number);
    r.insert(QStringLiteral("saleId"), res.saleId);
    r.insert(QStringLiteral("changeText"), Money::formatCents(res.change));
    r.insert(QStringLiteral("hasChange"), res.change > 0);
    if (res.ok) {
        clearCart();
        emit saleCompleted(res.number);
    }
    return r;
}

void SaleController::refreshSession()
{
    const bool open = m_cashService.current(nullptr);
    if (open != m_hasOpenSession) {
        m_hasOpenSession = open;
        emit sessionChanged();
    }
}

void SaleController::refreshCurrencies()
{
    m_currencies.clear();
    for (const Currency &c : m_currencyRepo.active()) {
        if (c.isBase)
            m_baseCurrency = c.code;
        m_currencies.append(QVariantMap{
            {QStringLiteral("code"), c.code},
            {QStringLiteral("symbol"), c.symbol},
            {QStringLiteral("isBase"), c.isBase},
            {QStringLiteral("rateText"), Money::formatMilli(c.rateMilli)},
        });
    }
    emit currenciesChanged();
}
