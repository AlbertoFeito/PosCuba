#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "core/Product.h"

// Lista de categorias para ComboBox. La fila 0 es un marcador con id 0
// ("Todas" para filtros, "Sin categoria" para formularios).
class CategoryListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles { IdRole = Qt::UserRole + 1, NameRole };

    explicit CategoryListModel(const QString &placeholder, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setCategories(const QVector<Category> &categories);

    Q_INVOKABLE int idAt(int row) const;
    Q_INVOKABLE int rowOfId(int id) const;

private:
    QVector<Category> m_items; // incluye el marcador en la posicion 0
    QString m_placeholder;
};
