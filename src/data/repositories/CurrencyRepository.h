#pragma once

#include <QSqlDatabase>
#include <QVector>

#include "core/Currency.h"

class CurrencyRepository
{
public:
    explicit CurrencyRepository(QSqlDatabase db);

    QVector<Currency> active();          // la base primero
    QVector<Currency> all();             // la base primero, luego activas
    bool insert(const Currency &c);
    bool update(const Currency &c);      // nombre, simbolo, tasa y activa
    bool setBase(const QString &code);   // en una transaccion del llamador
    bool base(Currency *out);
    bool get(const QString &code, Currency *out);

    QString lastError() const { return m_lastError; }

private:
    QSqlDatabase m_db;
    QString m_lastError;
};
