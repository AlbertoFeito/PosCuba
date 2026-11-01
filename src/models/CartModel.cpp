#include "CartModel.h"

#include "core/Money.h"
#include "core/SaleCalculator.h"

CartModel::CartModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CartModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_lines.size();
}

QVariant CartModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_lines.size())
        return QVariant();
    const CartLine &l = m_lines.at(index.row());
    switch (role) {
    case ProductIdRole: return l.productId;
    case CodeRole: return l.code;
    case Qt::DisplayRole:
    case NameRole: return l.name;
    case UnitRole: return l.unit;
    case QtyTextRole: return Money::formatMilli(l.qtyMilli);
    case PriceTextRole: return Money::formatCents(l.priceCents);
    case TotalTextRole: return Money::formatCents(SaleCalculator::lineTotal(l));
    case StockShortRole: return l.qtyMilli > l.stockMilli;
    default: return QVariant();
    }
}

QHash<int, QByteArray> CartModel::roleNames() const
{
    return {
        {ProductIdRole, "productId"},
        {CodeRole, "code"},
        {NameRole, "name"},
        {UnitRole, "unit"},
        {QtyTextRole, "qtyText"},
        {PriceTextRole, "priceText"},
        {TotalTextRole, "totalText"},
        {StockShortRole, "stockShort"},
    };
}

int CartModel::add(const CartLine &line)
{
    for (int i = 0; i < m_lines.size(); ++i) {
        CartLine &l = m_lines[i];
        if (l.productId == line.productId && l.priceCents == line.priceCents) {
            l.qtyMilli = qMin(l.qtyMilli + line.qtyMilli, SaleCalculator::MaxQtyMilli);
            l.stockMilli = line.stockMilli;
            emit dataChanged(index(i), index(i));
            emit contentChanged();
            return i;
        }
    }
    const int row = m_lines.size();
    beginInsertRows(QModelIndex(), row, row);
    m_lines.append(line);
    endInsertRows();
    emit countChanged();
    emit contentChanged();
    return row;
}

bool CartModel::setQty(int row, qint64 qtyMilli)
{
    if (row < 0 || row >= m_lines.size() || qtyMilli <= 0 || qtyMilli > SaleCalculator::MaxQtyMilli)
        return false;
    m_lines[row].qtyMilli = qtyMilli;
    emit dataChanged(index(row), index(row));
    emit contentChanged();
    return true;
}

qint64 CartModel::qtyAt(int row) const
{
    return (row >= 0 && row < m_lines.size()) ? m_lines.at(row).qtyMilli : 0;
}

bool CartModel::remove(int row)
{
    if (row < 0 || row >= m_lines.size())
        return false;
    beginRemoveRows(QModelIndex(), row, row);
    m_lines.removeAt(row);
    endRemoveRows();
    emit countChanged();
    emit contentChanged();
    return true;
}

void CartModel::clear()
{
    if (m_lines.isEmpty())
        return;
    beginResetModel();
    m_lines.clear();
    endResetModel();
    emit countChanged();
    emit contentChanged();
}
