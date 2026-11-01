#include <QtTest>

#include <QSqlQuery>

#include "core/services/CashService.h"
#include "core/services/InventoryService.h"
#include "core/services/ProductService.h"
#include "core/services/SaleService.h"
#include "data/Database.h"
#include "data/Migrator.h"

namespace {
qint64 scalar(const QString &sql)
{
    QSqlQuery q(Database::connection());
    if (q.exec(sql) && q.next())
        return q.value(0).toLongLong();
    return -999;
}
Payment cash(qint64 amount, const QString &cur = QStringLiteral("CUP"))
{
    Payment p;
    p.method = PaymentMethod::cash();
    p.currency = cur;
    p.amountCents = amount;
    return p;
}
Payment transfer(qint64 amount)
{
    Payment p;
    p.method = PaymentMethod::transfer();
    p.currency = QStringLiteral("CUP");
    p.amountCents = amount;
    p.reference = QStringLiteral("TRX-1");
    return p;
}
} // namespace

class TstSale : public QObject
{
    Q_OBJECT

    int m_user = 0;
    Product m_cola;   // precio 120.00, costo 80.00, stock 10
    Product m_arroz;  // precio 250.00/lb, costo 180.00, stock 5 lb

    CartLine lineOf(const Product &p, qint64 qtyMilli)
    {
        CartLine l;
        l.productId = p.id;
        l.name = p.name;
        l.unit = p.unit;
        l.qtyMilli = qtyMilli;
        l.priceCents = p.priceCents;
        l.costCents = p.costCents;
        return l;
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
        InventoryService inv(Database::connection());
        m_cola = Product();
        m_cola.code = QStringLiteral("1");
        m_cola.name = QStringLiteral("Cola");
        m_cola.priceCents = 12000;
        m_cola.costCents = 8000;
        QVERIFY(ps.save(m_cola, &error));
        QVERIFY(inv.registerPurchase(m_cola.id, 10000, -1, false, QString(), m_user, &error));

        m_arroz = Product();
        m_arroz.code = QStringLiteral("2");
        m_arroz.name = QStringLiteral("Arroz");
        m_arroz.unit = QStringLiteral("lb");
        m_arroz.priceCents = 25000;
        m_arroz.costCents = 18000;
        QVERIFY(ps.save(m_arroz, &error));
        QVERIFY(inv.registerPurchase(m_arroz.id, 5000, -1, false, QString(), m_user, &error));
    }

    void cleanupTestCase() { Database::close(); }

    void requiresOpenSession()
    {
        SaleService sales(Database::connection());
        const CheckoutResult r = sales.checkout({lineOf(m_cola, 1000)}, 0, {cash(12000)}, m_user, false);
        QVERIFY(!r.ok);
        QVERIFY(r.error.contains(QStringLiteral("caja")));
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM sales")), qint64(0));
    }

    void checkoutDeductsStockAndRecordsEverything()
    {
        CashService cashSvc(Database::connection());
        QString error;
        QVERIFY(cashSvc.open(50000, m_user, nullptr, &error));

        SaleService sales(Database::connection());
        // 3 colas (360.00) + 1.5 lb arroz (375.00) = 735.00, descuento 35.00 = 700.00; paga 1000 -> vuelto 300
        const CheckoutResult r = sales.checkout({lineOf(m_cola, 3000), lineOf(m_arroz, 1500)}, 3500,
                                                {cash(100000)}, m_user, false);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.number, 1);
        QCOMPARE(r.change, qint64(30000));

        QCOMPARE(scalar(QStringLiteral("SELECT total_cents FROM sales")), qint64(70000));
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM sale_items")), qint64(2));
        QCOMPARE(scalar(QStringLiteral("SELECT stock_milli FROM products WHERE code='1'")), qint64(7000));
        QCOMPARE(scalar(QStringLiteral("SELECT stock_milli FROM products WHERE code='2'")), qint64(3500));
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM stock_movements WHERE type='sale' AND sale_id=")
                        + QString::number(r.saleId)), qint64(2));

        // Segunda venta: numeracion correlativa.
        const CheckoutResult r2 = sales.checkout({lineOf(m_cola, 1000)}, 0, {cash(12000)}, m_user, false);
        QVERIFY2(r2.ok, qPrintable(r2.error));
        QCOMPARE(r2.number, 2);
    }

    void insufficientStockNeedsConfirmation()
    {
        CashService cashSvc(Database::connection());
        QString error;
        QVERIFY(cashSvc.open(0, m_user, nullptr, &error));
        SaleService sales(Database::connection());

        // Mismo producto en dos lineas: 6 + 6 = 12 > 10
        const QVector<CartLine> lines = {lineOf(m_cola, 6000), lineOf(m_cola, 6000)};
        CheckoutResult r = sales.checkout(lines, 0, {cash(144000)}, m_user, false);
        QVERIFY(!r.ok);
        QCOMPARE(r.stockIssues.size(), 1);
        QVERIFY(r.stockIssues.first().contains(QStringLiteral("Cola")));
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM sales")), qint64(0));
        QCOMPARE(scalar(QStringLiteral("SELECT stock_milli FROM products WHERE code='1'")), qint64(10000));

        r = sales.checkout(lines, 0, {cash(144000)}, m_user, true);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(scalar(QStringLiteral("SELECT stock_milli FROM products WHERE code='1'")), qint64(-2000));
    }

    void rejectsUnderpaymentWithoutSideEffects()
    {
        CashService cashSvc(Database::connection());
        QString error;
        QVERIFY(cashSvc.open(0, m_user, nullptr, &error));
        SaleService sales(Database::connection());
        const CheckoutResult r = sales.checkout({lineOf(m_cola, 2000)}, 0, {cash(10000)}, m_user, false);
        QVERIFY(!r.ok);
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM sales")), qint64(0));
        QCOMPARE(scalar(QStringLiteral("SELECT stock_milli FROM products WHERE code='1'")), qint64(10000));
    }

    void cashCloseWithMixedPaymentsAndForeignCurrency()
    {
        // USD a 400 CUP
        QSqlQuery q(Database::connection());
        QVERIFY(q.exec(QStringLiteral("INSERT INTO currencies (code, name, symbol, rate_milli) "
                                      "VALUES ('USD', 'Dolar', 'US$', 400000)")));

        CashService cashSvc(Database::connection());
        SaleService sales(Database::connection());
        QString error;
        QVERIFY(cashSvc.open(100000, m_user, nullptr, &error));          // fondo 1000.00

        // Venta 1: 2 colas = 240.00, paga 500.00 efectivo -> vuelto 260.00
        QVERIFY(sales.checkout({lineOf(m_cola, 2000)}, 0, {cash(50000)}, m_user, false).ok);
        // Venta 2: 4 lb arroz = 1000.00, transferencia 600 + 1 USD (400) = exacto
        QVERIFY(sales.checkout({lineOf(m_arroz, 4000)}, 0, {transfer(60000), cash(100, QStringLiteral("USD"))},
                               m_user, false).ok);
        // Venta 3: 1 cola = 120.00, paga 1 USD (400 CUP) -> vuelto 280 CUP
        QVERIFY(sales.checkout({lineOf(m_cola, 1000)}, 0, {cash(100, QStringLiteral("USD"))}, m_user, false).ok);

        QVERIFY(cashSvc.addMovement(-20000, QStringLiteral("Pago de hielo"), m_user, &error)); // retiro 200
        QVERIFY(!cashSvc.addMovement(-999999, QStringLiteral("Demasiado"), m_user, &error));

        CashSession s;
        QVERIFY(cashSvc.current(&s));
        const CashSummary sum = cashSvc.summary(s);
        QCOMPARE(sum.salesCount, 3);
        QCOMPARE(sum.salesTotal, qint64(136000));
        QCOMPARE(sum.transferSales, qint64(60000));
        // ganancia: 136000 - costo (3 colas x 80 + 4 lb x 180 = 240 + 720 = 960.00)
        QCOMPARE(sum.profit, qint64(136000 - 96000));
        QCOMPARE(sum.expected.size(), 2);
        // CUP: 1000 + 500 - 260 - 280 - 200 = 760.00
        QCOMPARE(sum.expected[0].currency, QStringLiteral("CUP"));
        QCOMPARE(sum.expected[0].expectedCents, qint64(76000));
        QCOMPARE(sum.expected[1].currency, QStringLiteral("USD"));
        QCOMPARE(sum.expected[1].expectedCents, qint64(200));

        // Falta conteo de USD
        QMap<QString, qint64> counted;
        counted.insert(QStringLiteral("CUP"), 75000);
        QVERIFY(!cashSvc.close(counted, m_user, nullptr, &error));

        // Cuentan 750.00 CUP (faltan 10) y 2 USD
        counted.insert(QStringLiteral("USD"), 200);
        CashSession closed;
        QVERIFY2(cashSvc.close(counted, m_user, &closed, &error), qPrintable(error));
        QCOMPARE(closed.status, QStringLiteral("closed"));
        QCOMPARE(closed.expectedCents, qint64(76000 + 80000));
        QCOMPARE(closed.countedCents, qint64(75000 + 80000));
        QCOMPARE(closed.differenceCents, qint64(-1000));
        QCOMPARE(cashSvc.counts(closed.id).size(), 2);

        // Caja cerrada: no se puede vender; se puede abrir otra.
        QVERIFY(!cashSvc.current(nullptr));
        QVERIFY(!sales.checkout({lineOf(m_cola, 1000)}, 0, {cash(12000)}, m_user, false).ok);
        QVERIFY(cashSvc.open(0, m_user, nullptr, &error));
        QVERIFY(!cashSvc.open(0, m_user, nullptr, &error)); // solo una abierta
    }
};

QTEST_GUILESS_MAIN(TstSale)
#include "tst_sale.moc"
