#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QSqlDatabase>
#include <QStringList>
#include <QVariantMap>

#include "core/Session.h"
#include "core/services/ProductService.h"
#include "models/CategoryListModel.h"
#include "models/ProductListModel.h"

class ProductController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *products READ products CONSTANT)
    Q_PROPERTY(QAbstractItemModel *categoryFilterModel READ categoryFilterModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel *categoryModel READ categoryModel CONSTANT)
    Q_PROPERTY(QStringList units READ units CONSTANT)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY filterChanged)
    Q_PROPERTY(int categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY filterChanged)
    Q_PROPERTY(bool showInactive READ showInactive WRITE setShowInactive NOTIFY filterChanged)

public:
    ProductController(QSqlDatabase db, const Session *session, QObject *parent = nullptr);

    QAbstractItemModel *products() { return &m_products; }
    QAbstractItemModel *categoryFilterModel() { return &m_categoryFilter; }
    QAbstractItemModel *categoryModel() { return &m_categoryForm; }
    QStringList units() const;

    QString searchText() const { return m_filter.text; }
    void setSearchText(const QString &text);
    int categoryFilter() const { return m_filter.categoryId; }
    void setCategoryFilter(int id);
    bool showInactive() const { return m_filter.includeInactive; }
    void setShowInactive(bool show);

    Q_INVOKABLE void reload();
    Q_INVOKABLE void reloadCategories();
    // Mapa con los campos del formulario como texto; vacio si no existe.
    Q_INVOKABLE QVariantMap get(int id);
    // Entrada: id, code, barcode, name, categoryId, cost, price, unit, minStock (textos).
    // Salida: { ok, error, id }
    Q_INVOKABLE QVariantMap save(const QVariantMap &form);
    // Salida: { ok, error, id }
    Q_INVOKABLE QVariantMap setActive(int id, bool active);
    // Salida: { ok, error, id }
    Q_INVOKABLE QVariantMap addCategory(const QString &name);
    Q_INVOKABLE QString nextCode();

signals:
    void filterChanged();
    void productsChanged();   // tras guardar o (des)activar: otras pantallas deben recargar

private:
    bool isAdmin() const { return m_session && m_session->isAdmin(); }

    ProductService m_service;
    const Session *m_session;
    ProductListModel m_products;
    CategoryListModel m_categoryFilter;
    CategoryListModel m_categoryForm;
    ProductFilter m_filter;
};
