#include "MovementListModel.h"

#include <QDateTime>

#include "core/Money.h"

MovementListModel::MovementListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int MovementListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QString MovementListModel::typeText(const QString &type)
{
    if (type == MovementType::purchase())
        return tr("Entrada");
    if (type == MovementType::sale())
        return tr("Venta");
    if (type == MovementType::adjust())
        return tr("Ajuste");
    if (type == MovementType::voidSale())
        return tr("Devolución");
    return type;
}

QVariant MovementListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const StockMovement &m = m_items.at(index.row());
    switch (role) {
    case IdRole: return m.id;
    case DateTextRole: {
        const QDateTime dt = QDateTime::fromString(m.createdAt, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        return dt.isValid() ? dt.toString(QStringLiteral("dd/MM/yyyy HH:mm")) : m.createdAt;
    }
    case TypeRole: return m.type;
    case TypeTextRole: return typeText(m.type);
    case QtyTextRole:
        return (m.qtyMilli > 0 ? QStringLiteral("+") : QStringLiteral("-")) + Money::formatMilli(qAbs(m.qtyMilli));
    case IsIncreaseRole: return m.qtyMilli > 0;
    case CostTextRole: return m.costCents >= 0 ? Money::formatCents(m.costCents) : QString();
    case StockAfterTextRole:
        return (m.stockAfterMilli < 0 ? QStringLiteral("-") : QString()) + Money::formatMilli(qAbs(m.stockAfterMilli));
    case UserNameRole: return m.userName;
    case NoteRole: return m.note;
    case ProductNameRole: return m.productName;
    case UnitRole: return m.unit;
    default: return QVariant();
    }
}

QHash<int, QByteArray> MovementListModel::roleNames() const
{
    return {
        {IdRole, "movementId"},
        {DateTextRole, "dateText"},
        {TypeRole, "type"},
        {TypeTextRole, "typeText"},
        {QtyTextRole, "qtyText"},
        {IsIncreaseRole, "isIncrease"},
        {CostTextRole, "costText"},
        {StockAfterTextRole, "stockAfterText"},
        {UserNameRole, "userName"},
        {NoteRole, "note"},
        {ProductNameRole, "productName"},
        {UnitRole, "unit"},
    };
}

void MovementListModel::setMovements(const QVector<StockMovement> &movements)
{
    const int old = m_items.size();
    beginResetModel();
    m_items = movements;
    endResetModel();
    if (old != m_items.size())
        emit countChanged();
}
