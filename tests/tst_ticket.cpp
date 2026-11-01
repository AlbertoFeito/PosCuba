#include <QtTest>

#include <QSqlQuery>

#include "core/TicketFormatter.h"
#include "core/services/CashService.h"
#include "core/services/InventoryService.h"
#include "core/services/ProductService.h"
#include "core/services/SaleService.h"
#include "data/Database.h"
#include "data/Migrator.h"
#include "data/repositories/SaleRepository.h"

namespace {
SaleDetail sampleSale()
{
    SaleDetail s;
    s.number = 42;
    s.createdAt = QStringLiteral("2026-11-01 15:35:10");
    s.userName = QStringLiteral("Ana");
    s.status = QStringLiteral("completed");
    SaleItemDetail a;
    a.name = QStringLiteral("Refresco de cola 355 ml");
    a.unit = QStringLiteral("u");
    a.qtyMilli = 3000;
    a.priceCents = 12000;
    a.lineTotalCents = 36000;
    SaleItemDetail b;
    b.name = QStringLiteral("Aceite vegetal refinado de girasol marca muy larga 1 litro");
    b.unit = QStringLiteral("kg");
    b.qtyMilli = 2500;
    b.priceCents = 25000;
    b.lineTotalCents = 62500;
    s.items = {a, b};
    s.subtotal = 98500;
    s.discount = 5000;
    s.total = 93500;
    PaymentDetail cash;
    cash.method = QStringLiteral("cash");
    cash.currency = QStringLiteral("CUP");
    cash.amountCents = 50000;
    cash.baseCents = 50000;
    PaymentDetail usd;
    usd.method = QStringLiteral("cash");
    usd.currency = QStringLiteral("USD");
    usd.amountCents = 100;
    usd.baseCents = 40000;
    PaymentDetail tr;
    tr.method = QStringLiteral("transfer");
    tr.currency = QStringLiteral("CUP");
    tr.amountCents = 5000;
    tr.baseCents = 5000;
    tr.reference = QStringLiteral("TRX-1");
    s.payments = {cash, usd, tr};
    s.change = 1500;
    return s;
}
} // namespace

class TstTicket : public QObject
{
    Q_OBJECT

private slots:
    void helpers()
    {
        QCOMPARE(TicketFormatter::columnsForPaper(58), 32);
        QCOMPARE(TicketFormatter::columnsForPaper(80), 48);
        QCOMPARE(TicketFormatter::justify(QStringLiteral("Total:"), QStringLiteral("10.00"), 20),
                 QStringLiteral("Total:         10.00"));
        QCOMPARE(TicketFormatter::center(QStringLiteral("ABC"), 9), QStringLiteral("   ABC"));
        // Texto izquierdo demasiado largo: se recorta sin pisar el importe.
        const QString j = TicketFormatter::justify(QString(40, QLatin1Char('x')), QStringLiteral("99.00"), 20);
        QCOMPARE(j.size(), 20);
        QVERIFY(j.endsWith(QStringLiteral(" 99.00")));

        const QStringList w = TicketFormatter::wrap(QStringLiteral("uno dos tres cuatro cinco"), 10);
        QCOMPARE(w, QStringList({QStringLiteral("uno dos"), QStringLiteral("tres"), QStringLiteral("cuatro"),
                                 QStringLiteral("cinco")}));
        const QStringList longWord = TicketFormatter::wrap(QStringLiteral("abcdefghijklmn"), 5);
        QCOMPARE(longWord.size(), 3);
    }

    void format_data()
    {
        QTest::addColumn<int>("columns");
        QTest::newRow("58mm") << 32;
        QTest::newRow("80mm") << 48;
    }

    void format()
    {
        QFETCH(int, columns);
        TicketInfo info;
        info.businessName = QStringLiteral("Bodega La Esquina");
        info.address = QStringLiteral("Calle 23 #456 entre L y M, Vedado, La Habana");
        info.phone = QStringLiteral("5555-5555");
        const QStringList lines = TicketFormatter::format(sampleSale(), info, columns);

        for (const QString &l : lines)
            QVERIFY2(l.size() <= columns, qPrintable(QStringLiteral("[%1] %2").arg(l.size()).arg(l)));

        const QString all = lines.join(QLatin1Char('\n'));
        QVERIFY(all.contains(QStringLiteral("BODEGA LA ESQUINA")));
        QVERIFY(all.contains(QStringLiteral("000042")));
        QVERIFY(all.contains(QStringLiteral("01/11/2026 15:35")));
        QVERIFY(all.contains(QStringLiteral("3 x 120.00")));
        QVERIFY(all.contains(QStringLiteral("2.5 kg x 250.00")));
        QVERIFY(all.contains(QStringLiteral("-50.00")));
        QVERIFY(all.contains(QStringLiteral("935.00")));
        QVERIFY(all.contains(QStringLiteral("Efectivo USD:")));
        QVERIFY(all.contains(QStringLiteral("Ref: TRX-1")));
        QVERIFY(all.contains(QStringLiteral("Vuelto:")));
        QVERIFY(all.contains(QStringLiteral("Gracias")));
        // Las lineas con importe terminan alineadas a la derecha.
        for (const QString &l : lines) {
            if (l.startsWith(QStringLiteral("TOTAL")))
                QVERIFY(l.endsWith(QStringLiteral("935.00")) && l.size() == columns);
        }
    }

    void noDiscountHidesSubtotal()
    {
        SaleDetail s = sampleSale();
        s.discount = 0;
        s.total = s.subtotal;
        const QString all = TicketFormatter::format(s, TicketInfo(), 32).join(QLatin1Char('\n'));
        QVERIFY(!all.contains(QStringLiteral("Descuento")));
        QVERIFY(!all.contains(QStringLiteral("Subtotal")));
    }

    void loadsRecordedSale()
    {
        Database::close();
        QString error;
        QVERIFY(Database::open(QStringLiteral(":memory:"), &error));
        QVERIFY(Migrator::run(Database::connection(), &error));
        QSqlQuery q(Database::connection());
        QVERIFY(q.exec(QStringLiteral("SELECT id FROM users LIMIT 1")) && q.next());
        const int user = q.value(0).toInt();

        ProductService ps(Database::connection());
        Product p;
        p.code = QStringLiteral("1");
        p.name = QStringLiteral("Arroz");
        p.unit = QStringLiteral("lb");
        p.priceCents = 25000;
        QVERIFY(ps.save(p, &error));
        QVERIFY(InventoryService(Database::connection()).registerPurchase(p.id, 10000, -1, false, QString(), user, &error));
        QVERIFY(CashService(Database::connection()).open(0, user, nullptr, &error));

        CartLine l;
        l.productId = p.id;
        l.name = p.name;
        l.qtyMilli = 2000;
        l.priceCents = p.priceCents;
        Payment pay;
        pay.method = PaymentMethod::cash();
        pay.currency = QStringLiteral("CUP");
        pay.amountCents = 60000;
        const CheckoutResult r = SaleService(Database::connection()).checkout({l}, 0, {pay}, user, false);
        QVERIFY2(r.ok, qPrintable(r.error));

        SaleRepository repo(Database::connection());
        SaleDetail d;
        QVERIFY(repo.load(r.saleId, &d));
        QCOMPARE(d.number, 1);
        QCOMPARE(d.items.size(), 1);
        QCOMPARE(d.items.first().unit, QStringLiteral("lb"));
        QCOMPARE(d.total, qint64(50000));
        QCOMPARE(d.change, qint64(10000));
        QCOMPARE(d.payments.size(), 1);
        QCOMPARE(d.userName, QStringLiteral("Administrador"));
        QCOMPARE(repo.recent(10).size(), 1);
        QCOMPARE(repo.lastSaleId(), r.saleId);
        QVERIFY(!repo.load(9999, &d));
        Database::close();
    }
};

QTEST_GUILESS_MAIN(TstTicket)
#include "tst_ticket.moc"
