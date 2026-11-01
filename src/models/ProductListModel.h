#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "core/Product.h"

class ProductListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        CodeRole,
        BarcodeRole,
        NameRole,
        CategoryIdRole,
        CategoryNameRole,
        CostTextRole,
        PriceTextRole,
        UnitRole,
        StockTextRole,
        MinStockTextRole,
        LowStockRole,
        ActiveRole,
    };

    explicit ProductListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setProducts(const QVector<Product> &products);
    int count() const { return m_items.size(); }

    Q_INVOKABLE int rowOfId(int id) const;
    Q_INVOKABLE int idAt(int row) const;

signals:
    void countChanged();

private:
    QVector<Product> m_items;
};
