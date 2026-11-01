#include "ProductRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

const char *kSelect =
    "SELECT p.id, p.code, p.barcode, p.name, p.category_id, c.name, p.cost_cents, "
    "p.price_cents, p.unit, p.stock_milli, p.min_stock_milli, p.active "
    "FROM products p LEFT JOIN categories c ON c.id = p.category_id ";

Product readRow(const QSqlQuery &q)
{
    Product p;
    p.id = q.value(0).toInt();
    p.code = q.value(1).toString();
    p.barcode = q.value(2).toString();
    p.name = q.value(3).toString();
    p.categoryId = q.value(4).toInt();
    p.categoryName = q.value(5).toString();
    p.costCents = q.value(6).toLongLong();
    p.priceCents = q.value(7).toLongLong();
    p.unit = q.value(8).toString();
    p.stockMilli = q.value(9).toLongLong();
    p.minStockMilli = q.value(10).toLongLong();
    p.active = q.value(11).toBool();
    return p;
}

QString escapeLike(QString s)
{
    s.replace(QLatin1Char('\\'), QLatin1String("\\\\"));
    s.replace(QLatin1Char('%'), QLatin1String("\\%"));
    s.replace(QLatin1Char('_'), QLatin1String("\\_"));
    return s;
}

QVariant nullIfEmpty(const QString &s) { return s.isEmpty() ? QVariant() : QVariant(s); }
QVariant nullIfZero(int id) { return id > 0 ? QVariant(id) : QVariant(); }

} // namespace

ProductRepository::ProductRepository(QSqlDatabase db)
    : m_db(db)
{
}

QVector<Product> ProductRepository::find(const ProductFilter &f)
{
    QVector<Product> result;
    const QString text = f.text.trimmed();

    QString sql = QString::fromLatin1(kSelect) + QStringLiteral("WHERE 1=1 ");
    if (!f.includeInactive)
        sql += QStringLiteral("AND p.active = 1 ");
    if (f.categoryId > 0)
        sql += QStringLiteral("AND p.category_id = :cat ");
    if (f.lowStockOnly)
        sql += QStringLiteral("AND p.min_stock_milli > 0 AND p.stock_milli <= p.min_stock_milli ");
    if (!text.isEmpty())
        sql += QStringLiteral("AND (p.name LIKE :like ESCAPE '\\' OR p.code LIKE :prefix ESCAPE '\\' "
                              "OR p.barcode = :barcode) ");
    sql += QStringLiteral("ORDER BY p.name COLLATE NOCASE LIMIT :limit");

    QSqlQuery q(m_db);
    q.prepare(sql);
    if (f.categoryId > 0)
        q.bindValue(QStringLiteral(":cat"), f.categoryId);
    if (!text.isEmpty()) {
        const QString esc = escapeLike(text);
        q.bindValue(QStringLiteral(":like"), QStringLiteral("%") + esc + QStringLiteral("%"));
        q.bindValue(QStringLiteral(":prefix"), esc + QStringLiteral("%"));
        q.bindValue(QStringLiteral(":barcode"), text);
    }
    q.bindValue(QStringLiteral(":limit"), f.limit);

    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return result;
    }
    while (q.next())
        result.append(readRow(q));
    return result;
}

bool ProductRepository::get(int id, Product *out)
{
    QSqlQuery q(m_db);
    q.prepare(QString::fromLatin1(kSelect) + QStringLiteral("WHERE p.id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    if (!q.next())
        return false;
    if (out)
        *out = readRow(q);
    return true;
}

bool ProductRepository::getByCode(const QString &code, Product *out)
{
    QSqlQuery q(m_db);
    q.prepare(QString::fromLatin1(kSelect) + QStringLiteral("WHERE p.code = :c"));
    q.bindValue(QStringLiteral(":c"), code);
    if (!q.exec() || !q.next())
        return false;
    if (out)
        *out = readRow(q);
    return true;
}

bool ProductRepository::getByBarcode(const QString &barcode, Product *out)
{
    if (barcode.isEmpty())
        return false;
    QSqlQuery q(m_db);
    q.prepare(QString::fromLatin1(kSelect) + QStringLiteral("WHERE p.barcode = :b"));
    q.bindValue(QStringLiteral(":b"), barcode);
    if (!q.exec() || !q.next())
        return false;
    if (out)
        *out = readRow(q);
    return true;
}

bool ProductRepository::codeExists(const QString &code, int excludeId)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT 1 FROM products WHERE code = :code AND id <> :id"));
    q.bindValue(QStringLiteral(":code"), code);
    q.bindValue(QStringLiteral(":id"), excludeId);
    return q.exec() && q.next();
}

bool ProductRepository::barcodeExists(const QString &barcode, int excludeId)
{
    if (barcode.isEmpty())
        return false;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT 1 FROM products WHERE barcode = :bc AND id <> :id"));
    q.bindValue(QStringLiteral(":bc"), barcode);
    q.bindValue(QStringLiteral(":id"), excludeId);
    return q.exec() && q.next();
}

int ProductRepository::insert(const Product &p)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO products (code, barcode, name, category_id, cost_cents, price_cents, "
        "unit, min_stock_milli, active) "
        "VALUES (:code, :barcode, :name, :cat, :cost, :price, :unit, :min, :active)"));
    q.bindValue(QStringLiteral(":code"), p.code);
    q.bindValue(QStringLiteral(":barcode"), nullIfEmpty(p.barcode));
    q.bindValue(QStringLiteral(":name"), p.name);
    q.bindValue(QStringLiteral(":cat"), nullIfZero(p.categoryId));
    q.bindValue(QStringLiteral(":cost"), p.costCents);
    q.bindValue(QStringLiteral(":price"), p.priceCents);
    q.bindValue(QStringLiteral(":unit"), p.unit);
    q.bindValue(QStringLiteral(":min"), p.minStockMilli);
    q.bindValue(QStringLiteral(":active"), p.active ? 1 : 0);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return -1;
    }
    return q.lastInsertId().toInt();
}

bool ProductRepository::update(const Product &p)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "UPDATE products SET code = :code, barcode = :barcode, name = :name, category_id = :cat, "
        "cost_cents = :cost, price_cents = :price, unit = :unit, min_stock_milli = :min, "
        "updated_at = datetime('now','localtime') WHERE id = :id"));
    q.bindValue(QStringLiteral(":code"), p.code);
    q.bindValue(QStringLiteral(":barcode"), nullIfEmpty(p.barcode));
    q.bindValue(QStringLiteral(":name"), p.name);
    q.bindValue(QStringLiteral(":cat"), nullIfZero(p.categoryId));
    q.bindValue(QStringLiteral(":cost"), p.costCents);
    q.bindValue(QStringLiteral(":price"), p.priceCents);
    q.bindValue(QStringLiteral(":unit"), p.unit);
    q.bindValue(QStringLiteral(":min"), p.minStockMilli);
    q.bindValue(QStringLiteral(":id"), p.id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

bool ProductRepository::setActive(int id, bool active)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE products SET active = :a, updated_at = datetime('now','localtime') "
                             "WHERE id = :id"));
    q.bindValue(QStringLiteral(":a"), active ? 1 : 0);
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

QString ProductRepository::nextNumericCode()
{
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT COALESCE(MAX(CAST(code AS INTEGER)), 0) + 1 FROM products "
                              "WHERE code <> '' AND code NOT GLOB '*[^0-9]*' AND length(code) <= 12"))
        && q.next())
        return QString::number(q.value(0).toLongLong());
    return QStringLiteral("1");
}

bool ProductRepository::stockOf(int id, qint64 *stockMilli)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT stock_milli FROM products WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    if (!q.next())
        return false;
    if (stockMilli)
        *stockMilli = q.value(0).toLongLong();
    return true;
}

bool ProductRepository::setStock(int id, qint64 stockMilli)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE products SET stock_milli = :s, updated_at = datetime('now','localtime') "
                             "WHERE id = :id"));
    q.bindValue(QStringLiteral(":s"), stockMilli);
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

bool ProductRepository::setCost(int id, qint64 costCents)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE products SET cost_cents = :c, updated_at = datetime('now','localtime') "
                             "WHERE id = :id"));
    q.bindValue(QStringLiteral(":c"), costCents);
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

bool ProductRepository::setPrice(int id, qint64 priceCents)
{
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE products SET price_cents = :p, updated_at = datetime('now','localtime') "
                             "WHERE id = :id"));
    q.bindValue(QStringLiteral(":p"), priceCents);
    q.bindValue(QStringLiteral(":id"), id);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return q.numRowsAffected() == 1;
}

int ProductRepository::countLowStock()
{
    QSqlQuery q(m_db);
    if (q.exec(QStringLiteral("SELECT COUNT(*) FROM products WHERE active = 1 AND min_stock_milli > 0 "
                              "AND stock_milli <= min_stock_milli")) && q.next())
        return q.value(0).toInt();
    return 0;
}

bool ProductRepository::findExact(const QString &text, Product *out)
{
    const QString t = text.trimmed();
    if (t.isEmpty())
        return false;
    QSqlQuery q(m_db);
    q.prepare(QString::fromLatin1(kSelect)
              + QStringLiteral("WHERE p.active = 1 AND (p.barcode = :b OR p.code = :c) "
                               "ORDER BY (p.barcode = :b2) DESC LIMIT 1"));
    q.bindValue(QStringLiteral(":b"), t);
    q.bindValue(QStringLiteral(":c"), t);
    q.bindValue(QStringLiteral(":b2"), t);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    if (!q.next())
        return false;
    if (out)
        *out = readRow(q);
    return true;
}
