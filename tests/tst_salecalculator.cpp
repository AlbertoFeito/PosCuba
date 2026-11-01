#include <QtTest>

#include "core/SaleCalculator.h"

namespace {
CartLine line(qint64 qtyMilli, qint64 price)
{
    CartLine l;
    l.productId = 1;
    l.name = QStringLiteral("X");
    l.qtyMilli = qtyMilli;
    l.priceCents = price;
    return l;
}
Payment pay(const QString &method, qint64 amount, qint64 rate = 1000, const QString &cur = QStringLiteral("CUP"))
{
    Payment p;
    p.method = method;
    p.amountCents = amount;
    p.rateMilli = rate;
    p.currency = cur;
    return p;
}
} // namespace

class TstSaleCalculator : public QObject
{
    Q_OBJECT

private slots:
    void lineTotals()
    {
        QCOMPARE(SaleCalculator::lineTotal(line(3000, 12000)), qint64(36000));   // 3 x 120.00
        QCOMPARE(SaleCalculator::lineTotal(line(250, 25000)), qint64(6250));     // 0.25 lb x 250.00
        QCOMPARE(SaleCalculator::lineTotal(line(333, 1000)), qint64(333));       // 0.333 x 10.00 = 3.33
        QCOMPARE(SaleCalculator::lineTotal(line(1, 500)), qint64(1));            // 0.001 x 5.00 = 0.005 -> 0.01
        QCOMPARE(SaleCalculator::lineTotal(line(1, 499)), qint64(0));            // 0.00499 -> 0.00
    }

    void subtotalAndDiscount()
    {
        const QVector<CartLine> lines = {line(2000, 12000), line(1500, 8000)}; // 240 + 120
        SaleTotals t = SaleCalculator::compute(lines, 3600, {});
        QCOMPARE(t.subtotal, qint64(36000));
        QCOMPARE(t.discount, qint64(3600));
        QCOMPARE(t.total, qint64(32400));
        QCOMPARE(t.remaining, qint64(32400));

        QCOMPARE(SaleCalculator::percentOf(36000, 1000), qint64(3600));   // 10 %
        QCOMPARE(SaleCalculator::percentOf(33333, 1250), qint64(4167));   // 12.5 % de 333.33 = 41.67
        // El descuento nunca supera el subtotal.
        t = SaleCalculator::compute(lines, 999999, {});
        QCOMPARE(t.total, qint64(0));
    }

    void cashChange()
    {
        const QVector<CartLine> lines = {line(1000, 36500)};
        const SaleTotals t = SaleCalculator::compute(lines, 0, {pay(PaymentMethod::cash(), 50000)});
        QCOMPARE(t.paid, qint64(50000));
        QCOMPARE(t.change, qint64(13500));
        QCOMPARE(t.remaining, qint64(0));
        QVERIFY(SaleCalculator::validatePayments(t, {pay(PaymentMethod::cash(), 50000)}).isEmpty());
    }

    void insufficientPayment()
    {
        const QVector<CartLine> lines = {line(1000, 36500)};
        const QVector<Payment> p = {pay(PaymentMethod::cash(), 30000)};
        const SaleTotals t = SaleCalculator::compute(lines, 0, p);
        QCOMPARE(t.remaining, qint64(6500));
        QCOMPARE(t.change, qint64(0));
        QVERIFY(SaleCalculator::validatePayments(t, p).contains(QStringLiteral("65.00")));
    }

    void mixedPayment()
    {
        const QVector<CartLine> lines = {line(1000, 100000)}; // 1000.00
        const QVector<Payment> p = {pay(PaymentMethod::transfer(), 70000), pay(PaymentMethod::cash(), 40000)};
        const SaleTotals t = SaleCalculator::compute(lines, 0, p);
        QCOMPARE(t.transferPaid, qint64(70000));
        QCOMPARE(t.cashPaid, qint64(40000));
        QCOMPARE(t.change, qint64(10000));
        QVERIFY(SaleCalculator::validatePayments(t, p).isEmpty());
    }

    void transferCannotExceedTotal()
    {
        const QVector<CartLine> lines = {line(1000, 100000)};
        const QVector<Payment> p = {pay(PaymentMethod::transfer(), 120000)};
        const SaleTotals t = SaleCalculator::compute(lines, 0, p);
        QVERIFY(!SaleCalculator::validatePayments(t, p).isEmpty());
    }

    void foreignCurrency()
    {
        // 1 USD = 400 CUP. Total 1000.00 CUP; pagan 3 USD (1200 CUP) -> vuelto 200 CUP.
        const QVector<CartLine> lines = {line(1000, 100000)};
        const QVector<Payment> p = {pay(PaymentMethod::cash(), 300, 400000, QStringLiteral("USD"))};
        QCOMPARE(SaleCalculator::toBase(300, 400000), qint64(120000));
        const SaleTotals t = SaleCalculator::compute(lines, 0, p);
        QCOMPARE(t.change, qint64(20000));
        QVERIFY(SaleCalculator::validatePayments(t, p).isEmpty());
    }

    void cartValidation()
    {
        QVERIFY(!SaleCalculator::validateCart({}, 0).isEmpty());
        QVERIFY(!SaleCalculator::validateCart({line(0, 100)}, 0).isEmpty());
        QVERIFY(!SaleCalculator::validateCart({line(1000, 100)}, 101).isEmpty());
        QVERIFY(!SaleCalculator::validateCart({line(SaleCalculator::MaxQtyMilli + 1, 100)}, 0).isEmpty());
        QVERIFY(SaleCalculator::validateCart({line(1000, 100)}, 100).isEmpty());
    }
};

QTEST_GUILESS_MAIN(TstSaleCalculator)
#include "tst_salecalculator.moc"
