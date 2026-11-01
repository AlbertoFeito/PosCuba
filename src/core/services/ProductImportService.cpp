#include "ProductImportService.h"

#include <QCoreApplication>
#include <QHash>
#include <QMap>
#include <QRegularExpression>

#include "core/CsvReader.h"
#include "core/HelpCatalog.h"
#include "core/Money.h"
#include "core/ProductValidator.h"
#include "data/Transaction.h"

namespace {
QString tr(const char *s) { return QCoreApplication::translate("ProductImportService", s); }

QString key(const QString &header)
{
    QString k = HelpCatalog::fold(header);
    k.replace(QLatin1Char('_'), QLatin1Char(' '));
    k.replace(QLatin1Char('-'), QLatin1Char(' '));
    k.replace(QLatin1Char('.'), QLatin1Char(' '));
    k.remove(QLatin1Char('*'));
    return k.simplified();
}

const QHash<QString, int> &aliases()
{
    using namespace ImportColumn;
    static const QHash<QString, int> map = {
        {QStringLiteral("codigo"), Code}, {QStringLiteral("code"), Code}, {QStringLiteral("cod"), Code},
        {QStringLiteral("codigo interno"), Code}, {QStringLiteral("ref"), Code}, {QStringLiteral("referencia"), Code},
        {QStringLiteral("sku"), Code},
        {QStringLiteral("codigo de barras"), Barcode}, {QStringLiteral("codigo barras"), Barcode},
        {QStringLiteral("barras"), Barcode}, {QStringLiteral("barcode"), Barcode}, {QStringLiteral("ean"), Barcode},
        {QStringLiteral("nombre"), Name}, {QStringLiteral("name"), Name}, {QStringLiteral("producto"), Name},
        {QStringLiteral("descripcion"), Name}, {QStringLiteral("articulo"), Name},
        {QStringLiteral("categoria"), CategoryName}, {QStringLiteral("category"), CategoryName},
        {QStringLiteral("familia"), CategoryName}, {QStringLiteral("grupo"), CategoryName},
        {QStringLiteral("unidad"), Unit}, {QStringLiteral("unit"), Unit}, {QStringLiteral("um"), Unit},
        {QStringLiteral("u m"), Unit}, {QStringLiteral("unidad de medida"), Unit},
        {QStringLiteral("costo"), Cost}, {QStringLiteral("cost"), Cost}, {QStringLiteral("precio de costo"), Cost},
        {QStringLiteral("precio costo"), Cost}, {QStringLiteral("costo unitario"), Cost},
        {QStringLiteral("precio"), Price}, {QStringLiteral("price"), Price},
        {QStringLiteral("precio de venta"), Price}, {QStringLiteral("precio venta"), Price},
        {QStringLiteral("pvp"), Price}, {QStringLiteral("venta"), Price},
        {QStringLiteral("stock minimo"), MinStock}, {QStringLiteral("minimo"), MinStock},
        {QStringLiteral("stock min"), MinStock}, {QStringLiteral("min"), MinStock},
        {QStringLiteral("stock"), Stock}, {QStringLiteral("existencia"), Stock},
        {QStringLiteral("existencias"), Stock}, {QStringLiteral("cantidad"), Stock},
        {QStringLiteral("inventario"), Stock},
        {QStringLiteral("activo"), Active}, {QStringLiteral("active"), Active}, {QStringLiteral("estado"), Active},
    };
    return map;
}

bool parseUnit(const QString &text, QString *unit)
{
    const QString k = key(text);
    static const QHash<QString, QString> map = {
        {QStringLiteral("u"), QStringLiteral("u")}, {QStringLiteral("un"), QStringLiteral("u")},
        {QStringLiteral("und"), QStringLiteral("u")}, {QStringLiteral("ud"), QStringLiteral("u")},
        {QStringLiteral("uds"), QStringLiteral("u")}, {QStringLiteral("unidad"), QStringLiteral("u")},
        {QStringLiteral("unidades"), QStringLiteral("u")},
        {QStringLiteral("kg"), QStringLiteral("kg")}, {QStringLiteral("kilo"), QStringLiteral("kg")},
        {QStringLiteral("kilos"), QStringLiteral("kg")}, {QStringLiteral("kilogramo"), QStringLiteral("kg")},
        {QStringLiteral("lb"), QStringLiteral("lb")}, {QStringLiteral("libra"), QStringLiteral("lb")},
        {QStringLiteral("libras"), QStringLiteral("lb")},
        {QStringLiteral("l"), QStringLiteral("l")}, {QStringLiteral("lt"), QStringLiteral("l")},
        {QStringLiteral("litro"), QStringLiteral("l")}, {QStringLiteral("litros"), QStringLiteral("l")},
        {QStringLiteral("m"), QStringLiteral("m")}, {QStringLiteral("metro"), QStringLiteral("m")},
        {QStringLiteral("metros"), QStringLiteral("m")},
    };
    const auto it = map.constFind(k);
    if (it == map.constEnd())
        return false;
    *unit = it.value();
    return true;
}

bool parseBool(const QString &text, bool *value)
{
    const QString k = key(text);
    if (k == QLatin1String("si") || k == QLatin1String("1") || k == QLatin1String("true")
        || k == QLatin1String("activo") || k == QLatin1String("x") || k == QLatin1String("yes")) {
        *value = true;
        return true;
    }
    if (k == QLatin1String("no") || k == QLatin1String("0") || k == QLatin1String("false")
        || k == QLatin1String("inactivo")) {
        *value = false;
        return true;
    }
    return false;
}

QString cell(const QStringList &row, int index)
{
    return index >= 0 && index < row.size() ? row.at(index).trimmed() : QString();
}

} // namespace

ProductImportService::ProductImportService(QSqlDatabase db)
    : m_db(db)
    , m_products(db)
    , m_inventory(db)
{
}

QString ProductImportService::normalizeNumber(const QString &text)
{
    QString t = text.trimmed();
    t.remove(QRegularExpression(QStringLiteral("[\\s$€]|CUP|USD|MLC")));
    const int lastDot = t.lastIndexOf(QLatin1Char('.'));
    const int lastComma = t.lastIndexOf(QLatin1Char(','));
    if (lastDot >= 0 && lastComma >= 0) {
        // El ultimo separador es el decimal; el otro, de miles.
        if (lastComma > lastDot) {
            t.remove(QLatin1Char('.'));
            t.replace(QLatin1Char(','), QLatin1Char('.'));
        } else {
            t.remove(QLatin1Char(','));
        }
    } else if (lastComma >= 0) {
        t.replace(QLatin1Char(','), QLatin1Char('.'));
    }
    return t;
}

int ProductImportService::columnFor(const QString &header)
{
    return aliases().value(key(header), -1);
}

QString ProductImportService::columnName(int column)
{
    using namespace ImportColumn;
    switch (column) {
    case Code: return tr("Código");
    case Barcode: return tr("Código de barras");
    case Name: return tr("Nombre");
    case CategoryName: return tr("Categoría");
    case Unit: return tr("Unidad");
    case Cost: return tr("Costo");
    case Price: return tr("Precio");
    case MinStock: return tr("Stock mínimo");
    case Stock: return tr("Stock");
    case Active: return tr("Activo");
    default: return QString();
    }
}

ImportPlan ProductImportService::plan(const QString &csvText, bool updateStock)
{
    using namespace ImportColumn;
    ImportPlan plan;
    const QVector<QStringList> rows = CsvReader::parse(csvText, CsvReader::detectSeparator(csvText));
    if (rows.isEmpty()) {
        plan.error = tr("El archivo está vacío.");
        return plan;
    }

    int col[Count];
    for (int &c : col)
        c = -1;
    const QStringList header = rows.first();
    for (int i = 0; i < header.size(); ++i) {
        const QString h = header.at(i).trimmed();
        if (h.isEmpty())
            continue;
        const int c = columnFor(h);
        if (c < 0 || col[c] >= 0) {
            plan.ignored << h;
            continue;
        }
        col[c] = i;
        plan.columns << columnName(c);
    }
    if (col[Code] < 0 && col[Name] < 0) {
        plan.error = tr("No se reconoce el encabezado: la primera fila debe tener al menos las columnas "
                        "«Código» o «Nombre». Usa la plantilla.");
        return plan;
    }

    QHash<QString, int> codesInFile;     // codigo -> linea
    QHash<QString, int> barcodesInFile;
    for (int r = 1; r < rows.size(); ++r) {
        const QStringList &row = rows.at(r);
        bool empty = true;
        for (const QString &v : row)
            empty = empty && v.trimmed().isEmpty();
        if (empty)
            continue;

        ImportRow ir;
        ir.line = r + 1;
        auto fail = [&](const QString &msg) {
            ir.status = ImportRow::Error;
            ir.error = msg;
        };

        const QString code = cell(row, col[Code]);
        const QString barcode = cell(row, col[Barcode]);
        Product existing;
        bool found = false;
        if (!code.isEmpty())
            found = m_products.products().getByCode(code, &existing);
        else if (!barcode.isEmpty())
            found = m_products.products().getByBarcode(barcode, &existing);

        Product p = found ? existing : Product();
        if (!found) {
            p.code = code;
            ir.autoCode = code.isEmpty();
        }

        // Campos presentes en el archivo. Una celda vacia no borra el valor que ya tiene el producto.
        if (!barcode.isEmpty())
            p.barcode = barcode;
        if (col[Name] >= 0 && !cell(row, col[Name]).isEmpty())
            p.name = cell(row, col[Name]).simplified();
        if (col[Unit] >= 0 && !cell(row, col[Unit]).isEmpty()) {
            QString unit;
            if (!parseUnit(cell(row, col[Unit]), &unit))
                fail(tr("Unidad no válida: «%1» (usa u, kg, lb, l o m).").arg(cell(row, col[Unit])));
            else
                p.unit = unit;
        }
        auto money = [&](int column, qint64 *out, const QString &what) {
            if (column < 0 || cell(row, column).isEmpty())
                return;
            qint64 v = 0;
            if (!Money::parseCents(normalizeNumber(cell(row, column)), &v))
                fail(tr("%1 no válido: «%2».").arg(what, cell(row, column)));
            else
                *out = v;
        };
        auto qty = [&](int column, qint64 *out, const QString &what) -> bool {
            if (column < 0 || cell(row, column).isEmpty())
                return false;
            qint64 v = 0;
            if (!Money::parseMilli(normalizeNumber(cell(row, column)), &v)) {
                fail(tr("%1 no válido: «%2».").arg(what, cell(row, column)));
                return false;
            }
            *out = v;
            return true;
        };
        money(col[Cost], &p.costCents, tr("Costo"));
        money(col[Price], &p.priceCents, tr("Precio"));
        qty(col[MinStock], &p.minStockMilli, tr("Stock mínimo"));
        ir.hasStock = qty(col[Stock], &ir.stockMilli, tr("Stock"));
        if (col[Active] >= 0 && !cell(row, col[Active]).isEmpty()) {
            bool active = true;
            if (!parseBool(cell(row, col[Active]), &active))
                fail(tr("Activo no válido: «%1» (usa sí o no).").arg(cell(row, col[Active])));
            else
                p.active = active;
        }
        if (col[CategoryName] >= 0 && !cell(row, col[CategoryName]).isEmpty()) {
            ir.setsCategory = true;
            ir.categoryName = cell(row, col[CategoryName]).simplified();
            p.categoryName = ir.categoryName;
        }

        if (ir.status == ImportRow::Error && !ir.error.isEmpty()) {
            ir.product = p;
            plan.rows.append(ir);
            continue;
        }
        if (!found && (col[Price] < 0 || cell(row, col[Price]).isEmpty())) {
            fail(tr("Producto nuevo sin precio de venta."));
        } else if (!found && p.name.isEmpty()) {
            fail(tr("Producto nuevo sin nombre."));
        } else if (!code.isEmpty() && codesInFile.contains(code)) {
            fail(tr("Código repetido en el archivo (línea %1).").arg(codesInFile.value(code)));
        } else if (!p.barcode.isEmpty() && barcodesInFile.contains(p.barcode)) {
            fail(tr("Código de barras repetido en el archivo (línea %1).").arg(barcodesInFile.value(p.barcode)));
        } else {
            Product check = p;
            if (ir.autoCode)
                check.code = QStringLiteral("0");   // se asigna al importar
            const QString invalid = ProductValidator::validate(check);
            if (!invalid.isEmpty()) {
                fail(invalid);
            } else if (!p.barcode.isEmpty() && m_products.products().barcodeExists(p.barcode, p.id)) {
                fail(tr("El código de barras %1 ya es de otro producto.").arg(p.barcode));
            }
        }
        if (!code.isEmpty())
            codesInFile.insert(code, ir.line);
        if (!p.barcode.isEmpty())
            barcodesInFile.insert(p.barcode, ir.line);

        if (ir.error.isEmpty()) {
            if (!found) {
                ir.status = ImportRow::New;
                if (ir.autoCode)
                    ir.changes << tr("código automático");
                if (ir.hasStock && ir.stockMilli > 0)
                    ir.changes << tr("entrada de %1 %2").arg(Money::formatMilli(ir.stockMilli), p.unit);
            } else {
                auto diff = [&](const QString &what, const QString &a, const QString &b) {
                    if (a != b)
                        ir.changes << QStringLiteral("%1 %2 → %3").arg(what, a.isEmpty() ? QStringLiteral("—") : a,
                                                                           b.isEmpty() ? QStringLiteral("—") : b);
                };
                diff(tr("nombre"), existing.name, p.name);
                diff(tr("código de barras"), existing.barcode, p.barcode);
                if (ir.setsCategory && HelpCatalog::fold(existing.categoryName) != HelpCatalog::fold(ir.categoryName))
                    diff(tr("categoría"), existing.categoryName, ir.categoryName);
                diff(tr("unidad"), existing.unit, p.unit);
                diff(tr("costo"), Money::formatCents(existing.costCents), Money::formatCents(p.costCents));
                diff(tr("precio"), Money::formatCents(existing.priceCents), Money::formatCents(p.priceCents));
                diff(tr("mínimo"), Money::formatMilli(existing.minStockMilli), Money::formatMilli(p.minStockMilli));
                if (existing.active != p.active)
                    ir.changes << (p.active ? tr("reactivar") : tr("desactivar"));
                ir.oldStockMilli = existing.stockMilli;
                if (updateStock && ir.hasStock && ir.stockMilli != existing.stockMilli)
                    diff(tr("stock"), Money::formatMilli(existing.stockMilli), Money::formatMilli(ir.stockMilli));
                ir.status = ir.changes.isEmpty() ? ImportRow::Unchanged : ImportRow::Update;
            }
        }
        ir.product = p;
        plan.rows.append(ir);
    }

    for (const ImportRow &r : plan.rows) {
        switch (r.status) {
        case ImportRow::New: ++plan.newCount; break;
        case ImportRow::Update: ++plan.updateCount; break;
        case ImportRow::Unchanged: ++plan.unchangedCount; break;
        default: ++plan.errorCount; break;
        }
    }
    if (plan.rows.isEmpty())
        plan.error = tr("El archivo no tiene filas de productos debajo del encabezado.");
    return plan;
}

ImportResult ProductImportService::apply(const QString &csvText, bool updateStock, int userId)
{
    ImportResult res;
    const ImportPlan pl = plan(csvText, updateStock);
    if (!pl.usable()) {
        res.error = pl.error;
        return res;
    }
    if (userId <= 0) {
        res.error = tr("No hay un usuario activo.");
        return res;
    }

    Transaction tx(m_db);
    if (!tx.isActive()) {
        res.error = tr("No se pudo iniciar la transacción.");
        return res;
    }

    QHash<QString, int> categories;   // nombre plegado -> id
    for (const Category &c : m_products.categories().all())
        categories.insert(HelpCatalog::fold(c.name), c.id);

    QString error;
    for (const ImportRow &r : pl.rows) {
        if (r.status != ImportRow::New && r.status != ImportRow::Update)
            continue;
        Product p = r.product;
        if (r.setsCategory) {
            if (r.categoryName.isEmpty()) {
                p.categoryId = 0;
            } else {
                const QString k = HelpCatalog::fold(r.categoryName);
                if (!categories.contains(k)) {
                    const int id = m_products.ensureCategory(r.categoryName, &error);
                    if (id <= 0) {
                        res.error = tr("Línea %1: %2").arg(r.line).arg(error);
                        return res;
                    }
                    categories.insert(k, id);
                    ++res.categoriesCreated;
                }
                p.categoryId = categories.value(k);
            }
        }
        if (r.autoCode)
            p.code = m_products.products().nextNumericCode();
        const bool isNew = p.id == 0;
        if (!m_products.save(p, &error)) {
            res.error = tr("Línea %1: %2").arg(r.line).arg(error);
            return res;
        }
        if (!p.active && isNew)
            m_products.setActive(p.id, false, nullptr);

        // Stock: entrada para los nuevos; ajuste para los existentes si se pidio.
        qint64 delta = 0;
        QString type;
        if (isNew && r.hasStock && r.stockMilli > 0) {
            delta = r.stockMilli;
            type = MovementType::purchase();
        } else if (!isNew && updateStock && r.hasStock && r.stockMilli != r.oldStockMilli) {
            delta = r.stockMilli - r.oldStockMilli;
            type = MovementType::adjust();
        }
        if (delta != 0) {
            StockChange c;
            c.productId = p.id;
            c.deltaMilli = delta;
            c.type = type;
            c.costCents = type == MovementType::purchase() ? p.costCents : -1;
            c.userId = userId;
            c.note = tr("Importación de productos");
            if (!m_inventory.applyChange(c, true, nullptr, &error)) {
                res.error = tr("Línea %1: %2").arg(r.line).arg(error);
                return res;
            }
        }
        if (isNew)
            ++res.created;
        else
            ++res.updated;
    }

    if (!tx.commit()) {
        res.error = tr("No se pudo confirmar la importación.");
        return res;
    }
    res.ok = true;
    return res;
}

bool ProductImportService::exportTo(const QString &path, CsvWriter::Format format, bool includeInactive, int *count,
                                    QString *error)
{
    const CsvWriter csv(format);
    ProductFilter f;
    f.includeInactive = includeInactive;
    f.limit = 1000000;
    QVector<QStringList> rows;
    for (const Product &p : m_products.products().find(f)) {
        rows.append({p.code, p.barcode, p.name, p.categoryName, p.unit, csv.number(Money::formatCents(p.costCents)),
                     csv.number(Money::formatCents(p.priceCents)), csv.number(Money::formatMilli(p.minStockMilli)),
                     csv.number(Money::formatMilli(p.stockMilli)), p.active ? tr("sí") : tr("no")});
    }
    if (count)
        *count = rows.size();
    return csv.write(path, {tr("Código"), tr("Código de barras"), tr("Nombre"), tr("Categoría"), tr("Unidad"),
                            tr("Costo"), tr("Precio"), tr("Stock mínimo"), tr("Stock"), tr("Activo")},
                     rows, error);
}

bool ProductImportService::writeTemplate(const QString &path, CsvWriter::Format format, QString *error)
{
    const CsvWriter csv(format);
    const QVector<QStringList> rows = {
        {QStringLiteral("1001"), QStringLiteral("7501055300"), tr("Refresco de cola 355 ml"), tr("Bebidas"),
         QStringLiteral("u"), csv.number(QStringLiteral("80.00")), csv.number(QStringLiteral("120.00")),
         csv.number(QStringLiteral("6")), csv.number(QStringLiteral("24")), tr("sí")},
        {QString(), QString(), tr("Arroz"), tr("Víveres"), QStringLiteral("lb"), csv.number(QStringLiteral("180.00")),
         csv.number(QStringLiteral("250.00")), csv.number(QStringLiteral("10")), csv.number(QStringLiteral("50.5")),
         tr("sí")},
    };
    return csv.write(path, {tr("Código"), tr("Código de barras"), tr("Nombre"), tr("Categoría"), tr("Unidad"),
                            tr("Costo"), tr("Precio"), tr("Stock mínimo"), tr("Stock"), tr("Activo")},
                     rows, error);
}
