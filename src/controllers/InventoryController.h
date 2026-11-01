#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QSqlDatabase>
#include <QStringList>
#include <QVariantMap>

#include "core/Session.h"
#include "core/services/InventoryService.h"
#include "models/MovementListModel.h"
#include "models/ProductListModel.h"

class InventoryController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *products READ products CONSTANT)
    Q_PROPERTY(QAbstractItemModel *movements READ movements CONSTANT)
    Q_PROPERTY(QStringList adjustReasons READ adjustReasons CONSTANT)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY filterChanged)
    Q_PROPERTY(bool lowStockOnly READ lowStockOnly WRITE setLowStockOnly NOTIFY filterChanged)
    Q_PROPERTY(bool showAllMovements READ showAllMovements WRITE setShowAllMovements NOTIFY filterChanged)
    Q_PROPERTY(int selectedProductId READ selectedProductId WRITE setSelectedProductId NOTIFY selectionChanged)
    Q_PROPERTY(QVariantMap selectedProduct READ selectedProduct NOTIFY selectionChanged)
    Q_PROPERTY(int lowStockCount READ lowStockCount NOTIFY lowStockCountChanged)

public:
    InventoryController(QSqlDatabase db, const Session *session, QObject *parent = nullptr);

    QAbstractItemModel *products() { return &m_products; }
    QAbstractItemModel *movements() { return &m_movements; }
    QStringList adjustReasons() const { return InventoryService::adjustReasons(); }

    QString searchText() const { return m_filter.text; }
    void setSearchText(const QString &text);
    bool lowStockOnly() const { return m_filter.lowStockOnly; }
    void setLowStockOnly(bool on);
    bool showAllMovements() const { return m_showAll; }
    void setShowAllMovements(bool on);
    int selectedProductId() const { return m_selectedId; }
    void setSelectedProductId(int id);
    QVariantMap selectedProduct() const { return m_selected; }
    int lowStockCount() const { return m_lowStockCount; }

    // Entrada: productId, qty, cost (texto, opcional), updateCost (bool), note. Salida: { ok, error }
    Q_INVOKABLE QVariantMap registerPurchase(const QVariantMap &form);
    // Entrada: productId, counted (texto), reason, note. Salida: { ok, error }
    Q_INVOKABLE QVariantMap adjust(const QVariantMap &form);

public slots:
    // Recarga lista, producto seleccionado, historial y contador de stock bajo.
    void reload();

signals:
    void filterChanged();
    void selectionChanged();
    void lowStockCountChanged();
    void stockChanged();   // tras una entrada o ajuste: otras pantallas deben recargar

private:
    void reloadProducts();
    void reloadSelection();
    void reloadMovements();
    void reloadLowStockCount();

    InventoryService m_service;
    const Session *m_session;
    ProductListModel m_products;
    MovementListModel m_movements;
    ProductFilter m_filter;
    bool m_showAll = false;
    int m_selectedId = 0;
    QVariantMap m_selected;
    int m_lowStockCount = 0;
};
