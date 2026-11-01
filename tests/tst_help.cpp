#include <QtTest>

#include <QDirIterator>
#include <QRegularExpression>
#include <QSet>

#include "core/HelpCatalog.h"

// Manual de usuario: contenido, enlaces, busqueda y temas usados por la interfaz.
class TstHelp : public QObject
{
    Q_OBJECT

private:
    QVector<HelpSection> m_sections;

    QSet<QString> ids() const
    {
        QSet<QString> out;
        for (const HelpSection &s : m_sections)
            out.insert(s.id);
        return out;
    }

private slots:
    void initTestCase()
    {
        m_sections = HelpCatalog::load();
        QVERIFY2(m_sections.size() >= 15, qPrintable(QString::number(m_sections.size())));
    }

    void sectionsAreComplete()
    {
        QSet<QString> seen;
        for (const HelpSection &s : m_sections) {
            QVERIFY2(!seen.contains(s.id), qPrintable(s.id));
            seen.insert(s.id);
            QVERIFY2(s.markdown.startsWith(QStringLiteral("# ")), qPrintable(s.id));
            QVERIFY2(s.title != s.id && !s.title.isEmpty(), qPrintable(s.id));
            QVERIFY2(s.markdown.size() > 500, qPrintable(s.id));
        }
        QCOMPARE(m_sections.first().id, QStringLiteral("inicio"));
        QCOMPARE(m_sections.first().title, QStringLiteral("Primeros pasos"));
    }

    void linksPointToExistingSections()
    {
        const QSet<QString> known = ids();
        int total = 0;
        for (const HelpSection &s : m_sections) {
            for (const QString &target : HelpCatalog::links(s.markdown)) {
                ++total;
                QVERIFY2(known.contains(target), qPrintable(s.id + QStringLiteral(" -> ") + target));
            }
        }
        QVERIFY(total > 20);
    }

    // Todo tema que la interfaz pide (F1, botones «?», dialogos) debe existir.
    void interfaceTopicsExist()
    {
        const QSet<QString> known = ids();
        const QRegularExpression patterns[] = {
            QRegularExpression(QStringLiteral("help\\.(?:enter|leave)\\(\"([a-z-]+)\"\\)")),
            QRegularExpression(QStringLiteral("\\btopic:\\s*\"([a-z-]+)\"")),
            QRegularExpression(QStringLiteral("openHelp\\(\"([a-z-]+)\"\\)")),
            QRegularExpression(QStringLiteral("helpTopic:.*")),
            QRegularExpression(QStringLiteral("pageTopic:[\\s\\S]*?onPageTopicChanged")),
        };
        const QRegularExpression quoted(QStringLiteral("\"([a-z-]+)\""));
        int checked = 0;
        QDirIterator it(QStringLiteral(POS_SOURCE_DIR "/qml"), {QStringLiteral("*.qml")}, QDir::Files,
                        QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            QFile f(path);
            QVERIFY(f.open(QIODevice::ReadOnly));
            const QString qml = QString::fromUtf8(f.readAll());
            for (const QRegularExpression &re : patterns) {
                auto m = re.globalMatch(qml);
                while (m.hasNext()) {
                    const QRegularExpressionMatch match = m.next();
                    QStringList found;
                    if (re.captureCount() >= 1) {
                        found << match.captured(1);
                    } else {
                        auto q = quoted.globalMatch(match.captured(0));
                        while (q.hasNext())
                            found << q.next().captured(1);
                    }
                    for (const QString &t : found) {
                        ++checked;
                        QVERIFY2(known.contains(t), qPrintable(QFileInfo(path).fileName() + QStringLiteral(": ") + t));
                    }
                }
            }
        }
        QVERIFY2(checked >= 25, qPrintable(QString::number(checked)));
    }

    void foldIgnoresAccentsAndCase()
    {
        QCOMPARE(HelpCatalog::fold(QStringLiteral("Configuración ÑANDÚ")), QStringLiteral("configuracion nandu"));
    }

    void plainTextStripsMarkdown()
    {
        const QString plain = HelpCatalog::plainText(
            QStringLiteral("# Título\n\n| A | B |\n|---|---|\n| **x** | `y` |\n\nVer [Caja](ayuda:caja). *Nota* 3*7 y 2*arroz"));
        QVERIFY(!plain.contains(QLatin1Char('#')));
        QVERIFY(!plain.contains(QLatin1String("---")));
        QVERIFY(!plain.contains(QLatin1String("**")));
        QVERIFY(!plain.contains(QLatin1String("ayuda:")));
        QVERIFY(plain.contains(QLatin1String("Ver Caja.")));
        QVERIFY(plain.contains(QLatin1String("x y")));
        QVERIFY(plain.contains(QLatin1String(" Nota 3*7 y 2*arroz")));
    }

    void searchFindsWithoutAccents()
    {
        const QVector<HelpHit> hits = HelpCatalog::search(m_sections, QStringLiteral("CONFIGURACION"));
        QVERIFY(!hits.isEmpty());
        // La seccion con la palabra en el titulo va primero.
        QCOMPARE(m_sections.at(hits.first().section).id, QStringLiteral("configuracion"));
        QVERIFY(HelpCatalog::fold(hits.first().snippet).contains(QLatin1String("configuracion")));
    }

    void searchRequiresAllWords()
    {
        const QVector<HelpHit> both = HelpCatalog::search(m_sections, QStringLiteral("vuelto moneda"));
        QVERIFY(!both.isEmpty());
        for (const HelpHit &h : both) {
            const QString f = HelpCatalog::fold(HelpCatalog::plainText(m_sections.at(h.section).markdown));
            QVERIFY(f.contains(QLatin1String("vuelto")) && f.contains(QLatin1String("moneda")));
        }
        QVERIFY(HelpCatalog::search(m_sections, QStringLiteral("vuelto zzzqqq")).isEmpty());
        QVERIFY(HelpCatalog::search(m_sections, QStringLiteral("   ")).isEmpty());
    }

    void searchSnippetIsShortAndMarked()
    {
        const QVector<HelpHit> hits = HelpCatalog::search(m_sections, QStringLiteral("arqueo"));
        QVERIFY(!hits.isEmpty());
        for (const HelpHit &h : hits) {
            QVERIFY(h.matches > 0);
            QVERIFY(h.snippet.size() <= 170);
            QVERIFY(!h.snippet.contains(QLatin1Char('\n')));
        }
    }
};

QTEST_GUILESS_MAIN(TstHelp)
#include "tst_help.moc"
