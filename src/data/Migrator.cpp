#include "Migrator.h"

#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

struct Migration {
    int version;
    const char *resource;
};

// Agregar nuevas migraciones al final. Nunca modificar una ya publicada.
const Migration kMigrations[] = {
    {1, ":/sql/001_init.sql"},
    {2, ":/sql/002_inventory.sql"},
    {3, ":/sql/003_cash.sql"},
    {4, ":/sql/004_users.sql"},
    {5, ":/sql/005_refunds.sql"},
};

} // namespace

int Migrator::latestVersion()
{
    int v = 0;
    for (const Migration &m : kMigrations)
        v = qMax(v, m.version);
    return v;
}

int Migrator::currentVersion(QSqlDatabase db)
{
    QSqlQuery q(db);
    if (q.exec(QStringLiteral("PRAGMA user_version")) && q.next())
        return q.value(0).toInt();
    return -1;
}

QStringList Migrator::splitStatements(const QString &script)
{
    // Los scripts no usan "--" ni ";" dentro de literales ni triggers.
    QString clean;
    const QStringList lines = script.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const int c = line.indexOf(QStringLiteral("--"));
        clean += (c >= 0 ? line.left(c) : line);
        clean += QLatin1Char('\n');
    }

    QStringList out;
    const QStringList parts = clean.split(QLatin1Char(';'));
    for (const QString &p : parts) {
        const QString s = p.trimmed();
        if (!s.isEmpty())
            out << s;
    }
    return out;
}

bool Migrator::run(QSqlDatabase db, QString *error)
{
    const int current = currentVersion(db);
    if (current < 0) {
        if (error)
            *error = QStringLiteral("No se pudo leer la version del esquema");
        return false;
    }
    if (current > latestVersion()) {
        if (error)
            *error = QStringLiteral("La base de datos (v%1) es mas nueva que la aplicacion (v%2)")
                         .arg(current).arg(latestVersion());
        return false;
    }

    for (const Migration &m : kMigrations) {
        if (m.version <= current)
            continue;

        QFile f(QString::fromLatin1(m.resource));
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            if (error)
                *error = QStringLiteral("No se encontro %1").arg(f.fileName());
            return false;
        }
        const QStringList statements = splitStatements(QString::fromUtf8(f.readAll()));

        if (!db.transaction()) {
            if (error)
                *error = db.lastError().text();
            return false;
        }

        QSqlQuery q(db);
        for (const QString &sql : statements) {
            if (!q.exec(sql)) {
                if (error)
                    *error = QStringLiteral("Migracion %1: %2\n%3")
                                 .arg(m.version)
                                 .arg(q.lastError().text(), sql.left(200));
                db.rollback();
                return false;
            }
        }
        if (!q.exec(QStringLiteral("PRAGMA user_version = %1").arg(m.version)) || !db.commit()) {
            if (error)
                *error = QStringLiteral("Migracion %1: %2").arg(m.version).arg(db.lastError().text());
            db.rollback();
            return false;
        }
    }
    return true;
}
