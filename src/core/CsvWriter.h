#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// CSV en UTF-8 con BOM (Excel muestra bien las tildes).
// Spanish: separador ";" y coma decimal (Excel en espanol). Standard: "," y punto decimal.
class CsvWriter
{
public:
    enum Format { Spanish, Standard };

    explicit CsvWriter(Format format = Spanish);

    QChar separator() const;
    // Convierte un importe ya formateado ("1234.50") al formato de numeros elegido.
    QString number(const QString &formatted) const;
    QString escape(const QString &field) const;
    QString line(const QStringList &fields) const;
    QString build(const QStringList &header, const QVector<QStringList> &rows) const;

    bool write(const QString &path, const QStringList &header, const QVector<QStringList> &rows,
               QString *error) const;

private:
    Format m_format;
};
