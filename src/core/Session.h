#pragma once

#include <QString>

// Usuario que opera la aplicacion. Lo fija el login (hito de usuarios);
// mientras tanto, el administrador inicial.
struct Session
{
    int userId = 0;
    QString userName;
    QString role;

    bool isValid() const { return userId > 0; }
    bool isAdmin() const { return role == QLatin1String("admin"); }
};
