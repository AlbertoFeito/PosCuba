#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QStringList>

// Migraciones versionadas con PRAGMA user_version.
// Cada script :/sql/NNN_*.sql se aplica en una transaccion.
class Migrator
{
public:
    static bool run(QSqlDatabase db, QString *error);
    static int currentVersion(QSqlDatabase db);
    static int latestVersion();

    // Publico para tests: separa un script en sentencias, quitando comentarios "--".
    static QStringList splitStatements(const QString &script);
};
