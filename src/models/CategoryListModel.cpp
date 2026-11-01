#include "CategoryListModel.h"

CategoryListModel::CategoryListModel(const QString &placeholder, QObject *parent)
    : QAbstractListModel(parent)
    , m_placeholder(placeholder)
{
    setCategories({});
}

int CategoryListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant CategoryListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();
    const Category &c = m_items.at(index.row());
    if (role == IdRole)
        return c.id;
    if (role == NameRole || role == Qt::DisplayRole)
        return c.name;
    return QVariant();
}

QHash<int, QByteArray> CategoryListModel::roleNames() const
{
    return {{IdRole, "categoryId"}, {NameRole, "name"}};
}

void CategoryListModel::setCategories(const QVector<Category> &categories)
{
    beginResetModel();
    m_items.clear();
    Category placeholder;
    placeholder.name = m_placeholder;
    m_items.append(placeholder);
    m_items += categories;
    endResetModel();
}

int CategoryListModel::idAt(int row) const
{
    return (row >= 0 && row < m_items.size()) ? m_items.at(row).id : 0;
}

int CategoryListModel::rowOfId(int id) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).id == id)
            return i;
    }
    return 0;
}
