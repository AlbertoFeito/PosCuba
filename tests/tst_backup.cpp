#include <QtTest>

#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "core/services/BackupService.h"
#include "core/services/ProductService.h"
#include "data/Database.h"
#include "data/Migrator.h"

namespace {
qint64 scalar(const QString &sql, QSqlDatabase db = Database::connection())
{
    QSqlQuery q(db);
    if (q.exec(sql) && q.next())
        return q.value(0).toLongLong();
    return -999;
}

void addProduct(const QString &code, const QString &name)
{
    ProductService ps(Database::connection());
    Product p;
    p.code = code;
    p.name = name;
    p.priceCents = 1000;
    QString error;
    QVERIFY2(ps.save(p, &error), qPrintable(error));
}
} // namespace

class TstBackup : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;

    QString folder(const QString &name) const { return m_dir.filePath(name); }

private slots:
    void init()
    {
        Database::close();
        QFile::remove(folder(QStringLiteral("pos.db")));
        QString error;
        QVERIFY(Database::open(folder(QStringLiteral("pos.db")), &error));   // archivo real, modo WAL
        QVERIFY(Migrator::run(Database::connection(), &error));
    }

    void cleanupTestCase() { Database::close(); }

    void namingAndAutoDue()
    {
        const QDateTime t(QDate(2026, 11, 1), QTime(16, 5, 9));
        QCOMPARE(BackupService::fileNameFor(BackupService::kindManual(), t),
                 QStringLiteral("PosCuba_respaldo_2026-11-01_160509.db"));
        QCOMPARE(BackupService::fileNameFor(BackupService::kindAuto(), t),
                 QStringLiteral("PosCuba_auto_2026-11-01_160509.db"));
        QVERIFY(BackupService::isAutoDue(QDateTime(), t));
        QVERIFY(!BackupService::isAutoDue(t.addSecs(-3600), t));
        QVERIFY(BackupService::isAutoDue(t.addDays(-1), t));
    }

    void backupAndInspect()
    {
        addProduct(QStringLiteral("1"), QStringLiteral("Arroz"));
        addProduct(QStringLiteral("2"), QStringLiteral("Frijoles"));
        BackupService svc(Database::connection());
        QString path, error;
        QVERIFY2(svc.backup(folder(QStringLiteral("respaldos")), BackupService::kindManual(), &path, &error),
                 qPrintable(error));
        QVERIFY(QFileInfo::exists(path));

        const BackupInfo info = svc.inspect(path);
        QVERIFY2(info.valid, qPrintable(info.error));
        QCOMPARE(info.products, 2);
        QCOMPARE(info.schemaVersion, Migrator::latestVersion());
        QCOMPARE(info.kind, BackupService::kindManual());

        // Dos respaldos seguidos no se pisan.
        QString path2;
        QVERIFY(svc.backup(folder(QStringLiteral("respaldos")), BackupService::kindManual(), &path2, &error));
        QVERIFY(path2 != path);
        QCOMPARE(svc.list(folder(QStringLiteral("respaldos"))).size(), 2);
    }

    void restoreReplacesData()
    {
        addProduct(QStringLiteral("1"), QStringLiteral("Arroz"));
        BackupService svc(Database::connection());
        QString backupPath, error;
        QVERIFY(svc.backup(folder(QStringLiteral("r1")), BackupService::kindManual(), &backupPath, &error));

        // Cambios posteriores al respaldo
        addProduct(QStringLiteral("2"), QStringLiteral("Aceite"));
        QSqlQuery q(Database::connection());
        QVERIFY(q.exec(QStringLiteral("UPDATE products SET name = 'Arroz cambiado' WHERE code = '1'")));
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM products")), qint64(2));

        QString safety;
        QVERIFY2(svc.restore(backupPath, folder(QStringLiteral("seguridad")), &safety, &error), qPrintable(error));
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM products")), qint64(1));
        QSqlQuery n(Database::connection());
        QVERIFY(n.exec(QStringLiteral("SELECT name FROM products")) && n.next());
        QCOMPARE(n.value(0).toString(), QStringLiteral("Arroz"));

        // La copia de seguridad previa tiene los datos que habia antes de restaurar.
        const BackupInfo pre = svc.inspect(safety);
        QVERIFY(pre.valid);
        QCOMPARE(pre.products, 2);
        QCOMPARE(pre.kind, BackupService::kindPreRestore());

        // La base sigue operativa y con claves foraneas activas.
        QCOMPARE(scalar(QStringLiteral("PRAGMA foreign_keys")), qint64(1));
        addProduct(QStringLiteral("3"), QStringLiteral("Sal"));
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM products")), qint64(2));
    }

    void rejectsBadFiles()
    {
        addProduct(QStringLiteral("1"), QStringLiteral("Arroz"));
        BackupService svc(Database::connection());
        QString error, safety;

        // No es SQLite
        const QString junk = folder(QStringLiteral("basura.db"));
        QFile f(junk);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("esto no es una base de datos");
        f.close();
        QVERIFY(!svc.inspect(junk).valid);
        QVERIFY(!svc.restore(junk, folder(QStringLiteral("seg")), &safety, &error));

        // Respaldo de una version mas nueva del programa
        QString newer;
        QVERIFY(svc.backup(folder(QStringLiteral("r2")), BackupService::kindManual(), &newer, &error));
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("edit"));
            db.setDatabaseName(newer);
            QVERIFY(db.open());
            QSqlQuery q(db);
            QVERIFY(q.exec(QStringLiteral("PRAGMA user_version = 999")));
            db.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("edit"));
        const BackupInfo info = svc.inspect(newer);
        QVERIFY(!info.valid);
        QVERIFY(info.error.contains(QStringLiteral("más nueva")));
        QVERIFY(!svc.restore(newer, folder(QStringLiteral("seg")), &safety, &error));

        // Nada cambio
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM products")), qint64(1));
    }

    void restoreOlderSchemaBackup()
    {
        // Simula un respaldo de una version anterior: base creada solo hasta la migracion 1.
        const QString old = folder(QStringLiteral("viejo/PosCuba_respaldo_viejo.db"));
        QDir().mkpath(QFileInfo(old).absolutePath());
        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("old"));
            db.setDatabaseName(old);
            QVERIFY(db.open());
            QFile sql(QStringLiteral(":/sql/001_init.sql"));
            QVERIFY(sql.open(QIODevice::ReadOnly));
            QSqlQuery q(db);
            for (const QString &s : Migrator::splitStatements(QString::fromUtf8(sql.readAll())))
                QVERIFY2(q.exec(s), qPrintable(q.lastError().text()));
            QVERIFY(q.exec(QStringLiteral("PRAGMA user_version = 1")));
            QVERIFY(q.exec(QStringLiteral("INSERT INTO products (code, name, price_cents) VALUES ('9', 'Viejo', 500)")));
            db.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("old"));

        BackupService svc(Database::connection());
        QString error, safety;
        QVERIFY2(svc.restore(old, folder(QStringLiteral("seg")), &safety, &error), qPrintable(error));
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM products WHERE name = 'Viejo'")), qint64(1));
        // Las migraciones posteriores crearon el administrador inicial en el respaldo.
        QCOMPARE(scalar(QStringLiteral("SELECT COUNT(*) FROM users WHERE role = 'admin'")), qint64(1));
        QCOMPARE(scalar(QStringLiteral("PRAGMA user_version")), qint64(Migrator::latestVersion()));
    }

    void pruneKeepsManual()
    {
        const QString dir = folder(QStringLiteral("rotacion"));
        QDir().mkpath(dir);
        BackupService svc(Database::connection());
        const QDateTime base(QDate(2026, 10, 1), QTime(9, 0));
        for (int i = 0; i < 5; ++i) {
            const QString p = QDir(dir).filePath(BackupService::fileNameFor(BackupService::kindAuto(), base.addDays(i)));
            QFile f(p);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("x");
            f.close();
            QVERIFY(f.open(QIODevice::ReadWrite));
            QVERIFY(f.setFileTime(base.addDays(i), QFileDevice::FileModificationTime));
            f.close();
        }
        QString manual, error;
        QVERIFY(svc.backup(dir, BackupService::kindManual(), &manual, &error));

        QCOMPARE(svc.prune(dir, 3), 2);
        const QVector<BackupInfo> left = svc.list(dir);
        QCOMPARE(left.size(), 4);
        int autos = 0;
        for (const BackupInfo &b : left) {
            if (b.kind == BackupService::kindAuto()) {
                ++autos;
                QVERIFY(b.created.date() >= QDate(2026, 10, 3));   // quedan los 3 mas nuevos
            }
        }
        QCOMPARE(autos, 3);
        QVERIFY(QFileInfo::exists(manual));
    }
};

QTEST_GUILESS_MAIN(TstBackup)
#include "tst_backup.moc"
