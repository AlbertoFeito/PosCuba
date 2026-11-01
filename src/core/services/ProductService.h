#pragma once

#include <QSqlDatabase>
#include <QString>

#include "core/Product.h"
#include "data/repositories/CategoryRepository.h"
#include "data/repositories/ProductRepository.h"

class ProductService
{
public:
    explicit ProductService(QSqlDatabase db);

    // Valida, comprueba duplicados e inserta (id == 0) o actualiza. Asigna p.id.
    bool save(Product &p, QString *error);
    bool setActive(int id, bool active, QString *error);
    // Crea la categoria o devuelve la existente con el mismo nombre.
    int ensureCategory(const QString &name, QString *error);

    ProductRepository &products() { return m_products; }
    CategoryRepository &categories() { return m_categories; }

private:
    ProductRepository m_products;
    CategoryRepository m_categories;
};
