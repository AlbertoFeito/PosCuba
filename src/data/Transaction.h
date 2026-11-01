#pragma once

#include <QSqlDatabase>

// Transaccion RAII: hace rollback al salir del ambito si no se llamo commit().
class Transaction
{
public:
    explicit Transaction(QSqlDatabase db)
        : m_db(db)
        , m_active(m_db.transaction())
    {
    }
    ~Transaction()
    {
        if (m_active)
            m_db.rollback();
    }
    Transaction(const Transaction &) = delete;
    Transaction &operator=(const Transaction &) = delete;

    bool isActive() const { return m_active; }
    bool commit()
    {
        if (!m_active)
            return false;
        m_active = false;
        return m_db.commit();
    }

private:
    QSqlDatabase m_db;
    bool m_active;
};
