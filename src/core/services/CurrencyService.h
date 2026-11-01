#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "core/Currency.h"
#include "data/repositories/CurrencyRepository.h"

// Monedas y tasas de cambio. La tasa se expresa como cuanto vale 1 unidad en la moneda base
// (1 USD = 400 CUP -> rateMilli 400000). Cada pago guarda la tasa usada, asi que cambiarla
// no altera ventas anteriores.
class CurrencyService
{
public:
    explicit CurrencyService(QSqlDatabase db);

    QVector<Currency> all() { return m_repo.all(); }

    // Valida y normaliza (codigo en mayusculas). Vacio si es valida.
    static QString validate(Currency &c);

    bool add(Currency c, QString *error);
    bool update(Currency c, QString *error);
    // Solo se permite mientras no haya ventas registradas (los importes se guardan en la base).
    bool setBase(const QString &code, QString *error);
    bool hasSales();

private:
    QSqlDatabase m_db;
    CurrencyRepository m_repo;
};
