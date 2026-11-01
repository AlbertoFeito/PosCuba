#pragma once

#include <QDate>
#include <QObject>
#include <QSqlDatabase>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "core/Session.h"
#include "core/TicketFormatter.h"
#include "data/repositories/CurrencyRepository.h"
#include "data/repositories/SaleRepository.h"
#include "data/repositories/SettingsRepository.h"

class TicketController : public QObject
{
    Q_OBJECT
    // businessName, businessAddress, businessPhone, ticketFooter, printerName, paperWidth (58|80), autoPrint
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY settingsChanged)
    Q_PROPERTY(bool autoPrint READ autoPrint NOTIFY settingsChanged)
    // "pdf" (por defecto) o "printer"
    Q_PROPERTY(bool usesPdf READ usesPdf NOTIFY settingsChanged)
    Q_PROPERTY(QStringList printers READ printers NOTIFY printersChanged)
    Q_PROPERTY(QString defaultPrinter READ defaultPrinter NOTIFY printersChanged)
    Q_PROPERTY(QString monoFamily READ monoFamily CONSTANT)
    Q_PROPERTY(QString pdfFolder READ pdfFolder CONSTANT)

public:
    TicketController(QSqlDatabase db, const Session *session, QObject *parent = nullptr);

    QVariantMap settings() const { return m_settings; }
    bool autoPrint() const { return m_settings.value(QStringLiteral("autoPrint")).toBool(); }
    bool usesPdf() const { return m_settings.value(QStringLiteral("output")).toString() != QLatin1String("printer"); }
    QStringList printers() const { return m_printers; }
    QString defaultPrinter() const { return m_defaultPrinter; }
    QString monoFamily() const;
    QString pdfFolder() const;

    Q_INVOKABLE QVariantMap saveSettings(const QVariantMap &values);
    Q_INVOKABLE void refreshPrinters();

    Q_INVOKABLE QString previewText(int saleId);
    // Vista previa de un ticket de ejemplo con ajustes aun no guardados (pantalla de configuracion).
    Q_INVOKABLE QString previewSample(const QVariantMap &values) const;

    // Salida: { ok, error, path }
    // Imprime o guarda PDF segun el ajuste "output".
    Q_INVOKABLE QVariantMap emitSale(int saleId);
    Q_INVOKABLE QVariantMap printSale(int saleId);
    // PDF del dia de esa venta (tickets_AAAA-MM-DD.pdf, una venta por hoja).
    Q_INVOKABLE QVariantMap exportSalePdf(int saleId);
    Q_INVOKABLE QVariantMap exportDayPdf(const QDate &day);
    Q_INVOKABLE QVariantMap printTest();
    Q_INVOKABLE QVariantMap exportTestPdf();

    // [{ id, number, dateText, totalText, userName, itemCount, voided }]
    Q_INVOKABLE QVariantList recentSales(int limit = 50);
    Q_INVOKABLE void openFolder(const QString &path) const;

public slots:
    void reloadSettings();

signals:
    void settingsChanged();
    void printersChanged();

private:
    TicketInfo info(const QVariantMap &values) const;
    int paperWidth(const QVariantMap &values) const;
    static SaleDetail sampleSale();
    QVariantMap printLines(const QStringList &lines);
    QVariantMap pdfLines(const QStringList &lines, const QString &fileName);

    const Session *m_session;
    SettingsRepository m_settingsRepo;
    SaleRepository m_sales;
    CurrencyRepository m_currencies;
    QVariantMap m_settings;
    QStringList m_printers;
    QString m_defaultPrinter;
    QString m_baseCurrency;
};
