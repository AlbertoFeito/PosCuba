#include <QtTest>

#include <QSqlQuery>

#include "core/services/CashService.h"
#include "core/services/ExpenseService.h"
#include "core/services/ReportService.h"
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
} // namespace

// Gastos: de caja o externos, borrado, gastos fijos mensuales y ganancia neta.
class TstExpenses : public QObject
{
    Q_OBJECT

    int m_user = 0;
    int m_transport = 0;
    int m_rent = 0;

    Expense make(qint64 amount, bool fromCash, const QDate &date = QDate::currentDate())
    {
        Expense e;
        e.date = date;
        e.categoryId = m_transport;
        e.description = QStringLiteral("Taxi a la tienda");
        e.amountCents = amount;
        e.fromCash = fromCash;
        return e;
    }

private slots:
    void init()
    {
        Database::close();
        QString error;
        QVERIFY2(Database::open(QStringLiteral(":memory:"), &error), qPrintable(error));
        QVERIFY2(Migrator::run(Database::connection(), &error), qPrintable(error));
        m_user = int(scalar(QStringLiteral("SELECT id FROM users LIMIT 1")));
        ExpenseService svc(Database::connection());
        m_transport = svc.expenses().categoryId(QStringLiteral("transporte"));
        m_rent = svc.expenses().categoryId(QStringLiteral("Alquiler"));
        QVERIFY(m_transport > 0 && m_rent > 0);
    }

    void cleanupTestCase() { Database::close(); }

    void validation()
    {
        const QDate today(2026, 11, 21);
        Expense e;
        e.date = today;
        e.categoryId = 1;
        e.description = QStringLiteral("x");
        e.amountCents = 100;
        QVERIFY(ExpenseService::validate(e, today).isEmpty());
        e.date = today.addDays(1);
        QVERIFY(!ExpenseService::validate(e, today).isEmpty());   // futuro
        e.date = today.addDays(-1);
        e.fromCash = true;
        QVERIFY(!ExpenseService::validate(e, today).isEmpty());   // de caja y no es de hoy
        e.fromCash = false;
        e.amountCents = 0;
        QVERIFY(!ExpenseService::validate(e, today).isEmpty());
    }

    void fromCashCreatesWithdrawalAndCanBeUndoneWhileOpen()
    {
        ExpenseService svc(Database::connection());
        QString error;
        Expense noCash = make(5000, true);
        QVERIFY(!svc.add(noCash, m_user, &error));        // sin caja abierta
        CashService cash(Database::connection());
        QVERIFY(cash.open(20000, m_user, nullptr, &error));
        Expense tooMuch = make(30000, true);
        QVERIFY(!svc.add(tooMuch, m_user, &error));        // no hay tanto efectivo
        Expense e = make(5000, true);
        QVERIFY2(svc.add(e, m_user, &error), qPrintable(error));
        QVERIFY(e.cashMovementId > 0);
        CashSession s;
        QVERIFY(cash.current(&s));
        QCOMPARE(cash.summary(s).expected.first().expectedCents, qint64(15000));
        QVERIFY2(svc.remove(e.id, &error), qPrintable(error));
        QCOMPARE(cash.summary(s).expected.first().expectedCents, qint64(20000));
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM cash_movements")), qint64(0));

        // Despues de cerrar la caja ya no se borra
        Expense kept = make(5000, true);
        QVERIFY(svc.add(kept, m_user, &error));
        QVERIFY(cash.close({{QStringLiteral("CUP"), 15000}}, m_user, nullptr, &error));
        QVERIFY(!svc.remove(kept.id, &error));
        QVERIFY(error.contains(QStringLiteral("cerró")));
    }

    void outsideCashAnyPastDate()
    {
        ExpenseService svc(Database::connection());
        QString error;
        Expense e = make(150000, false, QDate::currentDate().addDays(-3));
        QVERIFY2(svc.add(e, m_user, &error), qPrintable(error));
        QCOMPARE(e.cashMovementId, 0);
        QVERIFY(svc.remove(e.id, &error));
    }

    void recurringGeneratesEachMonthOnce()
    {
        ExpenseService svc(Database::connection());
        QString error;
        RecurringExpense r;
        r.categoryId = m_rent;
        r.description = QStringLiteral("Alquiler del local");
        r.amountCents = 500000;
        r.dayOfMonth = 31;                                   // en meses cortos, el ultimo dia
        QVERIFY2(svc.saveRecurring(r, QDate(2026, 1, 10), &error), qPrintable(error));
        QCOMPARE(r.startMonth, QStringLiteral("2026-01"));

        QCOMPARE(svc.generateDue(QDate(2026, 1, 20)), 0);    // todavia no es dia 31
        QCOMPARE(svc.generateDue(QDate(2026, 3, 5)), 2);     // 31 ene y 28 feb
        QCOMPARE(svc.generateDue(QDate(2026, 3, 5)), 0);     // no repite
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM expenses WHERE expense_date = '2026-02-28'")), qint64(1));
        QVERIFY(svc.setRecurringActive(r.id, false, &error));
        QCOMPARE(svc.generateDue(QDate(2026, 6, 30)), 0);    // desactivado
        QVERIFY(svc.setRecurringActive(r.id, true, &error, QDate(2026, 4, 10)));   // marzo quedo en pausa
        QCOMPARE(svc.generateDue(QDate(2026, 4, 30)), 1);    // solo 30 abr (abril tiene 30)
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM expenses WHERE expense_date LIKE '2026-03%'")), qint64(0));
    }

    void netProfitInReports()
    {
        ExpenseService svc(Database::connection());
        QString error;
        Expense a = make(30000, false);
        QVERIFY(svc.add(a, m_user, &error));
        Expense b = make(20000, false);
        b.categoryId = m_rent;
        QVERIFY(svc.add(b, m_user, &error));
        ReportService rep(Database::connection());
        const QDate today = QDate::currentDate();
        const ReportSummary s = rep.summary(today, today);
        QCOMPARE(s.expenses, qint64(50000));
        QCOMPARE(s.netProfit, qint64(-50000));
        const QVector<PeriodRow> rows = rep.byPeriod(today, today, ReportGroup::Day);
        QCOMPARE(rows.first().expenses, qint64(50000));
        const QVector<ExpenseTotal> totals = svc.expenses().totals(today, today);
        QCOMPARE(totals.size(), 2);
        QCOMPARE(totals.first().amountCents, qint64(30000));   // mayor primero
        QCOMPARE(svc.ensureCategory(QStringLiteral("  transporte "), &error), m_transport);
    }
};

QTEST_GUILESS_MAIN(TstExpenses)
#include "tst_expenses.moc"
