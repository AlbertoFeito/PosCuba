#include "TicketPrinter.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QMarginsF>
#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPrinter>
#include <QPrinterInfo>

#include <cmath>

namespace {

QString tr(const char *s) { return QCoreApplication::translate("TicketPrinter", s); }

// Ancho imprimible tipico de las termicas: 48 mm en rollos de 58 y 72 mm en rollos de 80.
double contentWidthMm(int paperWidthMm) { return paperWidthMm >= 80 ? 72.0 : 48.0; }
double paperWidth(int paperWidthMm) { return paperWidthMm >= 80 ? 80.0 : 58.0; }
int columnsFor(int paperWidthMm) { return paperWidthMm >= 80 ? 48 : 32; }

constexpr double kMarginMm = 3.0;
constexpr double kMinHeightMm = 40.0;
constexpr double kMaxHeightMm = 297.0;   // hoja maxima; un ticket mas largo sigue en otra hoja

void setPage(QPrinter &printer, int paperWidthMm, double heightMm)
{
    const double side = (paperWidth(paperWidthMm) - contentWidthMm(paperWidthMm)) / 2.0;
    printer.setPageSize(QPageSize(QSizeF(paperWidth(paperWidthMm), heightMm), QPageSize::Millimeter,
                                  QStringLiteral("Ticket"), QPageSize::ExactMatch));
    printer.setPageMargins(QMarginsF(side, kMarginMm, side, kMarginMm), QPageLayout::Millimeter);
}

double paintHeightPx(const QPrinter &printer)
{
    return printer.pageLayout().paintRectPixels(printer.resolution()).height();
}

// Fuente monoespaciada ajustada para que "columns" caracteres llenen el ancho imprimible.
QFont fitFont(QPrinter &printer, int paperWidthMm)
{
    const int columns = columnsFor(paperWidthMm);
    const double available = printer.pageLayout().paintRectPixels(printer.resolution()).width();
    QFont font(TicketPrinter::monospaceFamily());
    font.setStyleHint(QFont::TypeWriter);
    font.setFixedPitch(true);
    font.setPixelSize(qMax(4, int(available / columns * 1.6)));
    QFontMetricsF fm(font, &printer);
    const QString sample(columns, QLatin1Char('M'));
    for (int i = 0; i < 3; ++i) {
        const double w = fm.horizontalAdvance(sample);
        if (w <= 0)
            break;
        font.setPixelSize(qMax(4, int(font.pixelSize() * available / w)));
        fm = QFontMetricsF(font, &printer);
    }
    while (font.pixelSize() > 4 && fm.horizontalAdvance(sample) > available) {
        font.setPixelSize(font.pixelSize() - 1);
        fm = QFontMetricsF(font, &printer);
    }
    return font;
}

// Dibuja cada ticket empezando en una hoja nueva. La hoja se ajusta al largo del ticket
// (hasta kMaxHeightMm); si no cabe, el ticket continua en hojas siguientes.
bool render(QPrinter &printer, const QVector<QStringList> &tickets, int paperWidthMm, QString *error)
{
    if (tickets.isEmpty()) {
        if (error)
            *error = tr("No hay tickets para generar.");
        return false;
    }

    printer.setDocName(QStringLiteral("Ticket"));
    printer.setFullPage(false);
    setPage(printer, paperWidthMm, kMaxHeightMm);

    const QFont font = fitFont(printer, paperWidthMm);
    QFont bold = font;
    bold.setBold(true);
    const QFontMetricsF fm(font, &printer);
    const double lineHeight = fm.lineSpacing();
    const double lineMm = lineHeight * 25.4 / printer.resolution();

    QPainter painter;
    for (int t = 0; t < tickets.size(); ++t) {
        const QStringList &lines = tickets.at(t);
        const double needed = lines.size() * lineMm + 2 * kMarginMm + 1.0;
        setPage(printer, paperWidthMm, qBound(kMinHeightMm, needed, kMaxHeightMm));

        if (t == 0) {
            if (!painter.begin(&printer)) {
                if (error)
                    *error = tr("No se pudo iniciar la impresión. Revisa que la impresora esté encendida y conectada.");
                return false;
            }
        } else if (!printer.newPage()) {
            if (error)
                *error = tr("No se pudo crear una hoja nueva.");
            painter.end();
            return false;
        }

        double y = 0;
        double pageHeight = paintHeightPx(printer);
        for (const QString &line : lines) {
            if (y + lineHeight > pageHeight) {   // ticket largo: sigue en otra hoja del mismo tamano
                printer.newPage();
                y = 0;
                pageHeight = paintHeightPx(printer);
            }
            painter.setFont(line.startsWith(QLatin1String("TOTAL")) ? bold : font);
            painter.drawText(QPointF(0, y + fm.ascent()), line);
            y += lineHeight;
        }
    }
    painter.end();
    return true;
}

} // namespace

QStringList TicketPrinter::printers()
{
    return QPrinterInfo::availablePrinterNames();
}

QString TicketPrinter::defaultPrinter()
{
    return QPrinterInfo::defaultPrinterName();
}

QString TicketPrinter::monospaceFamily()
{
    return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
}

bool TicketPrinter::print(const QStringList &lines, const QString &printerName, int paperWidthMm, QString *error)
{
    const QString name = printerName.isEmpty() ? defaultPrinter() : printerName;
    if (name.isEmpty()) {
        if (error)
            *error = tr("No hay impresoras instaladas. Usa la exportación a PDF.");
        return false;
    }
    const QPrinterInfo info = QPrinterInfo::printerInfo(name);
    if (info.isNull()) {
        if (error)
            *error = tr("No se encontró la impresora «%1».").arg(name);
        return false;
    }

    QPrinter printer(info, QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::NativeFormat);
    return render(printer, {lines}, paperWidthMm, error);
}

bool TicketPrinter::exportPdf(const QStringList &lines, int paperWidthMm, const QString &filePath, QString *error)
{
    return exportPdf(QVector<QStringList>{lines}, paperWidthMm, filePath, error);
}

bool TicketPrinter::exportPdf(const QVector<QStringList> &tickets, int paperWidthMm, const QString &filePath,
                              QString *error)
{
    QDir().mkpath(QFileInfo(filePath).absolutePath());
    // Se escribe a un temporal y se reemplaza al final: si algo falla no se pierde el PDF anterior.
    const QString temp = filePath + QStringLiteral(".tmp");
    QFile::remove(temp);
    {
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(temp);
        if (!render(printer, tickets, paperWidthMm, error)) {
            QFile::remove(temp);
            return false;
        }
    }
    if (!QFileInfo::exists(temp)) {
        if (error)
            *error = tr("No se pudo crear el PDF en %1.").arg(QDir::toNativeSeparators(filePath));
        return false;
    }
    QFile::remove(filePath);
    if (!QFile::rename(temp, filePath)) {
        QFile::remove(temp);
        if (error)
            *error = tr("No se pudo guardar %1. ¿Está abierto en otro programa?").arg(QDir::toNativeSeparators(filePath));
        return false;
    }
    return true;
}
