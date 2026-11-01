#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "core/StockMovement.h"

class MovementListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        DateTextRole,
        TypeRole,
        TypeTextRole,
        QtyTextRole,
        IsIncreaseRole,
        CostTextRole,
        StockAfterTextRole,
        UserNameRole,
        NoteRole,
        ProductNameRole,
        UnitRole,
    };

    explicit MovementListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setMovements(const QVector<StockMovement> &movements);
    int count() const { return m_items.size(); }

    static QString typeText(const QString &type);

signals:
    void countChanged();

private:
    QVector<StockMovement> m_items;
};
