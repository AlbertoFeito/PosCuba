#pragma once

#include <QString>

namespace Role {
inline QString admin() { return QStringLiteral("admin"); }
inline QString cashier() { return QStringLiteral("cashier"); }
} // namespace Role

struct User
{
    int id = 0;
    QString name;
    QString role;
    bool active = true;
    bool hasPin = false;
    QString createdAt;
    QString lastLoginAt;
};
