#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QDir>
#include <QUrl>

#include "app/AppController.h"
#include "app/Formatter.h"
#include "controllers/BackupController.h"
#include "controllers/CashController.h"
#include "controllers/CatalogController.h"
#include "controllers/CurrencyController.h"
#include "controllers/HelpController.h"
#include "controllers/InventoryController.h"
#include "controllers/ProductController.h"
#include "controllers/RefundController.h"
#include "controllers/ReportController.h"
#include "controllers/SaleController.h"
#include "controllers/TicketController.h"
#include "controllers/UserController.h"
#include "data/Database.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    // PCs viejas con drivers OpenGL malos: ejecutar con --software
    for (int i = 1; i < argc; ++i) {
        if (qstrcmp(argv[i], "--software") == 0)
            QQuickWindow::setSceneGraphBackend(QStringLiteral("software"));
    }

    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("PosCuba"));
    QCoreApplication::setApplicationName(QStringLiteral("PosCuba"));
    QCoreApplication::setApplicationVersion(QStringLiteral(POS_VERSION));
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/icons/poscuba.png")));

    // Fusion existe en Qt 5.14 y Qt 6 y se ve igual en ambos.
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    AppController controller;
    controller.initialize(QDir(dataDir).filePath(QStringLiteral("pos.db")));

    Formatter formatter;
    ProductController productController(Database::connection(), controller.session());
    InventoryController inventoryController(Database::connection(), controller.session());
    SaleController saleController(Database::connection(), controller.session());
    CashController cashController(Database::connection(), controller.session());
    TicketController ticketController(Database::connection(), controller.session());
    ReportController reportController(Database::connection(), controller.session());
    UserController userController(controller.userService(), controller.session());
    BackupController backupController(Database::connection(), controller.session());
    CurrencyController currencyController(Database::connection(), controller.session());
    HelpController helpController(Database::connection());
    CatalogController catalogController(Database::connection(), controller.session());
    QObject::connect(&catalogController, &CatalogController::catalogChanged, &app, [&]() {
        productController.reloadCategories();
        productController.reload();
        inventoryController.reload();
    });
    RefundController refundController(Database::connection(), controller.session(), controller.userService());
    QObject::connect(&refundController, &RefundController::refunded, &app, [&]() {
        cashController.refresh();
        inventoryController.reload();
        productController.reload();
        reportController.refresh();
    });
    QObject::connect(&currencyController, &CurrencyController::currenciesEdited, &app, [&]() {
        saleController.refreshCurrencies();
        ticketController.reloadSettings();
        cashController.refresh();
    });
    QObject::connect(&userController, &UserController::usersChanged, &controller, &AppController::refreshLoginUsers);
    QObject::connect(&userController, &UserController::usersChanged, &controller, &AppController::refreshSessionUser);
    ticketController.refreshPrinters();
    if (controller.dbReady()) {
        productController.reloadCategories();
        productController.reload();
        inventoryController.reload();
        saleController.refreshCurrencies();
        saleController.refreshSession();
        cashController.refresh();
        ticketController.reloadSettings();
        backupController.autoBackupIfDue();
    }
    QObject::connect(&cashController, &CashController::sessionClosed, &backupController, &BackupController::autoBackupNow);
    // Tras restaurar un respaldo todo se relee de la base (sin reiniciar la app).
    QObject::connect(&backupController, &BackupController::dataRestored, &app, [&]() {
        saleController.clearCart();
        productController.reloadCategories();
        productController.reload();
        inventoryController.reload();
        saleController.refreshCurrencies();
        saleController.refreshSession();
        cashController.refresh();
        ticketController.reloadSettings();
        reportController.refresh();
        currencyController.refresh();
        helpController.reloadSettings();
        controller.reloadAfterRestore();
    });
    QObject::connect(&cashController, &CashController::sessionOpenedOrClosed,
                     &saleController, &SaleController::refreshSession);
    QObject::connect(&saleController, &SaleController::saleCompleted, &cashController, &CashController::refresh);
    QObject::connect(&saleController, &SaleController::saleCompleted,
                     &inventoryController, &InventoryController::reload);
    QObject::connect(&saleController, &SaleController::saleCompleted, &productController, &ProductController::reload);
    QObject::connect(&productController, &ProductController::productsChanged,
                     &inventoryController, &InventoryController::reload);
    QObject::connect(&inventoryController, &InventoryController::stockChanged,
                     &productController, &ProductController::reload);

    QQmlApplicationEngine engine;
    QQmlContext *ctx = engine.rootContext();
    ctx->setContextProperty(QStringLiteral("app"), &controller);
    ctx->setContextProperty(QStringLiteral("fmt"), &formatter);
    ctx->setContextProperty(QStringLiteral("productCtrl"), &productController);
    ctx->setContextProperty(QStringLiteral("inventoryCtrl"), &inventoryController);
    ctx->setContextProperty(QStringLiteral("saleCtrl"), &saleController);
    ctx->setContextProperty(QStringLiteral("cashCtrl"), &cashController);
    ctx->setContextProperty(QStringLiteral("ticketCtrl"), &ticketController);
    ctx->setContextProperty(QStringLiteral("reportCtrl"), &reportController);
    ctx->setContextProperty(QStringLiteral("userCtrl"), &userController);
    ctx->setContextProperty(QStringLiteral("backupCtrl"), &backupController);
    ctx->setContextProperty(QStringLiteral("currencyCtrl"), &currencyController);
    ctx->setContextProperty(QStringLiteral("help"), &helpController);
    ctx->setContextProperty(QStringLiteral("refundCtrl"), &refundController);
    ctx->setContextProperty(QStringLiteral("catalogCtrl"), &catalogController);
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
