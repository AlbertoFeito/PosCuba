#include <QtTest>

#include <QSqlQuery>

#include "core/services/InventoryService.h"
#include "core/services/ProductService.h"
#include "data/Database.h"
#include "data/Migrator.h"
#include "data/Transaction.h"

class TstInventory : public QObject
{
    Q_OBJECT

    int m_userId = 0;
    int m_productId = 0;

    qint64 stock(int id)
    {
        qint64 s = -999;
        ProductRepository(Database::connection()).stockOf(id, &s);
        return s;
    }

    int movementCount()
    {
        QSqlQuery q(Database::connection());
        q.exec(QStringLiteral("SELECT COUNT(*) FROM stock_movements"));
        q.next();
        return q.value(0).toInt();
    }

private slots:
    void init()
    {
        Database::close();
        QString error;
        QVERIFY2(Database::open(QStringLiteral(":memory:"), &error), qPrintable(error));
        QVERIFY2(Migrator::run(Database::connection(), &error), qPrintable(error));

        QSqlQuery q(Database::connection());
        QVERIFY(q.exec(QStringLiteral("SELECT id FROM users WHERE role = 'admin'")) && q.next());
        m_userId = q.value(0).toInt();

        ProductService ps(Database::connection());
        Product p;
        p.code = QStringLiteral("1");
        p.name = QStringLiteral("Arroz");
        p.unit = QStringLiteral("lb");
        p.costCents = 15000;
        p.priceCents = 25000;
        QVERIFY2(ps.save(p, &error), qPrintable(error));
        m_productId = p.id;
    }

    void cleanupTestCase() { Database::close(); }

    void defaultAdminExists() { QVERIFY(m_userId > 0); }

    void purchaseIncreasesStock()
    {
        InventoryService inv(Database::connection());
        QString error;
        QVERIFY2(inv.registerPurchase(m_productId, 10500, 18000, true, QStringLiteral("Proveedor X"),
                                      m_userId, &error), qPrintable(error));
        QCOMPARE(stock(m_productId), qint64(10500));

        Product p;
        QVERIFY(inv.products().get(m_productId, &p));
        QCOMPARE(p.costCents, qint64(18000));

        const QVector<StockMovement> h = inv.history(m_productId);
        QCOMPARE(h.size(), 1);
        QCOMPARE(h.first().type, MovementType::purchase());
        QCOMPARE(h.first().qtyMilli, qint64(10500));
        QCOMPARE(h.first().stockAfterMilli, qint64(10500));
        QCOMPARE(h.first().costCents, qint64(18000));
        QCOMPARE(h.first().userName, QStringLiteral("Administrador"));
        QCOMPARE(h.first().note, QStringLiteral("Proveedor X"));
    }

    void purchaseWithoutUpdatingCost()
    {
        InventoryService inv(Database::connection());
        QString error;
        QVERIFY(inv.registerPurchase(m_productId, 1000, 99900, false, QString(), m_userId, &error));
        Product p;
        QVERIFY(inv.products().get(m_productId, &p));
        QCOMPARE(p.costCents, qint64(15000));
    }

    void purchaseRejectsInvalid()
    {
        InventoryService inv(Database::connection());
        QString error;
        QVERIFY(!inv.registerPurchase(m_productId, 0, -1, false, QString(), m_userId, &error));
        QVERIFY(!inv.registerPurchase(9999, 1000, -1, false, QString(), m_userId, &error));
        QCOMPARE(movementCount(), 0);
    }

    void adjustToCounted()
    {
        InventoryService inv(Database::connection());
        QString error;
        QVERIFY(inv.registerPurchase(m_productId, 20000, -1, false, QString(), m_userId, &error));

        QVERIFY2(inv.adjustToCounted(m_productId, 17250, QStringLiteral("Merma"), QStringLiteral("humedad"),
                                     m_userId, &error), qPrintable(error));
        QCOMPARE(stock(m_productId), qint64(17250));
        const StockMovement last = inv.history(m_productId).first();
        QCOMPARE(last.qtyMilli, qint64(-2750));
        QCOMPARE(last.stockAfterMilli, qint64(17250));
        QCOMPARE(last.note, QStringLiteral("Merma: humedad"));

        // Igual al actual, negativo o sin motivo: rechazado y sin cambios.
        QVERIFY(!inv.adjustToCounted(m_productId, 17250, QStringLiteral("Conteo"), QString(), m_userId, &error));
        QVERIFY(!inv.adjustToCounted(m_productId, -1, QStringLiteral("Conteo"), QString(), m_userId, &error));
        QVERIFY(!inv.adjustToCounted(m_productId, 5000, QStringLiteral(" "), QString(), m_userId, &error));
        QCOMPARE(stock(m_productId), qint64(17250));
        QCOMPARE(movementCount(), 2);
    }

    // Descuento de stock tal como lo usara la venta: dentro de una transaccion del llamador.
    void saleDeductionRespectsStock()
    {
        InventoryService inv(Database::connection());
        QString error;
        QVERIFY(inv.registerPurchase(m_productId, 5000, -1, false, QString(), m_userId, &error));

        {
            Transaction tx(Database::connection());
            StockChange c;
            c.productId = m_productId;
            c.deltaMilli = -2000;
            c.type = MovementType::sale();
            c.userId = m_userId;
            qint64 after = 0;
            QVERIFY2(inv.applyChange(c, false, &after, &error), qPrintable(error));
            QCOMPARE(after, qint64(3000));
            QVERIFY(tx.commit());
        }
        QCOMPARE(stock(m_productId), qint64(3000));

        // Sin permiso para negativo: falla y no toca nada.
        StockChange big;
        big.productId = m_productId;
        big.deltaMilli = -4000;
        big.type = MovementType::sale();
        big.userId = m_userId;
        {
            Transaction tx(Database::connection());
            QVERIFY(!inv.applyChange(big, false, nullptr, &error));
            QVERIFY(error.contains(QStringLiteral("insuficiente")));
        }
        QCOMPARE(stock(m_productId), qint64(3000));

        // Con permiso (venta confirmada por el usuario): queda negativo.
        {
            Transaction tx(Database::connection());
            QVERIFY(inv.applyChange(big, true, nullptr, &error));
            QVERIFY(tx.commit());
        }
        QCOMPARE(stock(m_productId), qint64(-1000));
    }

    void rollbackUndoesStock()
    {
        InventoryService inv(Database::connection());
        QString error;
        QVERIFY(inv.registerPurchase(m_productId, 5000, -1, false, QString(), m_userId, &error));
        const int before = movementCount();
        {
            Transaction tx(Database::connection());
            StockChange c;
            c.productId = m_productId;
            c.deltaMilli = -1000;
            c.type = MovementType::sale();
            c.userId = m_userId;
            QVERIFY(inv.applyChange(c, false, nullptr, &error));
            // sin commit: rollback al salir del bloque
        }
        QCOMPARE(stock(m_productId), qint64(5000));
        QCOMPARE(movementCount(), before);
    }

    void lowStockFilter()
    {
        ProductService ps(Database::connection());
        QString error;
        Product p;
        QVERIFY(ps.products().get(m_productId, &p));
        p.minStockMilli = 2000;
        QVERIFY(ps.save(p, &error));

        InventoryService inv(Database::connection());
        QCOMPARE(inv.products().countLowStock(), 1);  // stock 0 <= 2
        QVERIFY(inv.registerPurchase(m_productId, 3000, -1, false, QString(), m_userId, &error));
        QCOMPARE(inv.products().countLowStock(), 0);

        ProductFilter f;
        f.lowStockOnly = true;
        QCOMPARE(inv.products().find(f).size(), 0);
    }
};

QTEST_GUILESS_MAIN(TstInventory)
#include "tst_inventory.moc"
