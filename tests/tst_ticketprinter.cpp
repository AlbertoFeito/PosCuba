#include <QtTest>

#include <QFileInfo>
#include <QTemporaryDir>

#include "core/TicketFormatter.h"
#include "printing/TicketPrinter.h"

class TstTicketPrinter : public QObject
{
    Q_OBJECT

private slots:
    void exportPdf_data()
    {
        QTest::addColumn<int>("paper");
        QTest::newRow("58mm") << 58;
        QTest::newRow("80mm") << 80;
    }

    void exportPdf()
    {
        QFETCH(int, paper);
        const int cols = TicketFormatter::columnsForPaper(paper);
        QStringList lines;
        lines << TicketFormatter::center(QStringLiteral("PRUEBA ÁÉÍÓÚ Ñ"), cols)
              << QString(cols, QLatin1Char('-'));
        for (int i = 1; i <= 60; ++i)   // ticket largo: no debe fallar
            lines << TicketFormatter::justify(QStringLiteral("Producto %1").arg(i), QStringLiteral("10.00"), cols);
        lines << TicketFormatter::justify(QStringLiteral("TOTAL CUP:"), QStringLiteral("600.00"), cols);

        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath(QStringLiteral("sub/ticket.pdf"));   // crea subcarpetas
        QString error;
        QVERIFY2(TicketPrinter::exportPdf(lines, paper, path, &error), qPrintable(error));
        QVERIFY(QFileInfo(path).size() > 1000);
        if (!qEnvironmentVariableIsEmpty("POS_KEEP_PDF"))
            QFile::copy(path, QString::fromLocal8Bit(qgetenv("POS_KEEP_PDF")) + QStringLiteral("/ticket_%1.pdf").arg(paper));
    }

    // Un PDF con varias ventas: cada una empieza en hoja nueva; una venta larga ocupa varias hojas.
    void multiTicketPdf()
    {
        auto ticket = [](int n, const QString &tag) {
            QStringList l;
            l << TicketFormatter::center(tag, 32);
            for (int i = 1; i < n; ++i)
                l << TicketFormatter::justify(QStringLiteral("%1 linea %2").arg(tag).arg(i), QStringLiteral("1.00"), 32);
            return l;
        };
        const QVector<QStringList> tickets = {ticket(12, QStringLiteral("CORTO")), ticket(260, QStringLiteral("LARGO")),
                                              ticket(8, QStringLiteral("FINAL"))};
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("tickets_dia.pdf"));
        QString error;
        QVERIFY2(TicketPrinter::exportPdf(tickets, 58, path, &error), qPrintable(error));

        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray pdf = f.readAll();
        // Hojas: 1 (corto) + al menos 2 (largo: 260 lineas no caben en 297 mm) + 1 (final)
        const int pages = pdf.count("/Type /Page\n") + pdf.count("/Type /Page\r") + pdf.count("/Type /Page ")
                          + pdf.count("/Type /Page/") + pdf.count("/Type/Page\n") + pdf.count("/Type/Page/");
        QVERIFY2(pages >= 4, qPrintable(QStringLiteral("hojas: %1").arg(pages)));
        QVERIFY(!QFile::exists(path + QStringLiteral(".tmp")));
        if (!qEnvironmentVariableIsEmpty("POS_KEEP_PDF"))
            QFile::copy(path, QString::fromLocal8Bit(qgetenv("POS_KEEP_PDF")) + QStringLiteral("/tickets_dia.pdf"));
    }

    void unknownPrinterFails()
    {
        QString error;
        QVERIFY(!TicketPrinter::print({QStringLiteral("x")}, QStringLiteral("NoExiste-123"), 58, &error));
        QVERIFY(!error.isEmpty());
    }
};

QTEST_MAIN(TstTicketPrinter)
#include "tst_ticketprinter.moc"
