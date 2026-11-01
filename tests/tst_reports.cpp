#include <QtTest>

#include <QFile>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "core/CsvWriter.h"
#include "core/services/CashService.h"
#include "core/services/InventoryService.h"
#include "core/services/ProductService.h"
#include "core/services/ReportService.h"
#include "core/services/SaleService.h"
#include "data/Database.h"
#include "data/Migrator.h"

class TstReports : public QObject
{
    Q_OBJECT

    int m_user = 0;
    Product m_a; // 100.00, costo 60.00
    Product m_b; // 50.00/lb, costo 20.00

    CartLine line(const Product &p, qint64 qty)
    {
        CartLine l;
        l.productId = p.id;
        l.name = p.name;
        l.qtyMilli = qty;
        l.priceCents = p.priceCents;
        l.costCents = p.costCents;
        return l;
    }
    Payment pay(const QString &method, qint64 amount)
    {
        Payment p;
        p.method = method;
        p.currency = QStringLiteral("CUP");
        p.amountCents = amount;
        return p;
    }
    void sell(const QVector<CartLine> &lines, qint64 discount, const QVector<Payment> &pays, const QString &when)
    {
        const CheckoutResult r = SaleService(Database::connection()).checkout(lines, discount, pays, m_user, true);
        QVERIFY2(r.ok, qPrintable(r.error));
        QSqlQuery q(Database::connection());
        q.prepare(QStringLiteral("UPDATE sales SET created_at = :d WHERE id = :id"));
        q.bindValue(QStringLiteral(":d"), when);
        q.bindValue(QStringLiteral(":id"), r.saleId);
        QVERIFY(q.exec());
    }
    static QDate d(int y, int m, int day) { return QDate(y, m, day); }

private slots:
    void initTestCase()
    {
        QString error;
        QVERIFY(Database::open(QStringLiteral(":memory:"), &error));
        QVERIFY(Migrator::run(Database::connection(), &error));
        QSqlQuery q(Database::connection());
        QVERIFY(q.exec(QStringLiteral("SELECT id FROM users LIMIT 1")) && q.next());
        m_user = q.value(0).toInt();

        ProductService ps(Database::connection());
        m_a.code = QStringLiteral("A");
        m_a.name = QStringLiteral("Producto A");
        m_a.priceCents = 10000;
        m_a.costCents = 6000;
        QVERIFY(ps.save(m_a, &error));
        m_b.code = QStringLiteral("B");
        m_b.name = QStringLiteral("Producto B; especial");
        m_b.unit = QStringLiteral("lb");
        m_b.priceCents = 5000;
        m_b.costCents = 2000;
        QVERIFY(ps.save(m_b, &error));
        QVERIFY(CashService(Database::connection()).open(0, m_user, nullptr, &error));

        // Octubre 2026: el 5 y el 12 son lunes, el 11 es domingo.
        sell({line(m_a, 2000)}, 0, {pay(QStringLiteral("cash"), 20000)}, QStringLiteral("2026-10-05 10:00:00"));
        sell({line(m_b, 1500)}, 500, {pay(QStringLiteral("transfer"), 7000)}, QStringLiteral("2026-10-05 18:00:00"));
        sell({line(m_a, 1000)}, 0, {pay(QStringLiteral("cash"), 15000)}, QStringLiteral("2026-10-11 23:59:59"));
        sell({line(m_b, 4000)}, 0, {pay(QStringLiteral("cash"), 20000)}, QStringLiteral("2026-10-12 08:00:00"));
        sell({line(m_a, 1000)}, 0, {pay(QStringLiteral("cash"), 10000)}, QStringLiteral("2026-11-01 09:00:00"));
    }

    void cleanupTestCase() { Database::close(); }

    void summary()
    {
        const ReportSummary s = ReportService(Database::connection()).summary(d(2026, 10, 1), d(2026, 10, 31));
        QCOMPARE(s.salesCount, 4);
        QCOMPARE(s.total, qint64(57000));
        QCOMPARE(s.discount, qint64(500));
        QCOMPARE(s.cost, qint64(29000));     // A: 3 x 60 = 180; B: 5.5 x 20 = 110
        QCOMPARE(s.profit, qint64(28000));
        QCOMPARE(s.cash, qint64(50000));     // 200 + (150 - 50 vuelto) + 200
        QCOMPARE(s.transfer, qint64(7000));
        QCOMPARE(s.averageTicket, qint64(14250));
        QCOMPARE(s.marginPercent, 49);
    }

    void rangeIsInclusive()
    {
        ReportService r(Database::connection());
        QCOMPARE(r.summary(d(2026, 10, 11), d(2026, 10, 11)).salesCount, 1); // 23:59:59 incluido
        QCOMPARE(r.summary(d(2026, 10, 6), d(2026, 10, 10)).salesCount, 0);
        QCOMPARE(r.summary(d(2026, 10, 1), d(2026, 11, 30)).salesCount, 5);
    }

    void byDay()
    {
        const QVector<PeriodRow> rows =
            ReportService(Database::connection()).byPeriod(d(2026, 10, 5), d(2026, 10, 12), ReportGroup::Day);
        QCOMPARE(rows.size(), 8);                 // dias sin ventas incluidos
        QCOMPARE(rows.first().label, QStringLiteral("05/10"));
        QCOMPARE(rows.first().salesCount, 2);
        QCOMPARE(rows.first().total, qint64(27000));
        QCOMPARE(rows[1].total, qint64(0));
        QCOMPARE(rows.last().total, qint64(20000));
    }

    void byWeek()
    {
        const QVector<PeriodRow> rows =
            ReportService(Database::connection()).byPeriod(d(2026, 10, 1), d(2026, 10, 31), ReportGroup::Week);
        QCOMPARE(rows.size(), 5);                 // semanas del 28/09, 05/10, 12/10, 19/10, 26/10
        QCOMPARE(rows[0].start, d(2026, 9, 28));
        QCOMPARE(rows[0].total, qint64(0));
        QCOMPARE(rows[1].salesCount, 3);          // lunes 5 a domingo 11
        QCOMPARE(rows[1].total, qint64(37000));
        QCOMPARE(rows[1].profit, qint64(37000 - 18000 - 3000));
        QCOMPARE(rows[2].total, qint64(20000));
        QCOMPARE(rows[1].label, QStringLiteral("Sem. 05/10"));
    }

    void byMonth()
    {
        const QVector<PeriodRow> rows =
            ReportService(Database::connection()).byPeriod(d(2026, 10, 1), d(2026, 11, 30), ReportGroup::Month);
        QCOMPARE(rows.size(), 2);
        QCOMPARE(rows[0].label, QStringLiteral("oct 2026"));
        QCOMPARE(rows[0].total, qint64(57000));
        QCOMPARE(rows[1].total, qint64(10000));
    }

    void topProducts()
    {
        ReportService r(Database::connection());
        const QDate from = d(2026, 10, 1), to = d(2026, 10, 31);
        QVector<ProductRow> byQty = r.topProducts(from, to, TopOrder::Quantity, 10);
        QCOMPARE(byQty.size(), 2);
        QCOMPARE(byQty[0].productId, m_b.id);
        QCOMPARE(byQty[0].qtyMilli, qint64(5500));
        QCOMPARE(byQty[0].unit, QStringLiteral("lb"));

        QVector<ProductRow> byRevenue = r.topProducts(from, to, TopOrder::Revenue, 10);
        QCOMPARE(byRevenue[0].productId, m_a.id);
        QCOMPARE(byRevenue[0].revenue, qint64(30000));

        QVector<ProductRow> byProfit = r.topProducts(from, to, TopOrder::Profit, 10);
        QCOMPARE(byProfit[0].productId, m_b.id);
        QCOMPARE(byProfit[0].profit, qint64(27500 - 11000));

        QCOMPARE(r.topProducts(from, to, TopOrder::Quantity, 1).size(), 1);
    }

    void salesDetail()
    {
        const QVector<SaleReportRow> rows = ReportService(Database::connection()).sales(d(2026, 10, 1), d(2026, 10, 31));
        QCOMPARE(rows.size(), 4);
        QCOMPARE(rows[2].cash, qint64(10000));   // 150 - 50 de vuelto
        QCOMPARE(rows[1].transfer, qint64(7000));
        QCOMPARE(rows[1].discount, qint64(500));
        QCOMPARE(rows[0].cost, qint64(12000));
    }

    void csv()
    {
        CsvWriter es(CsvWriter::Spanish);
        QCOMPARE(es.number(QStringLiteral("1234.50")), QStringLiteral("1234,50"));
        QCOMPARE(es.line({QStringLiteral("a;b"), QStringLiteral("di \"hola\""), QStringLiteral("x")}),
                 QStringLiteral("\"a;b\";\"di \"\"hola\"\"\";x"));

        CsvWriter std(CsvWriter::Standard);
        QCOMPARE(std.number(QStringLiteral("1234.50")), QStringLiteral("1234.50"));
        QCOMPARE(std.line({QStringLiteral("a;b"), QStringLiteral("c,d")}), QStringLiteral("a;b,\"c,d\""));

        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("x/reporte.csv"));
        QString error;
        QVERIFY2(es.write(path, {QStringLiteral("Producto"), QStringLiteral("Importe")},
                          {{QStringLiteral("Café"), es.number(QStringLiteral("10.50"))}}, &error), qPrintable(error));
        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray data = f.readAll();
        QVERIFY(data.startsWith("\xEF\xBB\xBF"));
        QCOMPARE(QString::fromUtf8(data.mid(3)), QStringLiteral("Producto;Importe\r\nCafé;10,50\r\n"));
    }
};

QTEST_GUILESS_MAIN(TstReports)
#include "tst_reports.moc"
