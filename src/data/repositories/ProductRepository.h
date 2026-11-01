#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "core/Product.h"

struct ProductFilter
{
    QString text;          // nombre (contiene), codigo (prefijo) o codigo de barras (exacto)
    int categoryId = 0;    // 0 = todas
    bool includeInactive = false;
    bool lowStockOnly = false;
    int limit = 1000;
};

class ProductRepository
{
public:
    explicit ProductRepository(QSqlDatabase db);

    QVector<Product> find(const ProductFilter &filter);
    bool get(int id, Product *out);
    // Producto activo cuyo codigo o codigo de barras es exactamente text.
    bool findExact(const QString &text, Product *out);
    bool codeExists(const QString &code, int excludeId);
    bool barcodeExists(const QString &barcode, int excludeId);
    int insert(const Product &p);            // id, o -1 si falla
    bool update(const Product &p);           // no toca el stock
    bool setActive(int id, bool active);
    QString nextNumericCode();

    // Inventario (usar dentro de una transaccion).
    bool stockOf(int id, qint64 *stockMilli);
    bool setStock(int id, qint64 stockMilli);
    bool setCost(int id, qint64 costCents);
    int countLowStock();

    QString lastError() const { return m_lastError; }

private:
    QSqlDatabase m_db;
    QString m_lastError;
};
