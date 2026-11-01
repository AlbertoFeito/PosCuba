#include <QtTest>

#include "core/ProductValidator.h"
#include "core/services/ProductService.h"
#include "data/Database.h"
#include "data/Migrator.h"

namespace {
Product makeProduct(const QString &code, const QString &name, qint64 price = 1000)
{
    Product p;
    p.code = code;
    p.name = name;
    p.priceCents = price;
    p.costCents = price / 2;
    return p;
}
} // namespace

class TstProductService : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        Database::close();
        QString error;
        QVERIFY2(Database::open(QStringLiteral(":memory:"), &error), qPrintable(error));
        QVERIFY2(Migrator::run(Database::connection(), &error), qPrintable(error));
    }

    void cleanupTestCase() { Database::close(); }

    void migrationIsIdempotent()
    {
        QString error;
        QCOMPARE(Migrator::currentVersion(Database::connection()), Migrator::latestVersion());
        QVERIFY(Migrator::run(Database::connection(), &error));
        QCOMPARE(Migrator::currentVersion(Database::connection()), Migrator::latestVersion());
    }

    void validatorRules()
    {
        Product p = makeProduct(QStringLiteral("  A1 "), QStringLiteral("  Pan   de flauta "));
        QVERIFY(ProductValidator::validate(p).isEmpty());
        QCOMPARE(p.code, QStringLiteral("A1"));
        QCOMPARE(p.name, QStringLiteral("Pan de flauta"));

        Product noName = makeProduct(QStringLiteral("A2"), QStringLiteral("   "));
        QVERIFY(!ProductValidator::validate(noName).isEmpty());

        Product spaced = makeProduct(QStringLiteral("A 3"), QStringLiteral("X"));
        QVERIFY(!ProductValidator::validate(spaced).isEmpty());

        Product badUnit = makeProduct(QStringLiteral("A4"), QStringLiteral("X"));
        badUnit.unit = QStringLiteral("caja");
        QVERIFY(!ProductValidator::validate(badUnit).isEmpty());

        Product cheap = makeProduct(QStringLiteral("A5"), QStringLiteral("X"), 100);
        cheap.costCents = 200;
        QVERIFY(ProductValidator::validate(cheap).isEmpty());
        QVERIFY(ProductValidator::priceBelowCost(cheap));
    }

    void insertAndUpdate()
    {
        ProductService svc(Database::connection());
        QString error;
        Product p = makeProduct(QStringLiteral("100"), QStringLiteral("Refresco"));
        QVERIFY2(svc.save(p, &error), qPrintable(error));
        QVERIFY(p.id > 0);

        p.name = QStringLiteral("Refresco lata");
        p.priceCents = 1500;
        QVERIFY2(svc.save(p, &error), qPrintable(error));

        Product loaded;
        QVERIFY(svc.products().get(p.id, &loaded));
        QCOMPARE(loaded.name, QStringLiteral("Refresco lata"));
        QCOMPARE(loaded.priceCents, qint64(1500));
        QCOMPARE(loaded.stockMilli, qint64(0));
    }

    void rejectsDuplicates()
    {
        ProductService svc(Database::connection());
        QString error;
        Product a = makeProduct(QStringLiteral("200"), QStringLiteral("A"));
        a.barcode = QStringLiteral("7501234");
        QVERIFY(svc.save(a, &error));

        Product sameCode = makeProduct(QStringLiteral("200"), QStringLiteral("B"));
        QVERIFY(!svc.save(sameCode, &error));
        QVERIFY(error.contains(QStringLiteral("200")));

        Product sameBarcode = makeProduct(QStringLiteral("201"), QStringLiteral("C"));
        sameBarcode.barcode = QStringLiteral("7501234");
        QVERIFY(!svc.save(sameBarcode, &error));

        // Varios productos sin codigo de barras no chocan (se guarda NULL).
        Product n1 = makeProduct(QStringLiteral("300"), QStringLiteral("N1"));
        Product n2 = makeProduct(QStringLiteral("301"), QStringLiteral("N2"));
        QVERIFY2(svc.save(n1, &error), qPrintable(error));
        QVERIFY2(svc.save(n2, &error), qPrintable(error));

        // Actualizar un producto sin cambiar su codigo no cuenta como duplicado.
        QVERIFY2(svc.save(a, &error), qPrintable(error));
    }

    void searchAndDeactivate()
    {
        ProductService svc(Database::connection());
        QString error;
        Product cafe = makeProduct(QStringLiteral("10"), QStringLiteral("Café molido"));
        cafe.barcode = QStringLiteral("850001");
        Product pan = makeProduct(QStringLiteral("11"), QStringLiteral("Pan"));
        Product pct = makeProduct(QStringLiteral("12"), QStringLiteral("Descuento 10%"));
        QVERIFY(svc.save(cafe, &error));
        QVERIFY(svc.save(pan, &error));
        QVERIFY(svc.save(pct, &error));

        ProductFilter f;
        f.text = QStringLiteral("molido");
        QCOMPARE(svc.products().find(f).size(), 1);

        f.text = QStringLiteral("850001");          // codigo de barras exacto
        QCOMPARE(svc.products().find(f).size(), 1);

        f.text = QStringLiteral("1");               // prefijo de codigo
        QCOMPARE(svc.products().find(f).size(), 3);

        f.text = QStringLiteral("%");               // comodin escapado
        QCOMPARE(svc.products().find(f).size(), 1);

        QVERIFY(svc.setActive(pan.id, false, &error));
        f.text.clear();
        QCOMPARE(svc.products().find(f).size(), 2);
        f.includeInactive = true;
        QCOMPARE(svc.products().find(f).size(), 3);
    }

    void categories()
    {
        ProductService svc(Database::connection());
        QString error;
        const int bebidas = svc.ensureCategory(QStringLiteral(" Bebidas "), &error);
        QVERIFY(bebidas > 0);
        QCOMPARE(svc.ensureCategory(QStringLiteral("bebidas"), &error), bebidas);
        QVERIFY(svc.ensureCategory(QStringLiteral("  "), &error) < 0);

        Product p = makeProduct(QStringLiteral("20"), QStringLiteral("Malta"));
        p.categoryId = bebidas;
        QVERIFY(svc.save(p, &error));

        ProductFilter f;
        f.categoryId = bebidas;
        const QVector<Product> found = svc.products().find(f);
        QCOMPARE(found.size(), 1);
        QCOMPARE(found.first().categoryName, QStringLiteral("Bebidas"));
    }

    void nextCode()
    {
        ProductService svc(Database::connection());
        QString error;
        QCOMPARE(svc.products().nextNumericCode(), QStringLiteral("1"));
        Product a = makeProduct(QStringLiteral("41"), QStringLiteral("A"));
        Product b = makeProduct(QStringLiteral("ABC"), QStringLiteral("B"));
        QVERIFY(svc.save(a, &error));
        QVERIFY(svc.save(b, &error));
        QCOMPARE(svc.products().nextNumericCode(), QStringLiteral("42"));
    }
};

QTEST_GUILESS_MAIN(TstProductService)
#include "tst_productservice.moc"
