#include "ProductListModel.h"

#include "core/Money.h"

ProductListModel::ProductListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ProductListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant ProductListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const Product &p = m_items.at(index.row());
    switch (role) {
    case IdRole: return p.id;
    case CodeRole: return p.code;
    case BarcodeRole: return p.barcode;
    case Qt::DisplayRole:
    case NameRole: return p.name;
    case CategoryIdRole: return p.categoryId;
    case CategoryNameRole: return p.categoryName;
    case CostTextRole: return Money::formatCents(p.costCents);
    case PriceTextRole: return Money::formatCents(p.priceCents);
    case UnitRole: return p.unit;
    case StockTextRole: return Money::formatMilli(p.stockMilli);
    case MinStockTextRole: return Money::formatMilli(p.minStockMilli);
    case LowStockRole: return p.isLowStock();
    case ActiveRole: return p.active;
    default: return QVariant();
    }
}

QHash<int, QByteArray> ProductListModel::roleNames() const
{
    return {
        {IdRole, "productId"},
        {CodeRole, "code"},
        {BarcodeRole, "barcode"},
        {NameRole, "name"},
        {CategoryIdRole, "categoryId"},
        {CategoryNameRole, "categoryName"},
        {CostTextRole, "costText"},
        {PriceTextRole, "priceText"},
        {UnitRole, "unit"},
        {StockTextRole, "stockText"},
        {MinStockTextRole, "minStockText"},
        {LowStockRole, "lowStock"},
        {ActiveRole, "active"},
    };
}

void ProductListModel::setProducts(const QVector<Product> &products)
{
    const int old = m_items.size();
    beginResetModel();
    m_items = products;
    endResetModel();
    if (old != m_items.size())
        emit countChanged();
}

int ProductListModel::rowOfId(int id) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).id == id)
            return i;
    }
    return -1;
}

int ProductListModel::idAt(int row) const
{
    return (row >= 0 && row < m_items.size()) ? m_items.at(row).id : 0;
}
