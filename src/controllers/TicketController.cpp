#include "TicketController.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QStandardPaths>
#include <QUrl>

#include "core/Money.h"
#include "printing/TicketPrinter.h"

namespace {
QVariantMap result(bool ok, const QString &error, const QString &path = QString())
{
    return {{QStringLiteral("ok"), ok}, {QStringLiteral("error"), error}, {QStringLiteral("path"), path}};
}

// Clave en QML -> clave en la tabla settings
const QList<QPair<QString, QString>> &keyMap()
{
    static const QList<QPair<QString, QString>> keys = {
        {QStringLiteral("businessName"), QStringLiteral("business_name")},
        {QStringLiteral("businessAddress"), QStringLiteral("business_address")},
        {QStringLiteral("businessPhone"), QStringLiteral("business_phone")},
        {QStringLiteral("ticketFooter"), QStringLiteral("ticket_footer")},
        {QStringLiteral("printerName"), QStringLiteral("printer_name")},
        {QStringLiteral("paperWidth"), QStringLiteral("paper_width")},
        {QStringLiteral("autoPrint"), QStringLiteral("auto_print")},
        {QStringLiteral("output"), QStringLiteral("ticket_output")},
    };
    return keys;
}
} // namespace

TicketController::TicketController(QSqlDatabase db, const Session *session, QObject *parent)
    : QObject(parent)
    , m_session(session)
    , m_settingsRepo(db)
    , m_sales(db)
    , m_currencies(db)
{
}

void TicketController::reloadSettings()
{
    m_settings.clear();
    for (const auto &k : keyMap())
        m_settings.insert(k.first, m_settingsRepo.value(k.second));
    if (m_settings.value(QStringLiteral("paperWidth")).toInt() != 80)
        m_settings.insert(QStringLiteral("paperWidth"), 58);
    else
        m_settings.insert(QStringLiteral("paperWidth"), 80);
    m_settings.insert(QStringLiteral("autoPrint"), m_settingsRepo.value(QStringLiteral("auto_print")) == QLatin1String("1"));
    // Sin impresora fisica el ticket se guarda como PDF (valor por defecto).
    m_settings.insert(QStringLiteral("output"),
                      m_settingsRepo.value(QStringLiteral("ticket_output")) == QLatin1String("printer")
                          ? QStringLiteral("printer") : QStringLiteral("pdf"));

    Currency base;
    m_baseCurrency = m_currencies.base(&base) ? base.code : QStringLiteral("CUP");
    emit settingsChanged();
}

void TicketController::refreshPrinters()
{
    m_printers = TicketPrinter::printers();
    m_defaultPrinter = TicketPrinter::defaultPrinter();
    emit printersChanged();
}

QString TicketController::monoFamily() const
{
    return TicketPrinter::monospaceFamily();
}

QString TicketController::pdfFolder() const
{
    const QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return QDir(docs).filePath(QStringLiteral("PosCuba/Tickets"));
}

QVariantMap TicketController::saveSettings(const QVariantMap &values)
{
    if (!m_session || !m_session->isAdmin())
        return result(false, tr("Solo un administrador puede hacer esto."));
    const int width = values.value(QStringLiteral("paperWidth")).toInt();
    if (width != 58 && width != 80)
        return result(false, tr("El ancho del papel debe ser 58 u 80 mm."));
    const QString output = values.value(QStringLiteral("output")).toString();
    if (!output.isEmpty() && output != QLatin1String("pdf") && output != QLatin1String("printer"))
        return result(false, tr("Salida de ticket no válida."));
    if (values.value(QStringLiteral("businessName")).toString().trimmed().isEmpty())
        return result(false, tr("El nombre del negocio es obligatorio."));

    for (const auto &k : keyMap()) {
        if (!values.contains(k.first))
            continue;
        QString v;
        if (k.first == QLatin1String("autoPrint"))
            v = values.value(k.first).toBool() ? QStringLiteral("1") : QStringLiteral("0");
        else
            v = values.value(k.first).toString().trimmed();
        if (!m_settingsRepo.setValue(k.second, v))
            return result(false, tr("No se pudo guardar la configuración."));
    }
    reloadSettings();
    return result(true, QString());
}

TicketInfo TicketController::info(const QVariantMap &values) const
{
    TicketInfo i;
    i.businessName = values.value(QStringLiteral("businessName")).toString();
    i.address = values.value(QStringLiteral("businessAddress")).toString();
    i.phone = values.value(QStringLiteral("businessPhone")).toString();
    i.footer = values.value(QStringLiteral("ticketFooter")).toString();
    i.baseCurrency = m_baseCurrency.isEmpty() ? QStringLiteral("CUP") : m_baseCurrency;
    return i;
}

int TicketController::paperWidth(const QVariantMap &values) const
{
    return values.value(QStringLiteral("paperWidth")).toInt() == 80 ? 80 : 58;
}

SaleDetail TicketController::sampleSale()
{
    SaleDetail s;
    s.number = 123;
    s.createdAt = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    s.userName = tr("Cajero");
    s.status = QStringLiteral("completed");
    SaleItemDetail a;
    a.name = tr("Producto de ejemplo");
    a.unit = QStringLiteral("u");
    a.qtyMilli = 2000;
    a.priceCents = 15000;
    a.lineTotalCents = 30000;
    SaleItemDetail b;
    b.name = tr("Producto por peso");
    b.unit = QStringLiteral("lb");
    b.qtyMilli = 1500;
    b.priceCents = 20000;
    b.lineTotalCents = 30000;
    s.items = {a, b};
    s.subtotal = s.total = 60000;
    PaymentDetail p;
    p.method = QStringLiteral("cash");
    p.currency = QStringLiteral("CUP");
    p.amountCents = p.baseCents = 100000;
    s.payments = {p};
    s.change = 40000;
    return s;
}

QString TicketController::previewText(int saleId)
{
    SaleDetail d;
    if (!m_sales.load(saleId, &d))
        return QString();
    return TicketFormatter::format(d, info(m_settings), TicketFormatter::columnsForPaper(paperWidth(m_settings)))
        .join(QLatin1Char('\n'));
}

QString TicketController::previewSample(const QVariantMap &values) const
{
    return TicketFormatter::format(sampleSale(), info(values), TicketFormatter::columnsForPaper(paperWidth(values)))
        .join(QLatin1Char('\n'));
}

QVariantMap TicketController::printLines(const QStringList &lines)
{
    QString error;
    if (!TicketPrinter::print(lines, m_settings.value(QStringLiteral("printerName")).toString(),
                              paperWidth(m_settings), &error))
        return result(false, error);
    return result(true, QString());
}

QVariantMap TicketController::pdfLines(const QStringList &lines, const QString &fileName)
{
    const QString path = QDir(pdfFolder()).filePath(fileName);
    QString error;
    if (!TicketPrinter::exportPdf(lines, paperWidth(m_settings), path, &error))
        return result(false, error);
    return result(true, QString(), QDir::toNativeSeparators(path));
}

QVariantMap TicketController::emitSale(int saleId)
{
    return usesPdf() ? exportSalePdf(saleId) : printSale(saleId);
}

QVariantMap TicketController::printSale(int saleId)
{
    SaleDetail d;
    if (!m_sales.load(saleId, &d))
        return result(false, tr("No se encontró la venta."));
    return printLines(TicketFormatter::format(d, info(m_settings),
                                              TicketFormatter::columnsForPaper(paperWidth(m_settings))));
}

QVariantMap TicketController::exportSalePdf(int saleId)
{
    SaleDetail d;
    if (!m_sales.load(saleId, &d))
        return result(false, tr("No se encontró la venta."));
    const QDateTime when = QDateTime::fromString(d.createdAt, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    return exportDayPdf(when.isValid() ? when.date() : QDate::currentDate());
}

QVariantMap TicketController::exportDayPdf(const QDate &day)
{
    // Un PDF por dia con todas sus ventas; cada venta empieza en hoja nueva.
    // Se regenera completo desde la base: un PDF no se puede ampliar.
    const int columns = TicketFormatter::columnsForPaper(paperWidth(m_settings));
    const TicketInfo ticketInfo = info(m_settings);
    QVector<QStringList> tickets;
    for (const int id : m_sales.idsForDay(day)) {
        SaleDetail d;
        if (m_sales.load(id, &d))
            tickets.append(TicketFormatter::format(d, ticketInfo, columns));
    }
    if (tickets.isEmpty())
        return result(false, tr("No hay ventas el %1.").arg(day.toString(QStringLiteral("dd/MM/yyyy"))));

    const QString path = QDir(pdfFolder()).filePath(
        QStringLiteral("tickets_%1.pdf").arg(day.toString(QStringLiteral("yyyy-MM-dd"))));
    QString error;
    if (!TicketPrinter::exportPdf(tickets, paperWidth(m_settings), path, &error))
        return result(false, error);
    return result(true, QString(), QDir::toNativeSeparators(path));
}

QVariantMap TicketController::printTest()
{
    return printLines(TicketFormatter::format(sampleSale(), info(m_settings),
                                              TicketFormatter::columnsForPaper(paperWidth(m_settings))));
}

QVariantMap TicketController::exportTestPdf()
{
    const QStringList lines = TicketFormatter::format(sampleSale(), info(m_settings),
                                                      TicketFormatter::columnsForPaper(paperWidth(m_settings)));
    return pdfLines(lines, QStringLiteral("ticket_prueba.pdf"));
}

QVariantList TicketController::recentSales(int limit)
{
    QVariantList out;
    for (const SaleRow &r : m_sales.recent(limit)) {
        const QDateTime dt = QDateTime::fromString(r.createdAt, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        out.append(QVariantMap{
            {QStringLiteral("id"), r.id},
            {QStringLiteral("number"), r.number},
            {QStringLiteral("dateText"), dt.isValid() ? dt.toString(QStringLiteral("dd/MM HH:mm")) : r.createdAt},
            {QStringLiteral("totalText"), Money::formatCents(r.total)},
            {QStringLiteral("userName"), r.userName},
            {QStringLiteral("itemCount"), r.itemCount},
            {QStringLiteral("voided"), r.status == QLatin1String("voided")},
        });
    }
    return out;
}

void TicketController::openFolder(const QString &path) const
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(path.isEmpty() ? pdfFolder() : path));
}
