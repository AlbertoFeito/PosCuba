#pragma once

#include <QSqlDatabase>
#include <QString>

// Conexion unica a SQLite. Los repositorios obtienen la conexion con connection().
class Database
{
public:
    static constexpr const char *ConnectionName = "pos";

    static bool open(const QString &path, QString *error);
    static void close();
    static QSqlDatabase connection();
    static QString sqliteVersion();
};
