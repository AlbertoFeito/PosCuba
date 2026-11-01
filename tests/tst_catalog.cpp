#include <QtTest>

#include <QSqlQuery>
#include <QTemporaryDir>

#include "core/CsvReader.h"
#include "core/services/InventoryService.h"
#include "core/services/PricingService.h"
#include "core/services/ProductImportService.h"
#include "core/services/ProductService.h"
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
QString text(const QString &sql)
{
    QSqlQuery q(Database::connection());
    if (q.exec(sql) && q.next())
        return q.value(0).toString();
    return QStringLiteral("<none>");
}
} // namespace

// Cambio de precios en lote, lectura de CSV e importacion/exportacion de productos.
class TstCatalog : public QObject
{
    Q_OBJECT

    int m_user = 0;
    int m_bebidas = 0;

    Product make(const QString &code, const QString &name, qint64 cost, qint64 price, int cat = 0)
    {
        ProductService ps(Database::connection());
        Product p;
        p.code = code;
        p.name = name;
        p.costCents = cost;
        p.priceCents = price;
        p.categoryId = cat;
        QString error;
        if (!ps.save(p, &error))
            qWarning() << error;
        return p;
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
        m_bebidas = ps.ensureCategory(QStringLiteral("Bebidas"), &error);
        make(QStringLiteral("1"), QStringLiteral("Cola"), 8000, 12000, m_bebidas);
        make(QStringLiteral("2"), QStringLiteral("Malta"), 9000, 14000, m_bebidas);
        make(QStringLiteral("3"), QStringLiteral("Arroz"), 0, 25000);
    }

    void cleanupTestCase() { Database::close(); }

    // ---- Precios ----

    void parsePercent()
    {
        int bp = 0;
        QVERIFY(PricingService::parsePercent(QStringLiteral("10"), &bp));
        QCOMPARE(bp, 1000);
        QVERIFY(PricingService::parsePercent(QStringLiteral("-5,5 %"), &bp));
        QCOMPARE(bp, -550);
        QVERIFY(PricingService::parsePercent(QStringLiteral("+12.25"), &bp));
        QCOMPARE(bp, 1225);
        QVERIFY(!PricingService::parsePercent(QStringLiteral("-100"), &bp));
        QVERIFY(!PricingService::parsePercent(QStringLiteral("abc"), &bp));
        QVERIFY(!PricingService::parsePercent(QStringLiteral("1.234"), &bp));
        QVERIFY(!PricingService::parsePercent(QString(), &bp));
    }

    void roundingRules()
    {
        PriceChangeRequest r;
        r.basisPoints = 1000;   // +10 %
        QCOMPARE(PricingService::newPrice(12000, 0, r), qint64(13200));
        r.stepCents = 500;      // a 5
        QCOMPARE(PricingService::newPrice(12000, 0, r), qint64(13500));   // 132 -> 135 hacia arriba
        r.rounding = PriceChangeRequest::RoundNearest;
        QCOMPARE(PricingService::newPrice(12000, 0, r), qint64(13000));   // 132 -> 130
        r.stepCents = 1000;
        r.rounding = PriceChangeRequest::RoundUp;
        QCOMPARE(PricingService::newPrice(10000, 0, r), qint64(11000));   // 110 exacto no sube a 120
        r.basisPoints = -2500;  // -25 %
        r.stepCents = 0;
        QCOMPARE(PricingService::newPrice(14000, 0, r), qint64(10500));
        r.mode = PriceChangeRequest::MarginOnCost;
        r.basisPoints = 5000;   // costo + 50 %
        QCOMPARE(PricingService::newPrice(1, 8000, r), qint64(12000));
    }

    void previewAndApplyByCategory()
    {
        PricingService svc(Database::connection());
        PriceChangeRequest r;
        r.categoryId = m_bebidas;
        r.basisPoints = 1000;
        r.stepCents = 500;
        const PricePreview pv = svc.preview(r);
        QCOMPARE(pv.changes.size(), 2);
        QString error;
        QCOMPARE(svc.apply(r, &error), 2);
        QCOMPARE(scalar(QStringLiteral("SELECT price_cents FROM products WHERE code='1'")), qint64(13500));
        QCOMPARE(scalar(QStringLiteral("SELECT price_cents FROM products WHERE code='2'")), qint64(15500));
        QCOMPARE(scalar(QStringLiteral("SELECT price_cents FROM products WHERE code='3'")), qint64(25000));
    }

    void marginSkipsProductsWithoutCost()
    {
        PricingService svc(Database::connection());
        PriceChangeRequest r;
        r.mode = PriceChangeRequest::MarginOnCost;
        r.basisPoints = 5000;
        const PricePreview pv = svc.preview(r);
        QCOMPARE(pv.skippedNoCost, 1);          // Arroz sin costo
        QCOMPARE(pv.unchanged, 1);              // Cola: 80 * 1.5 = 120, ya cuesta 120
        QCOMPARE(pv.changes.size(), 1);         // Malta: 90 * 1.5 = 135
        QCOMPARE(pv.changes.first().newPrice, qint64(13500));
    }

    // ---- CSV ----

    void csvReaderHandlesQuotesAndSeparators()
    {
        const QString t = QStringLiteral("a;\"b;c\";\"d \"\"e\"\"\"\r\n1;2,5;\"x\ny\"\n\n");
        QCOMPARE(CsvReader::detectSeparator(t), QChar(QLatin1Char(';')));
        const QVector<QStringList> rows = CsvReader::parse(t, QLatin1Char(';'));
        QCOMPARE(rows.size(), 2);
        QCOMPARE(rows.at(0), (QStringList{QStringLiteral("a"), QStringLiteral("b;c"), QStringLiteral("d \"e\"")}));
        QCOMPARE(rows.at(1).at(2), QStringLiteral("x\ny"));
        QCOMPARE(CsvReader::detectSeparator(QStringLiteral("a,b,c\n1;2")), QChar(QLatin1Char(',')));
    }

    void csvDecodesWindows1252()
    {
        const QByteArray latin("Caf\xE9;Precio \x80\n", 14);
        QCOMPARE(CsvReader::decode(latin), QStringLiteral("Café;Precio €\n"));
        const QByteArray utf8 = QByteArray("\xEF\xBB\xBF") + QStringLiteral("Café").toUtf8();
        QCOMPARE(CsvReader::decode(utf8), QStringLiteral("Café"));
    }

    void normalizesNumbers()
    {
        QCOMPARE(ProductImportService::normalizeNumber(QStringLiteral("1.234,50")), QStringLiteral("1234.50"));
        QCOMPARE(ProductImportService::normalizeNumber(QStringLiteral("1,234.50")), QStringLiteral("1234.50"));
        QCOMPARE(ProductImportService::normalizeNumber(QStringLiteral("$ 120,5")), QStringLiteral("120.5"));
        QCOMPARE(ProductImportService::normalizeNumber(QStringLiteral("80 CUP")), QStringLiteral("80"));
    }

    // ---- Importacion ----

    void importPlansNewUpdatesAndErrors()
    {
        const QString csv = QStringLiteral(
            "Código;Nombre;Categoría;Unidad;Costo;Precio de venta;Stock;Columna rara\n"
            "1;Cola;Bebidas;u;80,00;130,00;;x\n"           // actualiza precio
            "2;Malta;Bebidas;u;90;140;;\n"                  // sin cambios
            ";Frijoles negros;Víveres;libra;150;220;30;\n"  // nuevo, codigo automatico, categoria nueva
            "9;Azúcar;;lb;;;;\n"                            // nuevo sin precio -> error
            "10;Aceite;;galón;1;2;;\n"                      // unidad invalida -> error
            "11;Pan;;u;1;abc;;\n"                           // precio invalido -> error
            "1;Cola otra vez;;u;1;2;;\n");                  // codigo repetido -> error
        ProductImportService svc(Database::connection());
        const ImportPlan pl = svc.plan(csv, false);
        QVERIFY2(pl.usable(), qPrintable(pl.error));
        QCOMPARE(pl.ignored, QStringList{QStringLiteral("Columna rara")});
        QCOMPARE(pl.updateCount, 1);
        QCOMPARE(pl.unchangedCount, 1);
        QCOMPARE(pl.newCount, 1);
        QCOMPARE(pl.errorCount, 4);
        QVERIFY(pl.rows.at(0).changes.join(QLatin1Char('|')).contains(QStringLiteral("precio 120.00 → 130.00")));
        QVERIFY(pl.rows.at(2).autoCode);
        QCOMPARE(pl.rows.at(2).product.unit, QStringLiteral("lb"));
        QVERIFY(pl.rows.at(3).error.contains(QStringLiteral("precio")));
        QVERIFY(pl.rows.at(4).error.contains(QStringLiteral("Unidad")));
        QVERIFY(pl.rows.at(5).error.contains(QStringLiteral("Precio")));
        QVERIFY(pl.rows.at(6).error.contains(QStringLiteral("repetido")));
    }

    void importAppliesValidRowsInOneTransaction()
    {
        const QString csv = QStringLiteral(
            "codigo,nombre,categoria,unidad,costo,precio,stock\n"
            "1,Cola,Bebidas,u,80.00,130.00,99\n"
            ",Frijoles negros,Víveres,lb,150,220,30\n"
            "X1,Malo,,u,1,,\n");
        ProductImportService svc(Database::connection());
        const ImportResult r = svc.apply(csv, false, m_user);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.created, 1);
        QCOMPARE(r.updated, 1);
        QCOMPARE(r.categoriesCreated, 1);
        QCOMPARE(scalar(QStringLiteral("SELECT price_cents FROM products WHERE code='1'")), qint64(13000));
        // Stock de un existente no se toca sin updateStock
        QCOMPARE(scalar(QStringLiteral("SELECT stock_milli FROM products WHERE code='1'")), qint64(0));
        // Nuevo: codigo siguiente (4), categoria creada y entrada de stock registrada
        QCOMPARE(text(QStringLiteral("SELECT code FROM products WHERE name='Frijoles negros'")), QStringLiteral("4"));
        QCOMPARE(text(QStringLiteral("SELECT c.name FROM products p JOIN categories c ON c.id = p.category_id "
                                     "WHERE p.name='Frijoles negros'")), QStringLiteral("Víveres"));
        QCOMPARE(scalar(QStringLiteral("SELECT stock_milli FROM products WHERE code='4'")), qint64(30000));
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM stock_movements WHERE type='purchase'")), qint64(1));
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM products WHERE code='X1'")), qint64(0));
    }

    void emptyCellsKeepExistingValues()
    {
        QSqlQuery q(Database::connection());
        QVERIFY(q.exec(QStringLiteral("UPDATE products SET barcode = '750' WHERE code = '1'")));
        ProductImportService svc(Database::connection());
        const ImportResult r = svc.apply(QStringLiteral("Código;Código de barras;Categoría;Precio\n1;;;125\n"), false, m_user);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(text(QStringLiteral("SELECT barcode FROM products WHERE code='1'")), QStringLiteral("750"));
        QCOMPARE(scalar(QStringLiteral("SELECT category_id FROM products WHERE code='1'")), qint64(m_bebidas));
        QCOMPARE(scalar(QStringLiteral("SELECT price_cents FROM products WHERE code='1'")), qint64(12500));
    }

    void importCanAdjustExistingStock()
    {
        ProductImportService svc(Database::connection());
        const ImportResult r = svc.apply(QStringLiteral("Código;Stock\n1;12\n2;0\n"), true, m_user);
        QVERIFY2(r.ok, qPrintable(r.error));
        QCOMPARE(r.updated, 1);   // el 2 ya tenia 0
        QCOMPARE(scalar(QStringLiteral("SELECT stock_milli FROM products WHERE code='1'")), qint64(12000));
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM stock_movements WHERE type='adjust'")), qint64(1));
        // Columnas que no estan en el archivo no se tocan
        QCOMPARE(scalar(QStringLiteral("SELECT price_cents FROM products WHERE code='1'")), qint64(12000));
    }

    void importRejectsUnknownHeader()
    {
        ProductImportService svc(Database::connection());
        const ImportPlan pl = svc.plan(QStringLiteral("foo;bar\n1;2\n"), false);
        QVERIFY(!pl.usable());
        QVERIFY(!svc.apply(QStringLiteral("foo;bar\n1;2\n"), false, m_user).ok);
    }

    void exportRoundTrip()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("productos.csv"));
        ProductImportService svc(Database::connection());
        int count = 0;
        QString error;
        QVERIFY2(svc.exportTo(path, CsvWriter::Spanish, true, &count, &error), qPrintable(error));
        QCOMPARE(count, 3);
        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QString csv = CsvReader::decode(f.readAll());
        QVERIFY(csv.startsWith(QStringLiteral("Código;Código de barras;Nombre")));
        // Reimportar lo exportado no cambia nada
        const ImportPlan pl = svc.plan(csv, true);
        QVERIFY2(pl.usable(), qPrintable(pl.error));
        QCOMPARE(pl.unchangedCount, 3);
        QCOMPARE(pl.errorCount, 0);

        const QString tpl = dir.filePath(QStringLiteral("plantilla.csv"));
        QVERIFY(ProductImportService::writeTemplate(tpl, CsvWriter::Standard, &error));
        QFile t(tpl);
        QVERIFY(t.open(QIODevice::ReadOnly));
        const ImportPlan tp = svc.plan(CsvReader::decode(t.readAll()), false);
        QCOMPARE(tp.errorCount, 0);
        QCOMPARE(tp.newCount, 2);
    }
};

QTEST_GUILESS_MAIN(TstCatalog)
#include "tst_catalog.moc"
