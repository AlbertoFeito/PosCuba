#include "Database.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QStringList>
#include <QVariant>

namespace {
QString connName() { return QString::fromLatin1(Database::ConnectionName); }
}

bool Database::open(const QString &path, QString *error)
{
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        if (error)
            *error = QStringLiteral("El driver QSQLITE no esta disponible");
        return false;
    }

    QSqlDatabase db = QSqlDatabase::contains(connName())
                          ? QSqlDatabase::database(connName(), false)
                          : QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName());
    db.setDatabaseName(path);
    db.setConnectOptions(QStringLiteral("QSQLITE_BUSY_TIMEOUT=5000"));

    if (!db.open()) {
        if (error)
            *error = db.lastError().text();
        return false;
    }

    // foreign_keys no se puede cambiar dentro de una transaccion: se fija aqui.
    const QStringList pragmas = {
        QStringLiteral("PRAGMA foreign_keys = ON"),
        QStringLiteral("PRAGMA journal_mode = WAL"),
        QStringLiteral("PRAGMA synchronous = NORMAL"),
        QStringLiteral("PRAGMA temp_store = MEMORY"),
    };
    QSqlQuery q(db);
    for (const QString &p : pragmas) {
        if (!q.exec(p)) {
            if (error)
                *error = QStringLiteral("%1: %2").arg(p, q.lastError().text());
            db.close();
            return false;
        }
    }
    return true;
}

void Database::close()
{
    if (QSqlDatabase::contains(connName())) {
        {
            QSqlDatabase db = QSqlDatabase::database(connName(), false);
            db.close();
        }
        QSqlDatabase::removeDatabase(connName());
    }
}

QSqlDatabase Database::connection()
{
    return QSqlDatabase::database(connName(), false);
}

QString Database::sqliteVersion()
{
    QSqlQuery q(connection());
    if (q.exec(QStringLiteral("SELECT sqlite_version()")) && q.next())
        return q.value(0).toString();
    return QString();
}
