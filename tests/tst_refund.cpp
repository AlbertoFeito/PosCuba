#include <QtTest>

#include <QSqlQuery>

#include "core/services/CashService.h"
#include "core/services/InventoryService.h"
#include "core/services/ProductService.h"
#include "core/services/RefundService.h"
#include "core/services/ReportService.h"
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
Payment pay(const QString &method, qint64 amount)
{
    Payment p;
    p.method = method;
    p.currency = QStringLiteral("CUP");
    p.amountCents = amount;
    return p;
}
} // namespace

// Devoluciones parciales, anulaciones y su efecto en stock, caja y reportes.
class TstRefund : public QObject
{
    Q_OBJECT

    int m_user = 0;
    Product m_cola;   // 120.00, costo 80.00, stock 10
    Product m_arroz;  // 250.00/lb, costo 180.00, stock 5 lb
    int m_saleId = 0; // 3 colas + 2 lb arroz = 860.00, descuento 86.00 -> 774.00, efectivo

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

    int itemId(const Product &p)
    {
        return int(scalar(QStringLiteral("SELECT id FROM sale_items WHERE sale_id = %1 AND product_id = %2")
                              .arg(m_saleId).arg(p.id)));
    }

    RefundRequest request(const QMap<int, qint64> &qty, bool all = false)
    {
        RefundRequest r;
        r.saleId = m_saleId;
        r.qtyByItem = qty;
        r.all = all;
        r.reason = QStringLiteral("Cliente devolvió");
        r.userId = m_user;
        r.authorizedBy = m_user;
        return r;
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

        CashService cash(Database::connection());
        QVERIFY(cash.open(100000, m_user, nullptr, &error));
        SaleService sales(Database::connection());
        const CheckoutResult r = sales.checkout({lineOf(m_cola, 3000), lineOf(m_arroz, 2000)}, 8600,
                                                {pay(PaymentMethod::cash(), 80000)}, m_user, false);
        QVERIFY2(r.ok, qPrintable(r.error));
        m_saleId = r.saleId;
    }

    void cleanupTestCase() { Database::close(); }

    void partialRefundIncludesDiscountShare()
    {
        RefundService svc(Database::connection());
        const RefundResult r = svc.refund(request({{itemId(m_cola), 1000}}));
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.number, 1);
        QCOMPARE(r.total, qint64(10800));   // 120.00 * 774 / 860
        QVERIFY(!r.voided);
        QCOMPARE(scalar(QStringLiteral("SELECT status = 'completed' FROM sales")), qint64(1));
        QCOMPARE(scalar(QStringLiteral("SELECT stock_milli FROM products WHERE code='1'")), qint64(8000));
        QCOMPARE(scalar(QStringLiteral("SELECT cost_cents FROM refunds")), qint64(8000));
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM stock_movements WHERE type='void'")), qint64(1));

        // Caja: el efectivo devuelto sale de la gaveta y de lo vendido.
        CashService cash(Database::connection());
        CashSession s;
        QVERIFY(cash.current(&s));
        const CashSummary sum = cash.summary(s);
        QCOMPARE(sum.salesCount, 1);
        QCOMPARE(sum.salesTotal, qint64(77400 - 10800));
        QCOMPARE(sum.refundsCount, 1);
        QCOMPARE(sum.refundsTotal, qint64(10800));
        QCOMPARE(sum.expected.first().expectedCents, qint64(100000 + 77400 - 10800));
        QCOMPARE(sum.profit, qint64((77400 - 10800) - (24000 + 36000 - 8000)));
    }

    void completingTheSaleVoidsItExactly()
    {
        RefundService svc(Database::connection());
        QVERIFY(svc.refund(request({{itemId(m_cola), 1000}})).ok);
        const RefundResult r = svc.refund(request({}, true));
        QVERIFY2(r.ok, qPrintable(r.error));
        QVERIFY(r.voided);
        QCOMPARE(r.total, qint64(77400 - 10800));   // lo que faltaba, sin perder centavos
        QCOMPARE(scalar(QStringLiteral("SELECT SUM(total_cents) FROM refunds")), qint64(77400));
        QCOMPARE(scalar(QStringLiteral("SELECT status = 'voided' FROM sales")), qint64(1));
        QCOMPARE(scalar(QStringLiteral("SELECT stock_milli FROM products WHERE code='1'")), qint64(10000));
        QCOMPARE(scalar(QStringLiteral("SELECT stock_milli FROM products WHERE code='2'")), qint64(5000));

        const RefundResult again = svc.refund(request({}, true));
        QVERIFY(!again.ok);
        QVERIFY(again.error.contains(QStringLiteral("anulada")));

        CashService cash(Database::connection());
        CashSession s;
        QVERIFY(cash.current(&s));
        const CashSummary sum = cash.summary(s);
        QCOMPARE(sum.salesCount, 0);
        QCOMPARE(sum.salesTotal, qint64(0));
        QCOMPARE(sum.profit, qint64(0));
        QCOMPARE(sum.expected.first().expectedCents, qint64(100000));

        ReportService rep(Database::connection());
        const QDate today = QDate::currentDate();
        const ReportSummary rs = rep.summary(today, today);
        QCOMPARE(rs.salesCount, 0);
        QCOMPARE(rs.total, qint64(0));
        QCOMPARE(rs.cash, qint64(0));
        QCOMPARE(rs.refunds, qint64(77400));
        QVERIFY(rep.topProducts(today, today, TopOrder::Quantity, 10).isEmpty());
        const QVector<SaleReportRow> rows = rep.sales(today, today);
        QCOMPARE(rows.size(), 1);
        QVERIFY(rows.first().voided);
        QCOMPARE(rows.first().refunded, qint64(77400));
    }

    void reportsSubtractPartialRefunds()
    {
        RefundService svc(Database::connection());
        QVERIFY(svc.refund(request({{itemId(m_arroz), 500}})).ok);   // media libra
        ReportService rep(Database::connection());
        const QDate today = QDate::currentDate();
        const ReportSummary rs = rep.summary(today, today);
        QCOMPARE(rs.salesCount, 1);
        const qint64 refunded = scalar(QStringLiteral("SELECT total_cents FROM refunds"));
        QCOMPARE(refunded, qint64(11250));   // 125.00 * 774 / 860
        QCOMPARE(rs.total, qint64(77400 - refunded));
        const QVector<ProductRow> top = rep.topProducts(today, today, TopOrder::Quantity, 10);
        QCOMPARE(top.size(), 2);
        for (const ProductRow &p : top) {
            if (p.productId == m_arroz.id) {
                QCOMPARE(p.qtyMilli, qint64(1500));
                QCOMPARE(p.revenue, qint64(50000 - 12500));
            }
        }
        const QVector<PeriodRow> periods = rep.byPeriod(today, today, ReportGroup::Day);
        QCOMPARE(periods.size(), 1);
        QCOMPARE(periods.first().total, qint64(77400 - refunded));
        QCOMPARE(periods.first().refunds, refunded);
    }

    void validatesQuantitiesAndReason()
    {
        RefundService svc(Database::connection());
        RefundResult r = svc.refund(request({{itemId(m_cola), 4000}}));
        QVERIFY(!r.ok);
        QVERIFY(r.error.contains(QStringLiteral("Cola")));
        r = svc.refund(request({}));
        QVERIFY(!r.ok);
        RefundRequest noReason = request({{itemId(m_cola), 1000}});
        noReason.reason = QStringLiteral("  ");
        QVERIFY(!svc.refund(noReason).ok);
        RefundRequest noAdmin = request({{itemId(m_cola), 1000}});
        noAdmin.authorizedBy = 0;
        QVERIFY(!svc.refund(noAdmin).ok);
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM refunds")), qint64(0));
    }

    void withoutRestockStockStays()
    {
        RefundService svc(Database::connection());
        RefundRequest req = request({{itemId(m_cola), 2000}});
        req.restock = false;
        QVERIFY(svc.refund(req).ok);
        QCOMPARE(scalar(QStringLiteral("SELECT stock_milli FROM products WHERE code='1'")), qint64(7000));
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM stock_movements WHERE type='void'")), qint64(0));
    }

    void cashMustBeAvailableTransferNeedsNot()
    {
        // Retiro de casi todo el efectivo: quedan 50.00 en la gaveta.
        CashService cash(Database::connection());
        QString error;
        QVERIFY2(cash.addMovement(-(100000 + 77400 - 5000), QStringLiteral("Depósito"), m_user, &error),
                 qPrintable(error));
        RefundService svc(Database::connection());
        RefundResult r = svc.refund(request({{itemId(m_cola), 1000}}));
        QVERIFY(!r.ok);
        QVERIFY(r.error.contains(QStringLiteral("efectivo")));
        RefundRequest byTransfer = request({{itemId(m_cola), 1000}});
        byTransfer.method = PaymentMethod::transfer();
        r = svc.refund(byTransfer);
        QVERIFY2(r.ok, qPrintable(r.error));
        CashSession s;
        QVERIFY(cash.current(&s));
        QCOMPARE(cash.summary(s).expected.first().expectedCents, qint64(5000));
    }

    void needsOpenCash()
    {
        CashService cash(Database::connection());
        CashSession s;
        QVERIFY(cash.current(&s));
        QMap<QString, qint64> counted;
        counted.insert(QStringLiteral("CUP"), 177400);
        QString error;
        QVERIFY2(cash.close(counted, m_user, nullptr, &error), qPrintable(error));
        RefundService svc(Database::connection());
        const RefundResult r = svc.refund(request({{itemId(m_cola), 1000}}));
        QVERIFY(!r.ok);
        QVERIFY(r.error.contains(QStringLiteral("caja")));
    }

    void roundingNeverLosesCents()
    {
        // 3 unidades de 100.00 con descuento de 0.01 -> 299.99; se devuelven una a una.
        SaleRefundState st;
        st.saleId = 1;
        st.number = 9;
        st.status = QStringLiteral("completed");
        st.subtotal = 30000;
        st.total = 29999;
        RefundableItem it;
        it.saleItemId = 7;
        it.name = QStringLiteral("X");
        it.soldMilli = 3000;
        it.lineTotalCents = 30000;
        st.items.append(it);
        qint64 sum = 0;
        for (int i = 0; i < 3; ++i) {
            RefundCalc c;
            QString error;
            QVERIFY2(RefundService::calculate(st, {{7, 1000}}, false, &c, &error), qPrintable(error));
            QCOMPARE(c.completesSale, i == 2);
            sum += c.total;
            st.refunded += c.total;
            st.items[0].returnedMilli += 1000;
        }
        QCOMPARE(sum, qint64(29999));
    }
};

QTEST_GUILESS_MAIN(TstRefund)
#include "tst_refund.moc"
