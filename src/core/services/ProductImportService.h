#pragma once

#include <QSet>
#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QVector>

#include "core/CsvWriter.h"
#include "core/Product.h"
#include "core/services/InventoryService.h"
#include "core/services/ProductService.h"

// Importacion y exportacion del catalogo de productos en CSV.
// Las columnas se reconocen por su encabezado (con o sin tildes, en espanol o ingles).
// Un producto existente se busca por codigo (o, si no hay codigo, por codigo de barras) y se
// actualizan solo las columnas presentes en el archivo; una celda vacia no cambia el valor.
// Sin codigo se le asigna el siguiente numero.
namespace ImportColumn {
enum Column { Code, Barcode, Name, CategoryName, Unit, Cost, Price, MinStock, Stock, Active, Count };
}

struct ImportRow
{
    enum Status { New, Update, Unchanged, Error };

    int line = 0;                 // linea del archivo (1 = encabezado)
    Status status = Error;
    Product product;              // como quedara (id > 0 si existe)
    QString categoryName;         // categoria a usar o crear ("" = sin cambio / sin categoria)
    bool setsCategory = false;
    bool hasStock = false;
    qint64 stockMilli = 0;
    qint64 oldStockMilli = 0;
    bool autoCode = false;
    QString error;
    QStringList changes;          // "precio 120.00 → 130.00"
};

struct ImportPlan
{
    QString error;                // archivo no usable (sin encabezado reconocible, vacio)
    QStringList columns;          // reconocidas, en el orden del archivo
    QStringList ignored;          // encabezados no reconocidos
    QVector<ImportRow> rows;
    int newCount = 0;
    int updateCount = 0;
    int unchangedCount = 0;
    int errorCount = 0;

    bool usable() const { return error.isEmpty(); }
};

struct ImportResult
{
    bool ok = false;
    int created = 0;
    int updated = 0;
    int categoriesCreated = 0;
    QString error;
};

class ProductImportService
{
public:
    explicit ProductImportService(QSqlDatabase db);

    // "1.234,50" "1,234.50" "1234.5" "$ 120" -> texto con punto decimal y sin miles
    static QString normalizeNumber(const QString &text);
    static int columnFor(const QString &header);   // ImportColumn o -1
    static QString columnName(int column);

    // updateStock: la columna stock tambien ajusta el stock de los productos existentes
    // (en los nuevos siempre se registra como entrada).
    ImportPlan plan(const QString &csvText, bool updateStock);
    ImportResult apply(const QString &csvText, bool updateStock, int userId);

    // CSV con todos los productos (encabezados en espanol, los que reconoce la importacion).
    bool exportTo(const QString &path, CsvWriter::Format format, bool includeInactive, int *count, QString *error);
    static bool writeTemplate(const QString &path, CsvWriter::Format format, QString *error);

private:
    QSqlDatabase m_db;
    ProductService m_products;
    InventoryService m_inventory;
};
