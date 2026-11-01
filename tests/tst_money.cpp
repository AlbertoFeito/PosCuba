#include <QtTest>

#include "core/Money.h"

class TstMoney : public QObject
{
    Q_OBJECT

private slots:
    void parseCents_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<bool>("ok");
        QTest::addColumn<qint64>("cents");

        QTest::newRow("entero") << "150" << true << qint64(15000);
        QTest::newRow("punto") << "12.5" << true << qint64(1250);
        QTest::newRow("coma") << "12,50" << true << qint64(1250);
        QTest::newRow("solo decimales") << ".75" << true << qint64(75);
        QTest::newRow("espacios") << " 1 250.00 " << true << qint64(125000);
        QTest::newRow("cero") << "0" << true << qint64(0);
        QTest::newRow("vacio") << "" << false << qint64(0);
        QTest::newRow("solo punto") << "." << false << qint64(0);
        QTest::newRow("negativo") << "-5" << false << qint64(0);
        QTest::newRow("3 decimales") << "1.005" << false << qint64(0);
        QTest::newRow("dos separadores") << "1.2.3" << false << qint64(0);
        QTest::newRow("letras") << "12a" << false << qint64(0);
        QTest::newRow("demasiado grande") << "1234567890123" << false << qint64(0);
    }

    void parseCents()
    {
        QFETCH(QString, text);
        QFETCH(bool, ok);
        QFETCH(qint64, cents);
        qint64 v = -1;
        QCOMPARE(Money::parseCents(text, &v), ok);
        if (ok)
            QCOMPARE(v, cents);
    }

    void formatCents()
    {
        QCOMPARE(Money::formatCents(0), QStringLiteral("0.00"));
        QCOMPARE(Money::formatCents(5), QStringLiteral("0.05"));
        QCOMPARE(Money::formatCents(125050), QStringLiteral("1250.50"));
        QCOMPARE(Money::formatCents(-1250), QStringLiteral("-12.50"));
    }

    void milli()
    {
        qint64 v = 0;
        QVERIFY(Money::parseMilli(QStringLiteral("0.250"), &v));
        QCOMPARE(v, qint64(250));
        QVERIFY(Money::parseMilli(QStringLiteral("2"), &v));
        QCOMPARE(v, qint64(2000));
        QVERIFY(!Money::parseMilli(QStringLiteral("0.0001"), &v));

        QCOMPARE(Money::formatMilli(2000), QStringLiteral("2"));
        QCOMPARE(Money::formatMilli(250), QStringLiteral("0.25"));
        QCOMPARE(Money::formatMilli(1005), QStringLiteral("1.005"));
        QCOMPARE(Money::formatMilli(0), QStringLiteral("0"));
    }

    void roundTrip()
    {
        for (qint64 c : {qint64(0), qint64(1), qint64(99), qint64(100), qint64(123456789)}) {
            qint64 back = -1;
            QVERIFY(Money::parseCents(Money::formatCents(c), &back));
            QCOMPARE(back, c);
        }
    }
};

QTEST_GUILESS_MAIN(TstMoney)
#include "tst_money.moc"
