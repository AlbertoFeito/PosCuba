#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "core/Sale.h"

class CartModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        ProductIdRole = Qt::UserRole + 1,
        CodeRole,
        NameRole,
        UnitRole,
        QtyTextRole,
        PriceTextRole,
        TotalTextRole,
        StockShortRole,
    };

    explicit CartModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_lines.size(); }
    const QVector<CartLine> &lines() const { return m_lines; }

    // Suma a la linea existente del mismo producto y precio, o agrega una nueva. Devuelve la fila.
    int add(const CartLine &line);
    bool setQty(int row, qint64 qtyMilli);
    qint64 qtyAt(int row) const;
    bool remove(int row);
    void clear();

signals:
    void countChanged();
    void contentChanged();

private:
    QVector<CartLine> m_lines;
};
