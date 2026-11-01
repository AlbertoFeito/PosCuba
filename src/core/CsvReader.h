#pragma once

#include <QByteArray>
#include <QChar>
#include <QString>
#include <QStringList>
#include <QVector>

// Lectura de CSV como lo guardan Excel y LibreOffice: separador ";" "," o tabulador,
// comillas dobles ("" dentro de comillas), saltos de linea dentro de comillas, BOM UTF-8.
namespace CsvReader {

// UTF-8 (con o sin BOM); si no es UTF-8 valido, Windows-1252/Latin-1 (Excel "CSV" en Windows en espanol).
QString decode(const QByteArray &bytes);
// Separador mas probable segun la primera linea (fuera de comillas).
QChar detectSeparator(const QString &text);
QVector<QStringList> parse(const QString &text, QChar separator);

} // namespace CsvReader
