#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "core/Product.h"

class CategoryRepository
{
public:
    explicit CategoryRepository(QSqlDatabase db);

    QVector<Category> all();
    int findByName(const QString &name);     // id, o 0 si no existe (sin distinguir mayusculas)
    int insert(const QString &name);         // id, o -1 si falla
    QString lastError() const { return m_lastError; }

private:
    QSqlDatabase m_db;
    QString m_lastError;
};
