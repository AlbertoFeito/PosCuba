#pragma once

#include <QString>
#include <QStringList>

#include "core/Product.h"

namespace ProductValidator {

QStringList units();

// Normaliza espacios y devuelve un mensaje de error, o vacio si es valido.
QString validate(Product &p);

inline bool priceBelowCost(const Product &p) { return p.priceCents < p.costCents; }

} // namespace ProductValidator
