#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "data/repositories/ProductRepository.h"

// Cambio de precios en lote (inflacion, nueva lista de precios).
struct PriceChangeRequest
{
    enum Mode { PercentOfPrice, MarginOnCost };
    enum Rounding { RoundUp, RoundNearest };

    int categoryId = 0;            // 0 = todas las categorias
    bool includeInactive = false;
    Mode mode = PercentOfPrice;
    int basisPoints = 0;           // porcentaje x 100: 10 % = 1000, -5.5 % = -550
    qint64 stepCents = 0;          // redondeo a multiplos (0 = sin redondeo)
    Rounding rounding = RoundUp;
};

struct PriceChange
{
    int productId = 0;
    QString code;
    QString name;
    qint64 costCents = 0;
    qint64 oldPrice = 0;
    qint64 newPrice = 0;
};

struct PricePreview
{
    QVector<PriceChange> changes;  // solo los que cambian
    int unchanged = 0;
    int skippedNoCost = 0;         // margen sobre costo sin costo cargado
};

class PricingService
{
public:
    explicit PricingService(QSqlDatabase db);

    // "10", "-5,5", "12.25" -> puntos basicos. Entre -99.99 % y 1000 %.
    static bool parsePercent(const QString &text, int *basisPoints);
    static qint64 newPrice(qint64 oldPrice, qint64 cost, const PriceChangeRequest &req);

    PricePreview preview(const PriceChangeRequest &req);
    // Aplica en una transaccion. Devuelve la cantidad de productos cambiados, o -1.
    int apply(const PriceChangeRequest &req, QString *error);

private:
    QSqlDatabase m_db;
    ProductRepository m_products;
};
