#include <QtTest>

#include "core/services/UserService.h"
#include "data/Database.h"
#include "data/Migrator.h"

class TstUsers : public QObject
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

    void pinRules()
    {
        QVERIFY(UserService::isValidPin(QStringLiteral("1234")));
        QVERIFY(UserService::isValidPin(QStringLiteral("12345678")));
        QVERIFY(!UserService::isValidPin(QStringLiteral("123")));
        QVERIFY(!UserService::isValidPin(QStringLiteral("123456789")));
        QVERIFY(!UserService::isValidPin(QStringLiteral("12a4")));

        const QString salt = UserService::newSalt();
        QCOMPARE(salt.size(), 32);
        QVERIFY(salt != UserService::newSalt());
        QCOMPARE(UserService::hashPin(QStringLiteral("1234"), salt), UserService::hashPin(QStringLiteral("1234"), salt));
        QVERIFY(UserService::hashPin(QStringLiteral("1234"), salt) != UserService::hashPin(QStringLiteral("1235"), salt));
        QVERIFY(UserService::hashPin(QStringLiteral("1234"), salt)
                != UserService::hashPin(QStringLiteral("1234"), UserService::newSalt()));
    }

    void firstRunSetup()
    {
        UserService svc(Database::connection());
        User admin;
        QVERIFY(svc.needsSetup(&admin));
        QCOMPARE(admin.name, QStringLiteral("Administrador"));

        QString error;
        Session s;
        QVERIFY(!svc.authenticate(admin.id, QStringLiteral("1234"), &s, &error)); // sin PIN aun
        QVERIFY(!svc.setupAdmin(QStringLiteral("1234"), QStringLiteral("4321"), &s, &error));
        QVERIFY(!svc.setupAdmin(QStringLiteral("12"), QStringLiteral("12"), &s, &error));
        QVERIFY2(svc.setupAdmin(QStringLiteral("2468"), QStringLiteral("2468"), &s, &error), qPrintable(error));
        QCOMPARE(s.userId, admin.id);
        QVERIFY(s.isAdmin());
        QVERIFY(!svc.needsSetup(nullptr));
        QVERIFY(!svc.setupAdmin(QStringLiteral("1111"), QStringLiteral("1111"), &s, &error)); // solo una vez
    }

    void createAndLogin()
    {
        UserService svc(Database::connection());
        QString error;
        QVERIFY(svc.setupAdmin(QStringLiteral("2468"), QStringLiteral("2468"), nullptr, &error));

        const int ana = svc.create(QStringLiteral("  Ana  "), Role::cashier(), QStringLiteral("1111"), &error);
        QVERIFY2(ana > 0, qPrintable(error));
        QVERIFY(svc.create(QStringLiteral("ana"), Role::cashier(), QStringLiteral("2222"), &error) < 0); // duplicado
        QVERIFY(svc.create(QStringLiteral("Luis"), QStringLiteral("jefe"), QStringLiteral("2222"), &error) < 0);
        QVERIFY(svc.create(QStringLiteral("Luis"), Role::cashier(), QStringLiteral("22"), &error) < 0);

        Session s;
        QVERIFY(svc.authenticate(ana, QStringLiteral("1111"), &s, &error));
        QCOMPARE(s.userName, QStringLiteral("Ana"));
        QVERIFY(!s.isAdmin());
        QVERIFY(!svc.authenticate(ana, QStringLiteral("9999"), &s, &error));

        QVERIFY(svc.changeOwnPin(ana, QStringLiteral("1111"), QStringLiteral("5555"), &error));
        QVERIFY(!svc.changeOwnPin(ana, QStringLiteral("1111"), QStringLiteral("6666"), &error));
        QVERIFY(svc.authenticate(ana, QStringLiteral("5555"), &s, &error));
    }

    void lockoutAfterFailures()
    {
        UserService svc(Database::connection());
        QString error;
        const int id = svc.create(QStringLiteral("Pepe"), Role::cashier(), QStringLiteral("1357"), &error);
        for (int i = 0; i < UserService::MaxAttempts; ++i)
            QVERIFY(!svc.authenticate(id, QStringLiteral("0000"), nullptr, &error));
        QVERIFY(svc.secondsLocked(id) > 0);
        // Bloqueado: ni el PIN correcto entra.
        QVERIFY(!svc.authenticate(id, QStringLiteral("1357"), nullptr, &error));
        QVERIFY(error.contains(QStringLiteral("Espera")));
        // Un administrador le asigna PIN nuevo: se levanta el bloqueo.
        QVERIFY(svc.resetPin(id, QStringLiteral("8642"), &error));
        QCOMPARE(svc.secondsLocked(id), 0);
        QVERIFY(svc.authenticate(id, QStringLiteral("8642"), nullptr, &error));
    }

    void lastAdminIsProtected()
    {
        UserService svc(Database::connection());
        QString error;
        Session admin;
        QVERIFY(svc.setupAdmin(QStringLiteral("2468"), QStringLiteral("2468"), &admin, &error));
        const int cashier = svc.create(QStringLiteral("Ana"), Role::cashier(), QStringLiteral("1111"), &error);

        QVERIFY(!svc.update(admin.userId, QStringLiteral("Administrador"), Role::cashier(), &error));
        QVERIFY(!svc.setActive(admin.userId, false, cashier, &error));     // ultimo admin
        QVERIFY(!svc.setActive(admin.userId, false, admin.userId, &error)); // a si mismo

        // Con un segundo admin si se puede degradar al primero.
        const int boss = svc.create(QStringLiteral("Jefa"), Role::admin(), QStringLiteral("9999"), &error);
        QVERIFY(boss > 0);
        QVERIFY2(svc.update(admin.userId, QStringLiteral("Admin viejo"), Role::cashier(), &error), qPrintable(error));

        // Usuario desactivado no puede entrar; reactivado si.
        QVERIFY(svc.setActive(cashier, false, boss, &error));
        QVERIFY(!svc.authenticate(cashier, QStringLiteral("1111"), nullptr, &error));
        QCOMPARE(svc.list(false).size(), 2);
        QCOMPARE(svc.list(true).size(), 3);
        QVERIFY(svc.setActive(cashier, true, boss, &error));
        QVERIFY(svc.authenticate(cashier, QStringLiteral("1111"), nullptr, &error));
    }
};

QTEST_GUILESS_MAIN(TstUsers)
#include "tst_users.moc"
