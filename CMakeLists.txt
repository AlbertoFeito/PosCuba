cmake_minimum_required(VERSION 3.16)

project(PosCuba VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(POS_GUI_ONLY "Compilar como aplicacion GUI sin consola (Windows)" OFF)
option(POS_BUILD_TESTS "Compilar tests unitarios (QtTest)" ON)

# Qt 6 si esta disponible, si no Qt 5 (minimo 5.14).
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
set(POS_QT_COMPONENTS Core Gui Qml Quick QuickControls2 Sql PrintSupport)
if(POS_BUILD_TESTS)
    list(APPEND POS_QT_COMPONENTS Test)
endif()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${POS_QT_COMPONENTS})

if(QT_VERSION_MAJOR EQUAL 5 AND Qt5Core_VERSION VERSION_LESS 5.14)
    message(FATAL_ERROR "Se requiere Qt 5.14 o superior (encontrado ${Qt5Core_VERSION})")
endif()
message(STATUS "Compilando con Qt ${QT_VERSION}")

# ---- Nucleo compartido por la app y los tests ----
# OBJECT (no STATIC) para que los recursos .qrc no se pierdan al enlazar.
add_library(poscore OBJECT
    src/core/Money.cpp
    src/core/Money.h
    src/core/Product.h
    src/core/ProductValidator.cpp
    src/core/ProductValidator.h
    src/core/Currency.h
    src/core/CsvWriter.cpp
    src/core/CsvWriter.h
    src/core/CsvReader.cpp
    src/core/CsvReader.h
    src/core/HelpCatalog.cpp
    src/core/HelpCatalog.h
    src/core/Report.h
    src/core/Sale.h
    src/core/SaleCalculator.cpp
    src/core/SaleCalculator.h
    src/core/Session.h
    src/core/TicketFormatter.cpp
    src/core/TicketFormatter.h
    src/core/User.h
    src/core/StockMovement.h
    src/core/services/BackupService.cpp
    src/core/services/BackupService.h
    src/core/services/CashService.cpp
    src/core/services/CashService.h
    src/core/services/CurrencyService.cpp
    src/core/services/CurrencyService.h
    src/core/services/InventoryService.cpp
    src/core/services/InventoryService.h
    src/core/services/PricingService.cpp
    src/core/services/PricingService.h
    src/core/services/ProductImportService.cpp
    src/core/services/ProductImportService.h
    src/core/services/ProductService.cpp
    src/core/services/ProductService.h
    src/core/services/RefundService.cpp
    src/core/services/RefundService.h
    src/core/services/ReportService.cpp
    src/core/services/ReportService.h
    src/core/services/SaleService.cpp
    src/core/services/SaleService.h
    src/core/services/UserService.cpp
    src/core/services/UserService.h
    src/data/Database.cpp
    src/data/Database.h
    src/data/Migrator.cpp
    src/data/Migrator.h
    src/data/repositories/CashRepository.cpp
    src/data/repositories/CashRepository.h
    src/data/repositories/CategoryRepository.cpp
    src/data/repositories/CategoryRepository.h
    src/data/repositories/CurrencyRepository.cpp
    src/data/repositories/CurrencyRepository.h
    src/data/repositories/ProductRepository.cpp
    src/data/repositories/ProductRepository.h
    src/data/Transaction.h
    src/data/repositories/StockMovementRepository.cpp
    src/data/repositories/StockMovementRepository.h
    src/data/repositories/RefundRepository.cpp
    src/data/repositories/RefundRepository.h
    src/data/repositories/SaleRepository.cpp
    src/data/repositories/SaleRepository.h
    src/data/repositories/SettingsRepository.cpp
    src/data/repositories/SettingsRepository.h
    src/data/repositories/UserRepository.cpp
    src/data/repositories/UserRepository.h
    src/models/CartModel.cpp
    src/models/CartModel.h
    src/models/CategoryListModel.cpp
    src/models/CategoryListModel.h
    src/models/MovementListModel.cpp
    src/models/MovementListModel.h
    src/models/ProductListModel.cpp
    src/models/ProductListModel.h
    sql.qrc
    manual.qrc
)
target_include_directories(poscore PUBLIC src)
target_link_libraries(poscore PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)

# ---- Aplicacion ----
add_executable(pos
    src/main.cpp
    src/app/AppController.cpp
    src/app/AppController.h
    src/app/Formatter.cpp
    src/app/Formatter.h
    src/controllers/BackupController.cpp
    src/controllers/BackupController.h
    src/controllers/CashController.cpp
    src/controllers/CashController.h
    src/controllers/CatalogController.cpp
    src/controllers/CatalogController.h
    src/controllers/CurrencyController.cpp
    src/controllers/CurrencyController.h
    src/controllers/HelpController.cpp
    src/controllers/HelpController.h
    src/controllers/InventoryController.cpp
    src/controllers/InventoryController.h
    src/controllers/ProductController.cpp
    src/controllers/ProductController.h
    src/controllers/RefundController.cpp
    src/controllers/RefundController.h
    src/controllers/ReportController.cpp
    src/controllers/ReportController.h
    src/controllers/SaleController.cpp
    src/controllers/SaleController.h
    src/controllers/TicketController.cpp
    src/controllers/TicketController.h
    src/controllers/UserController.cpp
    src/controllers/UserController.h
    src/printing/TicketPrinter.cpp
    src/printing/TicketPrinter.h
    qml.qrc
)
# El ejecutable se llama PosCuba.exe
set_target_properties(pos PROPERTIES OUTPUT_NAME PosCuba)
if(POS_GUI_ONLY)
    set_target_properties(pos PROPERTIES WIN32_EXECUTABLE ON)
endif()
if(WIN32)
    # Icono e informacion de version del .exe (barras normales: valen para windres y rc.exe)
    set(POS_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/poscuba.ico")
    configure_file(resources/app.rc.in ${CMAKE_CURRENT_BINARY_DIR}/app.rc @ONLY)
    target_sources(pos PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/app.rc)
endif()
target_link_libraries(pos PRIVATE
    poscore
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::QuickControls2
    Qt${QT_VERSION_MAJOR}::PrintSupport
)

foreach(t poscore pos)
    target_compile_definitions(${t} PRIVATE QT_DEPRECATED_WARNINGS POS_VERSION="${PROJECT_VERSION}")
    if(MSVC)
        target_compile_options(${t} PRIVATE /W4 /utf-8)
    else()
        target_compile_options(${t} PRIVATE -Wall -Wextra)
    endif()
endforeach()

# ---- Tests ----
if(POS_BUILD_TESTS)
    enable_testing()
    foreach(name tst_money tst_productservice tst_inventory tst_salecalculator tst_sale tst_ticket tst_reports tst_users tst_backup tst_currency tst_help tst_refund tst_catalog)
        add_executable(${name} tests/${name}.cpp)
        target_link_libraries(${name} PRIVATE poscore Qt${QT_VERSION_MAJOR}::Test)
        if(MSVC)
            target_compile_options(${name} PRIVATE /utf-8)
        endif()
        add_test(NAME ${name} COMMAND ${name})
    endforeach()
    # tst_help revisa que los temas de ayuda usados en la interfaz existan en el manual.
    target_compile_definitions(tst_help PRIVATE POS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

    # Impresion: necesita GUI (fuentes), se ejecuta con la plataforma offscreen.
    add_executable(tst_ticketprinter tests/tst_ticketprinter.cpp src/printing/TicketPrinter.cpp)
    target_link_libraries(tst_ticketprinter PRIVATE poscore Qt${QT_VERSION_MAJOR}::Test
        Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::PrintSupport)
    add_test(NAME tst_ticketprinter COMMAND tst_ticketprinter)
    set_tests_properties(tst_ticketprinter PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
