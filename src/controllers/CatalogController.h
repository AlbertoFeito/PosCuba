#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <QVariantMap>

#include "core/Session.h"
#include "core/services/PricingService.h"
#include "core/services/ProductImportService.h"

// Cambio de precios en lote e importacion/exportacion de productos (solo administradores).
// Expuesto a QML como "catalogCtrl".
class CatalogController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString folder READ folder CONSTANT)

public:
    CatalogController(QSqlDatabase db, const Session *session, QObject *parent = nullptr);

    QString folder() const;   // Documentos\PosCuba\Productos

    // form: { categoryId, includeInactive, mode: "percent"|"margin", percent, step (centavos), rounding: "up"|"nearest" }
    // Salida: { ok, error, count, unchanged, skipped, rows: [{ code, name, oldText, newText, diffText, up }] }
    Q_INVOKABLE QVariantMap pricePreview(const QVariantMap &form);
    // Salida: { ok, error, count }
    Q_INVOKABLE QVariantMap applyPrices(const QVariantMap &form);

    // format: "es" | "std". Salida: { ok, error, path, count }
    Q_INVOKABLE QVariantMap exportProducts(const QString &format, bool includeInactive);
    Q_INVOKABLE QVariantMap exportTemplate(const QString &format);

    // Archivos .csv en Documentos\PosCuba\Productos, Documentos, Descargas, Escritorio y memorias USB.
    // [{ path, name, folder, dateText }]
    Q_INVOKABLE QVariantList importCandidates() const;
    // Salida: { ok, error, newCount, updateCount, unchangedCount, errorCount, columns, ignored,
    //           rows: [{ line, status, statusText, code, name, detail }] }
    Q_INVOKABLE QVariantMap importPlan(const QString &path, bool updateStock);
    // Salida: { ok, error, created, updated, categories }
    Q_INVOKABLE QVariantMap importApply(const QString &path, bool updateStock);
    Q_INVOKABLE void openFolder() const;

signals:
    void catalogChanged();

private:
    bool isAdmin() const { return m_session && m_session->isAdmin(); }
    bool readRequest(const QVariantMap &form, PriceChangeRequest *req, QString *error) const;
    static QString localPath(const QString &pathOrUrl);
    static bool readFile(const QString &path, QString *text, QString *error);

    PricingService m_pricing;
    ProductImportService m_import;
    const Session *m_session;
};
