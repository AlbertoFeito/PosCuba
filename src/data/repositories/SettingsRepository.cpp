#include "SettingsRepository.h"

#include <QSqlQuery>
#include <QVariant>

SettingsRepository::SettingsRepository(QSqlDatabase db)
    : m_db(db)
{
}

QString SettingsRepository::value(const QString &key, const QString &defaultValue)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT value FROM settings WHERE key = :k"));
    q.bindValue(QStringLiteral(":k"), key);
    if (q.exec() && q.next())
        return q.value(0).toString();
    return defaultValue;
}

bool SettingsRepository::setValue(const QString &key, const QString &value)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT OR REPLACE INTO settings (key, value) VALUES (:k, :v)"));
    q.bindValue(QStringLiteral(":k"), key);
    q.bindValue(QStringLiteral(":v"), value);
    return q.exec();
}
