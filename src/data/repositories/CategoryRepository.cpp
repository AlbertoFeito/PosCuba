#include "CategoryRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

CategoryRepository::CategoryRepository(QSqlDatabase db)
    : m_db(db)
{
}

QVector<Category> CategoryRepository::all()
{
    QVector<Category> out;
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral("SELECT id, name FROM categories WHERE active = 1 "
                               "ORDER BY name COLLATE NOCASE"))) {
        m_lastError = q.lastError().text();
        return out;
    }
    while (q.next()) {
        Category c;
        c.id = q.value(0).toInt();
        c.name = q.value(1).toString();
        out.append(c);
    }
    return out;
}

int CategoryRepository::findByName(const QString &name)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT id FROM categories WHERE name = :n COLLATE NOCASE"));
    q.bindValue(QStringLiteral(":n"), name);
    if (q.exec() && q.next())
        return q.value(0).toInt();
    return 0;
}

int CategoryRepository::insert(const QString &name)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("INSERT INTO categories (name) VALUES (:n)"));
    q.bindValue(QStringLiteral(":n"), name);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}
