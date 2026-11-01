#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

// Imprime lineas de texto monoespaciado en un rollo de 58 u 80 mm, o las guarda como PDF.
// Usa el driver de Windows de la impresora (QPrinter): funciona con cualquier impresora instalada.
class TicketPrinter
{
public:
    static QStringList printers();
    static QString defaultPrinter();
    static QString monospaceFamily();

    // printerName vacio = impresora predeterminada del sistema.
    static bool print(const QStringList &lines, const QString &printerName, int paperWidthMm, QString *error);
    static bool exportPdf(const QStringList &lines, int paperWidthMm, const QString &filePath, QString *error);
    // Varios tickets en un PDF: cada uno empieza en una hoja nueva (y puede ocupar varias).
    static bool exportPdf(const QVector<QStringList> &tickets, int paperWidthMm, const QString &filePath,
                          QString *error);
};
