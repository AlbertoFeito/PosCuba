#pragma once

#include <QSqlDatabase>
#include <QString>

class SettingsRepository
{
public:
    explicit SettingsRepository(QSqlDatabase db);

    QString value(const QString &key, const QString &defaultValue = QString());
    bool setValue(const QString &key, const QString &value);

private:
    QSqlDatabase m_db;
};
