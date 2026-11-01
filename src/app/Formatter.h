#pragma once

#include <QObject>
#include <QString>

// Utilidades de formato expuestas a QML como "fmt".
// Los valores viajan como double (exactos hasta 2^53) para evitar problemas con qint64 en QML.
class Formatter : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    Q_INVOKABLE QString money(double cents) const;
    Q_INVOKABLE double parseMoney(const QString &text) const; // -1 si no es valido
    Q_INVOKABLE QString qty(double milli) const;
    Q_INVOKABLE double parseQty(const QString &text) const;   // -1 si no es valido
};
