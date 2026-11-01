#include <QtTest>

#include <QSqlQuery>

#include "core/services/CashService.h"
#include "core/services/CurrencyService.h"
#include "core/services/InventoryService.h"
#include "core/services/ProductService.h"
#include "core/services/SaleService.h"
#include "data/Database.h"
#include "data/Migrator.h"

namespace {
Currency make(const QString &code, qint64 rate, bool active = true)
{
    Currency c;
    c.code = code;
    c.name = QStringLiteral("Moneda ") + code;
    c.symbol = QStringLiteral("$");
    c.rateMilli = rate;
    c.active = active;
    return c;
}
} // namespace

class TstCurrency : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        Database::close();
        QString error;
        QVERIFY(Database::open(QStringLiteral(":memory:"), &error));
        QVERIFY(Migrator::run(Database::connection(), &error));
    }

    void cleanupTestCase() { Database::close(); }

    void validation()
    {
        Currency c = make(QStringLiteral(" usd "), 400000);
        QVERIFY(CurrencyService::validate(c).isEmpty());
        QCOMPARE(c.code, QStringLiteral("USD"));

        Currency bad = make(QStringLiteral("US"), 400000);
        QVERIFY(!CurrencyService::validate(bad).isEmpty());
        bad = make(QStringLiteral("U5D"), 400000);
        QVERIFY(!CurrencyService::validate(bad).isEmpty());
        bad = make(QStringLiteral("EUR"), 0);
        QVERIFY(!CurrencyService::validate(bad).isEmpty());
        bad = make(QStringLiteral("EUR"), 1000);
        bad.symbol.clear();
        QVERIFY(!CurrencyService::validate(bad).isEmpty());
    }

    void addUpdateAndBaseRules()
    {
        CurrencyService svc(Database::connection());
        QString error;
        QVERIFY2(svc.add(make(QStringLiteral("USD"), 400000), &error), qPrintable(error));
        QVERIFY(!svc.add(make(QStringLiteral("usd"), 1000), &error));      // duplicada
        QCOMPARE(svc.all().size(), 2);
        QCOMPARE(svc.all().first().code, QStringLiteral("CUP"));             // la base primero

        Currency usd = make(QStringLiteral("USD"), 450000);
        QVERIFY(svc.update(usd, &error));
        Currency cup = make(QStringLiteral("CUP"), 999999);
        cup.active = false;
        QVERIFY(!svc.update(cup, &error));                                   // base no se desactiva
        cup.active = true;
        QVERIFY(svc.update(cup, &error));                                    // la tasa de la base queda en 1
        QSqlQuery q(Database::connection());
        QVERIFY(q.exec(QStringLiteral("SELECT rate_milli FROM currencies WHERE code = 'CUP'")) && q.next());
        QCOMPARE(q.value(0).toLongLong(), qint64(1000));

        // Sin ventas se puede cambiar la base.
        QVERIFY2(svc.setBase(QStringLiteral("USD"), &error), qPrintable(error));
        QVERIFY(q.exec(QStringLiteral("SELECT code FROM currencies WHERE is_base = 1")) && q.next());
        QCOMPARE(q.value(0).toString(), QStringLiteral("USD"));
        QVERIFY(svc.setBase(QStringLiteral("CUP"), &error));
    }

    void rateChangeAffectsOnlyNewSales()
    {
        QString error;
        CurrencyService svc(Database::connection());
        QVERIFY(svc.add(make(QStringLiteral("USD"), 400000), &error));

        QSqlQuery q(Database::connection());
        QVERIFY(q.exec(QStringLiteral("SELECT id FROM users LIMIT 1")) && q.next());
        const int user = q.value(0).toInt();
        ProductService ps(Database::connection());
        Product p;
        p.code = QStringLiteral("1");
        p.name = QStringLiteral("Cerveza");
        p.priceCents = 40000;   // 400 CUP
        QVERIFY(ps.save(p, &error));
        QVERIFY(InventoryService(Database::connection()).registerPurchase(p.id, 10000, -1, false, QString(), user, &error));
        QVERIFY(CashService(Database::connection()).open(0, user, nullptr, &error));

        CartLine l;
        l.productId = p.id;
        l.name = p.name;
        l.qtyMilli = 1000;
        l.priceCents = p.priceCents;
        Payment usd;
        usd.method = PaymentMethod::cash();
        usd.currency = QStringLiteral("USD");
        usd.amountCents = 100;  // 1 USD
        usd.rateMilli = 1;      // el servicio ignora la tasa del llamador
        SaleService sales(Database::connection());
        CheckoutResult r = sales.checkout({l}, 0, {usd}, user, false);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.change, qint64(0));

        QVERIFY(svc.update(make(QStringLiteral("USD"), 500000), &error));  // sube la tasa
        r = sales.checkout({l}, 0, {usd}, user, false);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.change, qint64(10000));                                // 500 - 400

        QVERIFY(q.exec(QStringLiteral("SELECT rate_milli FROM payments ORDER BY id")));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toLongLong(), qint64(400000));                // la primera conserva su tasa
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toLongLong(), qint64(500000));

        // Con ventas ya no se puede cambiar la base; moneda inactiva no se acepta en pagos.
        QVERIFY(!svc.setBase(QStringLiteral("USD"), &error));
        QVERIFY(svc.update(make(QStringLiteral("USD"), 500000, false), &error));
        QVERIFY(!sales.checkout({l}, 0, {usd}, user, false).ok);
    }
};

QTEST_GUILESS_MAIN(TstCurrency)
#include "tst_currency.moc"
