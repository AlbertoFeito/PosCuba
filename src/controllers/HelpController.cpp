#include "controllers/HelpController.h"

#include <QCoreApplication>
#include <QColor>
#include <QDesktopServices>
#include <QFont>
#include <QGuiApplication>
#include <QDir>
#include <QFile>
#include <QPageLayout>
#include <QPageSize>
#include <QPrinter>
#include <QStandardPaths>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextFrame>
#include <QTextTable>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QUrl>

#include "data/repositories/SettingsRepository.h"

namespace {
const char *const TipsKey = "show_tips";
}

HelpController::HelpController(QSqlDatabase db, QObject *parent)
    : QObject(parent), m_db(db), m_sections(HelpCatalog::load())
{
    reloadSettings();
}

void HelpController::reloadSettings()
{
    bool on = true;
    if (m_db.isOpen()) {
        SettingsRepository settings(m_db);
        on = settings.value(QString::fromLatin1(TipsKey), QStringLiteral("1")) != QLatin1String("0");
    }
    if (on != m_showTips) {
        m_showTips = on;
        emit showTipsChanged();
    }
}

void HelpController::setShowTips(bool on)
{
    if (on == m_showTips)
        return;
    m_showTips = on;
    if (m_db.isOpen()) {
        SettingsRepository settings(m_db);
        settings.setValue(QString::fromLatin1(TipsKey), on ? QStringLiteral("1") : QStringLiteral("0"));
    }
    emit showTipsChanged();
}

QVariantList HelpController::sections() const
{
    QVariantList out;
    for (const HelpSection &s : m_sections)
        out.append(QVariantMap{{QStringLiteral("id"), s.id}, {QStringLiteral("title"), s.title}});
    return out;
}

void HelpController::setPageTopic(const QString &topic)
{
    if (topic == m_pageTopic)
        return;
    m_pageTopic = topic;
    emit topicChanged();
}

QString HelpController::topic() const
{
    for (int i = m_dialogTopics.size() - 1; i >= 0; --i) {
        if (indexOf(m_dialogTopics.at(i)) >= 0)
            return m_dialogTopics.at(i);
    }
    return m_pageTopic;
}

void HelpController::enter(const QString &topic)
{
    m_dialogTopics.append(topic);
    emit topicChanged();
}

void HelpController::leave(const QString &topic)
{
    const int i = m_dialogTopics.lastIndexOf(topic);
    if (i < 0)
        return;
    m_dialogTopics.removeAt(i);
    emit topicChanged();
}

int HelpController::indexOf(const QString &id) const
{
    for (int i = 0; i < m_sections.size(); ++i) {
        if (m_sections.at(i).id == id)
            return i;
    }
    return -1;
}

QString HelpController::title(const QString &id) const
{
    const int i = indexOf(id);
    return i >= 0 ? m_sections.at(i).title : QString();
}

QString HelpController::markdown(const QString &id) const
{
    const int i = indexOf(id);
    return i >= 0 ? m_sections.at(i).markdown : QString();
}

QString HelpController::html(const QString &id, const QString &linkColor, const QString &borderColor,
                             int pixelSize) const
{
    const int i = indexOf(id);
    if (i < 0)
        return QString();
    QTextDocument doc;
    // toHtml() escribe el tamano de letra del documento: debe ser el mismo de la interfaz.
    QFont font = QGuiApplication::font();
    font.setPixelSize(qMax(8, pixelSize));
    doc.setDefaultFont(font);
    doc.setMarkdown(m_sections.at(i).markdown);

    // Espacio entre parrafos y antes de los titulos: el Markdown de Qt los deja pegados.
    QTextCursor cursor(&doc);
    for (QTextBlock b = doc.begin(); b.isValid(); b = b.next()) {
        cursor.setPosition(b.position());
        QTextBlockFormat f = b.blockFormat();
        const int level = f.headingLevel();
        if (level == 1) {
            f.setBottomMargin(10);
        } else if (level > 1) {
            f.setTopMargin(18);
            f.setBottomMargin(6);
        } else if (b.textList()) {
            // Ultimo elemento de la lista: separarla del parrafo siguiente.
            f.setBottomMargin(b.next().isValid() && b.next().textList() == b.textList() ? 4 : 10);
        } else if (!cursor.currentTable()) {
            f.setBottomMargin(10);
        }
        f.setLineHeight(115, QTextBlockFormat::ProportionalHeight);
        cursor.setBlockFormat(f);

        // Enlaces con el color del tema
        for (auto it = b.begin(); !it.atEnd(); ++it) {
            const QTextFragment frag = it.fragment();
            if (!frag.isValid() || !frag.charFormat().isAnchor())
                continue;
            QTextCharFormat cf = frag.charFormat();
            cf.setForeground(QColor(linkColor));
            cf.setFontUnderline(true);
            QTextCursor c(&doc);
            c.setPosition(frag.position());
            c.setPosition(frag.position() + frag.length(), QTextCursor::KeepAnchor);
            c.setCharFormat(cf);
        }
    }

    // Tablas: bordes finos y relleno en las celdas
    const QList<QTextFrame *> frames = doc.rootFrame()->childFrames();
    for (QTextFrame *frame : frames) {
        if (QTextTable *table = qobject_cast<QTextTable *>(frame)) {
            QTextTableFormat tf = table->format();
            tf.setBorder(1);
            tf.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
            tf.setBorderBrush(QColor(borderColor));
            tf.setCellSpacing(0);
            tf.setCellPadding(6);
            tf.setTopMargin(4);
            tf.setBottomMargin(12);
            table->setFormat(tf);
        }
    }
    return doc.toHtml();
}

QVariantList HelpController::search(const QString &query) const
{
    QVariantList out;
    const QVector<HelpHit> hits = HelpCatalog::search(m_sections, query);
    for (const HelpHit &h : hits) {
        const HelpSection &s = m_sections.at(h.section);
        out.append(QVariantMap{{QStringLiteral("id"), s.id},
                               {QStringLiteral("title"), s.title},
                               {QStringLiteral("snippet"), h.snippet},
                               {QStringLiteral("matches"), h.matches}});
    }
    return out;
}

int HelpController::find(const QString &text, const QString &term, int from) const
{
    const QString needle = HelpCatalog::fold(term.trimmed());
    if (needle.isEmpty())
        return -1;
    // Plegado caracter a caracter para conservar las posiciones del texto original.
    for (int i = qMax(0, from); i < text.size(); ++i) {
        int j = i;
        int k = 0;
        while (j < text.size() && k < needle.size()) {
            const QString c = HelpCatalog::fold(QString(text.at(j)));
            if (c.isEmpty()) {          // marca suelta: se ignora
                ++j;
                continue;
            }
            if (needle.mid(k, c.size()) != c)
                break;
            k += c.size();
            ++j;
        }
        if (k >= needle.size())
            return i;
    }
    return -1;
}

QVariantMap HelpController::exportPdf() const
{
    const QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString folder = QDir(docs).filePath(QStringLiteral("PosCuba"));
    const QString path = QDir(folder).filePath(QStringLiteral("Manual_PosCuba.pdf"));
    auto result = [](bool ok, const QString &error, const QString &p) {
        return QVariantMap{{QStringLiteral("ok"), ok}, {QStringLiteral("error"), error},
                           {QStringLiteral("path"), QDir::toNativeSeparators(p)}};
    };
    if (!QDir().mkpath(folder))
        return result(false, tr("No se pudo crear la carpeta %1.").arg(QDir::toNativeSeparators(folder)), path);

    QTextDocument doc;
    QFont font = doc.defaultFont();
    font.setPointSize(10);
    doc.setDefaultFont(font);
    QTextCursor cursor(&doc);

    // Portada con el indice
    QString cover = tr("# Manual de usuario de PosCuba\n\nVersión %1\n\n## Contenido\n\n")
                        .arg(QCoreApplication::applicationVersion());
    for (int i = 0; i < m_sections.size(); ++i)
        cover += QStringLiteral("%1. %2\n").arg(i + 1).arg(m_sections.at(i).title);
    QTextDocument part;
    part.setMarkdown(cover);
    cursor.insertFragment(QTextDocumentFragment(&part));

    for (const HelpSection &s : m_sections) {
        QTextBlockFormat pageBreak;
        pageBreak.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
        cursor.insertBlock(pageBreak);
        part.setMarkdown(s.markdown);
        cursor.insertFragment(QTextDocumentFragment(&part));
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageLayout(QPageLayout(QPageSize(QPageSize::A4), QPageLayout::Portrait, QMarginsF(15, 15, 15, 15),
                                      QPageLayout::Millimeter));
    printer.setDocName(tr("Manual de PosCuba"));
    const QString tmp = path + QStringLiteral(".tmp");
    QFile::remove(tmp);
    printer.setOutputFileName(tmp);
    doc.print(&printer);

    if (!QFile::exists(tmp) || QFile(tmp).size() == 0)
        return result(false, tr("No se pudo generar el PDF."), path);
    if (QFile::exists(path) && !QFile::remove(path)) {
        QFile::remove(tmp);
        return result(false, tr("No se pudo reemplazar %1. ¿Está abierto en otro programa?").arg(QDir::toNativeSeparators(path)), path);
    }
    if (!QFile::rename(tmp, path))
        return result(false, tr("No se pudo guardar %1.").arg(QDir::toNativeSeparators(path)), path);
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    return result(true, QString(), path);
}
