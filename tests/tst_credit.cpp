#include <QtTest>

#include <QSqlQuery>

#include "core/services/CashService.h"
#include "core/services/CustomerService.h"
#include "core/services/InventoryService.h"
#include "core/services/ProductService.h"
#include "core/services/RefundService.h"
#include "core/services/ReportService.h"
#include "core/services/SaleService.h"
#include "core/TicketFormatter.h"
#include "data/Database.h"
#include "data/Migrator.h"
#include "data/repositories/SaleRepository.h"

namespace {
qint64 scalar(const QString &sql)
{
    QSqlQuery q(Database::connection());
    if (q.exec(sql) && q.next())
        return q.value(0).toLongLong();
    return -999;
}
Payment pay(const QString &method, qint64 amount)
{
    Payment p;
    p.method = method;
    p.currency = QStringLiteral("CUP");
    p.amountCents = amount;
    return p;
}
} // namespace

// Clientes, venta a credito (fiado), abonos y su efecto en caja, devoluciones y reportes.
class TstCredit : public QObject
{
    Q_OBJECT

    int m_user = 0;
    Product m_cola;   // 120.00
    int m_ana = 0;    // limite 1000.00

    CartLine line(qint64 qtyMilli)
    {
        CartLine l;
        l.productId = m_cola.id;
        l.name = m_cola.name;
        l.unit = m_cola.unit;
        l.qtyMilli = qtyMilli;
        l.priceCents = m_cola.priceCents;
        l.costCents = m_cola.costCents;
        return l;
    }

    CheckoutResult sell(qint64 qty, const QVector<Payment> &payments, int customer)
    {
        SaleService s(Database::connection());
        return s.checkout({line(qty)}, 0, payments, m_user, true, customer);
    }

private slots:
    void init()
    {
        Database::close();
        QString error;
        QVERIFY2(Database::open(QStringLiteral(":memory:"), &error), qPrintable(error));
        QVERIFY2(Migrator::run(Database::connection(), &error), qPrintable(error));
        m_user = int(scalar(QStringLiteral("SELECT id FROM users LIMIT 1")));
        ProductService ps(Database::connection());
        m_cola = Product();
        m_cola.code = QStringLiteral("1");
        m_cola.name = QStringLiteral("Cola");
        m_cola.priceCents = 12000;
        m_cola.costCents = 8000;
        QVERIFY(ps.save(m_cola, &error));
        InventoryService inv(Database::connection());
        QVERIFY(inv.registerPurchase(m_cola.id, 100000, -1, false, QString(), m_user, &error));
        CustomerService cs(Database::connection());
        Customer ana;
        ana.name = QStringLiteral("Ana Pérez");
        ana.phone = QStringLiteral("5355 1234");
        ana.creditLimitCents = 100000;
        QVERIFY2(cs.save(ana, &error), qPrintable(error));
        m_ana = ana.id;
        CashService cash(Database::connection());
        QVERIFY(cash.open(50000, m_user, nullptr, &error));
    }

    void cleanupTestCase() { Database::close(); }

    void customerValidation()
    {
        CustomerService cs(Database::connection());
        QString error;
        Customer dup;
        dup.name = QStringLiteral("  ana   pérez ");
        QVERIFY(!cs.save(dup, &error));       // nombre repetido (sin importar mayusculas ni espacios)
        QVERIFY(error.contains(QStringLiteral("Ya existe")));
        Customer empty;
        QVERIFY(!cs.save(empty, &error));
    }

    void partialCreditSale()
    {
        // 5 colas = 600.00: paga 200 en efectivo, 400 a credito
        const CheckoutResult r = sell(5000, {pay(PaymentMethod::cash(), 20000), pay(PaymentMethod::credit(), 40000)}, m_ana);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.change, qint64(0));
        QCOMPARE(scalar(QStringLiteral("SELECT credit_cents FROM sales")), qint64(40000));
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM payments")), qint64(1));
        CustomerService cs(Database::connection());
        QCOMPARE(cs.customers().balance(m_ana), qint64(40000));

        CashService cash(Database::connection());
        CashSession s;
        QVERIFY(cash.current(&s));
        const CashSummary sum = cash.summary(s);
        QCOMPARE(sum.salesTotal, qint64(60000));
        QCOMPARE(sum.creditSales, qint64(40000));
        QCOMPARE(sum.expected.first().expectedCents, qint64(50000 + 20000));

        SaleRepository repo(Database::connection());
        SaleDetail d;
        QVERIFY(repo.load(r.saleId, &d));
        QCOMPARE(d.credit, qint64(40000));
        QCOMPARE(d.customerName, QStringLiteral("Ana Pérez"));
        TicketInfo info;
        info.baseCurrency = QStringLiteral("CUP");
        const QString ticket = TicketFormatter::format(d, info, 32).join(QLatin1Char('\n'));
        QVERIFY(ticket.contains(QStringLiteral("A crédito:")));
        QVERIFY(ticket.contains(QStringLiteral("Ana Pérez")));
    }

    void creditNeedsCustomerAndRespectsLimit()
    {
        CheckoutResult r = sell(1000, {pay(PaymentMethod::credit(), 12000)}, 0);
        QVERIFY(!r.ok);
        QVERIFY(r.error.contains(QStringLiteral("cliente")));
        // Limite 1000.00: 9 colas = 1080.00 a credito lo supera
        r = sell(9000, {pay(PaymentMethod::credit(), 108000)}, m_ana);
        QVERIFY(!r.ok);
        QVERIFY(r.error.contains(QStringLiteral("límite")));
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM sales")), qint64(0));
        // Sin limite (0) no hay tope
        CustomerService cs(Database::connection());
        Customer ana;
        QVERIFY(cs.customers().get(m_ana, &ana));
        ana.creditLimitCents = 0;
        QString error;
        QVERIFY(cs.save(ana, &error));
        r = sell(9000, {pay(PaymentMethod::credit(), 108000)}, m_ana);
        QVERIFY2(r.ok, qPrintable(r.error));
    }

    void creditCannotProduceChange()
    {
        const CheckoutResult r = sell(1000, {pay(PaymentMethod::cash(), 10000), pay(PaymentMethod::credit(), 5000)}, m_ana);
        QVERIFY(!r.ok);   // 100 + 50 > 120
    }

    void paymentsGoToTheCash()
    {
        QVERIFY(sell(5000, {pay(PaymentMethod::credit(), 60000)}, m_ana).ok);
        CustomerService cs(Database::connection());
        QString error;
        QVERIFY(!cs.registerPayment(m_ana, 70000, PaymentMethod::cash(), QString(), m_user, &error));   // mas que la deuda
        QVERIFY(error.contains(QStringLiteral("deuda")));
        QVERIFY2(cs.registerPayment(m_ana, 25000, PaymentMethod::cash(), QStringLiteral("abono"), m_user, &error), qPrintable(error));
        QVERIFY(cs.registerPayment(m_ana, 10000, PaymentMethod::transfer(), QString(), m_user, &error));
        QCOMPARE(cs.customers().balance(m_ana), qint64(25000));
        const QVector<CustomerEntry> entries = cs.customers().entries(m_ana);
        QCOMPARE(entries.size(), 3);
        QCOMPARE(entries.first().balanceAfter, qint64(25000));     // mas reciente primero
        QCOMPARE(entries.last().kind, CustomerEntryKind::charge());

        CashService cash(Database::connection());
        CashSession s;
        QVERIFY(cash.current(&s));
        const CashSummary sum = cash.summary(s);
        QCOMPARE(sum.creditPaymentsCash, qint64(25000));
        QCOMPARE(sum.creditPaymentsTransfer, qint64(10000));
        QCOMPARE(sum.expected.first().expectedCents, qint64(50000 + 25000));

        // No se desactiva un cliente con deuda
        QVERIFY(!cs.setActive(m_ana, false, &error));
        int withDebt = 0;
        QCOMPARE(cs.customers().receivables(&withDebt), qint64(25000));
        QCOMPARE(withDebt, 1);

        ReportService rep(Database::connection());
        const ReportSummary rs = rep.summary(QDate::currentDate(), QDate::currentDate());
        QCOMPARE(rs.credit, qint64(60000));
        QCOMPARE(rs.creditPayments, qint64(35000));
        QCOMPARE(rs.cash, qint64(0));      // la venta fue toda a credito
    }

    void paymentNeedsOpenCash()
    {
        QVERIFY(sell(1000, {pay(PaymentMethod::credit(), 12000)}, m_ana).ok);
        CashService cash(Database::connection());
        QString error;
        QVERIFY(cash.close({{QStringLiteral("CUP"), 50000}}, m_user, nullptr, &error));
        CustomerService cs(Database::connection());
        QVERIFY(!cs.registerPayment(m_ana, 12000, PaymentMethod::cash(), QString(), m_user, &error));
        QVERIFY(error.contains(QStringLiteral("caja")));
    }

    void refundToAccountLowersDebt()
    {
        const CheckoutResult sale = sell(5000, {pay(PaymentMethod::credit(), 60000)}, m_ana);
        QVERIFY(sale.ok);
        RefundService svc(Database::connection());
        RefundRequest req;
        req.saleId = sale.saleId;
        req.qtyByItem = {{int(scalar(QStringLiteral("SELECT id FROM sale_items"))), 2000}};
        req.method = PaymentMethod::credit();
        req.reason = QStringLiteral("Devolvió dos");
        req.userId = m_user;
        req.authorizedBy = m_user;
        const RefundResult r = svc.refund(req);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.total, qint64(24000));
        CustomerService cs(Database::connection());
        QCOMPARE(cs.customers().balance(m_ana), qint64(36000));

        CashService cash(Database::connection());
        CashSession s;
        QVERIFY(cash.current(&s));
        const CashSummary sum = cash.summary(s);
        QCOMPARE(sum.expected.first().expectedCents, qint64(50000));   // no sale dinero de la gaveta
        QCOMPARE(sum.creditSales, qint64(36000));
        QCOMPARE(sum.salesTotal, qint64(36000));

        SaleRepository repo(Database::connection());
        SaleDetail d;
        QVERIFY(repo.load(sale.saleId, &d));
        QCOMPARE(d.refunds.first().method, PaymentMethod::credit());

        // Una venta sin cliente no admite devolucion a cuenta
        const CheckoutResult cashSale = sell(1000, {pay(PaymentMethod::cash(), 12000)}, 0);
        QVERIFY(cashSale.ok);
        RefundRequest bad = req;
        bad.saleId = cashSale.saleId;
        bad.qtyByItem.clear();
        bad.all = true;
        QVERIFY(!svc.refund(bad).ok);
    }
};

QTEST_GUILESS_MAIN(TstCredit)
#include "tst_credit.moc"
