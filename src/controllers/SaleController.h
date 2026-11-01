#pragma once

#include <QAbstractItemModel>
#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <QVariantMap>

#include "core/Session.h"
#include "core/services/CashService.h"
#include "core/services/SaleService.h"
#include "data/repositories/CurrencyRepository.h"
#include "data/repositories/ProductRepository.h"
#include "models/CartModel.h"
#include "models/ProductListModel.h"

class SaleController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *cart READ cart CONSTANT)
    Q_PROPERTY(QAbstractItemModel *searchResults READ searchResults CONSTANT)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(int itemCount READ itemCount NOTIFY totalsChanged)
    Q_PROPERTY(QString subtotalText READ subtotalText NOTIFY totalsChanged)
    Q_PROPERTY(QString discountText READ discountText NOTIFY totalsChanged)
    Q_PROPERTY(QString discountLabel READ discountLabel NOTIFY totalsChanged)
    Q_PROPERTY(QString totalText READ totalText NOTIFY totalsChanged)
    Q_PROPERTY(double totalCents READ totalCents NOTIFY totalsChanged)
    Q_PROPERTY(bool hasOpenSession READ hasOpenSession NOTIFY sessionChanged)
    Q_PROPERTY(QVariantList currencies READ currencies NOTIFY currenciesChanged)
    Q_PROPERTY(QString baseCurrency READ baseCurrency NOTIFY currenciesChanged)

public:
    SaleController(QSqlDatabase db, const Session *session, QObject *parent = nullptr);

    QAbstractItemModel *cart() { return &m_cart; }
    QAbstractItemModel *searchResults() { return &m_results; }
    QString searchText() const { return m_searchText; }
    void setSearchText(const QString &text);

    int itemCount() const { return m_cart.count(); }
    QString subtotalText() const;
    QString discountText() const;
    QString discountLabel() const;
    QString totalText() const;
    double totalCents() const { return static_cast<double>(total()); }
    bool hasOpenSession() const { return m_hasOpenSession; }
    QVariantList currencies() const { return m_currencies; }
    QString baseCurrency() const { return m_baseCurrency; }

    // "codigo", "codigo de barras" o "3*codigo". Salida: { ok, row, error, notFound }
    Q_INVOKABLE QVariantMap addByCode(const QString &text);
    Q_INVOKABLE QVariantMap addProduct(int productId, const QString &qtyText = QStringLiteral("1"));
    Q_INVOKABLE QVariantMap setQty(int row, const QString &qtyText);
    Q_INVOKABLE bool changeQty(int row, int deltaUnits);
    Q_INVOKABLE QString qtyTextAt(int row) const;
    Q_INVOKABLE void removeLine(int row);
    Q_INVOKABLE void clearCart();
    // value: importe ("50") o porcentaje ("10") segun percent. Vacio o "0" quita el descuento.
    Q_INVOKABLE QVariantMap setDiscount(const QString &value, bool percent);

    // form: cash, cashCurrency, transfer, reference (textos).
    // Salida: { valid, error, paidText, changeText, remainingText }
    Q_INVOKABLE QVariantMap previewPayment(const QVariantMap &form) const;
    // form: igual que previewPayment + allowNegative (bool).
    // Salida: { ok, error, stockIssues: [..], number, changeText }
    Q_INVOKABLE QVariantMap checkout(const QVariantMap &form);

public slots:
    void refreshSession();
    void refreshCurrencies();

signals:
    void searchTextChanged();
    void totalsChanged();
    void sessionChanged();
    void currenciesChanged();
    void saleCompleted(int number);

private:
    qint64 subtotal() const;
    qint64 discount() const;
    qint64 total() const;
    bool buildPayments(const QVariantMap &form, QVector<Payment> *out, QString *error) const;
    QVariantMap addLine(const Product &p, qint64 qtyMilli);

    QSqlDatabase m_db;
    const Session *m_session;
    ProductRepository m_productsRepo;
    mutable CurrencyRepository m_currencyRepo;
    SaleService m_saleService;
    CashService m_cashService;
    CartModel m_cart;
    ProductListModel m_results;
    QString m_searchText;
    bool m_discountPercent = false;
    qint64 m_discountValue = 0;   // centavos, o centesimas de % si m_discountPercent
    bool m_hasOpenSession = false;
    QVariantList m_currencies;
    QString m_baseCurrency;
};
