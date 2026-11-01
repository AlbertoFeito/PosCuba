#include "core/HelpCatalog.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

#include <algorithm>

namespace {

// "03-venta.md" -> "venta"
QString idFromFile(const QString &baseName)
{
    const int dash = baseName.indexOf(QLatin1Char('-'));
    return dash >= 0 ? baseName.mid(dash + 1) : baseName;
}

QString readUtf8(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromUtf8(f.readAll());
}

// Texto plegado (sin tildes, minusculas) y, para cada caracter del plegado, su posicion en el original.
QString foldWithMap(const QString &text, QVector<int> *map)
{
    QString out;
    out.reserve(text.size());
    if (map) {
        map->clear();
        map->reserve(text.size());
    }
    for (int i = 0; i < text.size(); ++i) {
        const QString decomposed = QString(text.at(i)).normalized(QString::NormalizationForm_D);
        for (const QChar c : decomposed) {
            if (c.category() == QChar::Mark_NonSpacing)
                continue;
            out.append(c.toLower());
            if (map)
                map->append(i);
        }
    }
    return out;
}

} // namespace

QVector<HelpSection> HelpCatalog::load(const QString &dir)
{
    QVector<HelpSection> out;
    const QFileInfoList files = QDir(dir).entryInfoList({QStringLiteral("*.md")}, QDir::Files, QDir::Name);
    for (const QFileInfo &fi : files) {
        HelpSection s;
        s.id = idFromFile(fi.completeBaseName());
        s.markdown = readUtf8(fi.filePath());
        const int nl = s.markdown.indexOf(QLatin1Char('\n'));
        const QString first = (nl >= 0 ? s.markdown.left(nl) : s.markdown).trimmed();
        s.title = first.startsWith(QLatin1String("# ")) ? first.mid(2).trimmed() : s.id;
        out.append(s);
    }
    return out;
}

QString HelpCatalog::fold(const QString &text)
{
    return foldWithMap(text, nullptr);
}

QString HelpCatalog::plainText(const QString &markdown)
{
    static const QRegularExpression link(QStringLiteral("\\[([^\\]]*)\\]\\([^)]*\\)"));
    static const QRegularExpression tableRule(QStringLiteral("^\\s*\\|?\\s*:?-{3,}"));
    static const QRegularExpression heading(QStringLiteral("^#{1,6}\\s+"));
    static const QRegularExpression quote(QStringLiteral("^>\\s?"));
    // *cursiva* (pero no el asterisco de "3*codigo")
    static const QRegularExpression italic(QStringLiteral("(?<![\\w*])\\*([^*\\n]+)\\*(?![\\w*])"));

    QStringList lines;
    const QStringList src = markdown.split(QLatin1Char('\n'));
    for (QString line : src) {
        if (tableRule.match(line).hasMatch())
            continue;
        line.replace(link, QStringLiteral("\\1"));
        line.remove(heading);
        line.remove(quote);
        line.remove(QStringLiteral("**"));
        line.replace(italic, QStringLiteral("\\1"));
        line.remove(QLatin1Char('`'));
        line.replace(QLatin1Char('|'), QLatin1Char(' '));
        lines.append(line.simplified());
    }
    return lines.join(QLatin1Char('\n')).trimmed();
}

QVector<HelpHit> HelpCatalog::search(const QVector<HelpSection> &sections, const QString &query)
{
    QVector<HelpHit> hits;
    const QStringList words = fold(query).split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (words.isEmpty())
        return hits;

    QVector<bool> inTitle;
    for (int i = 0; i < sections.size(); ++i) {
        const QString plain = plainText(sections.at(i).markdown);
        // Una sola linea para que el fragmento no salga cortado por saltos de linea.
        const QString flat = QString(plain).replace(QLatin1Char('\n'), QLatin1Char(' '));
        QVector<int> map;
        const QString folded = foldWithMap(flat, &map);
        const QString foldedTitle = fold(sections.at(i).title);

        int total = 0;
        bool all = true;
        bool titleHit = false;
        for (const QString &w : words) {
            const int n = folded.count(w);
            if (n == 0) {
                all = false;
                break;
            }
            total += n;
            titleHit = titleHit || foldedTitle.contains(w);
        }
        if (!all)
            continue;

        HelpHit h;
        h.section = i;
        h.matches = total;
        const int at = folded.indexOf(words.first());
        const int origAt = at >= 0 && at < map.size() ? map.at(at) : 0;
        int from = qMax(0, origAt - 50);
        int to = qMin(flat.size(), origAt + 110);
        // Cortar en limites de palabra
        if (from > 0) {
            const int sp = flat.indexOf(QLatin1Char(' '), from);
            if (sp >= 0 && sp < origAt)
                from = sp + 1;
        }
        if (to < flat.size()) {
            const int sp = flat.lastIndexOf(QLatin1Char(' '), to);
            if (sp > origAt)
                to = sp;
        }
        h.snippet = (from > 0 ? QStringLiteral("…") : QString()) + flat.mid(from, to - from).trimmed()
                    + (to < flat.size() ? QStringLiteral("…") : QString());
        hits.append(h);
        inTitle.append(titleHit);
    }

    QVector<int> order(hits.size());
    for (int i = 0; i < order.size(); ++i)
        order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        if (inTitle.at(a) != inTitle.at(b))
            return inTitle.at(a);
        return hits.at(a).matches > hits.at(b).matches;
    });
    QVector<HelpHit> sorted;
    for (int i : order)
        sorted.append(hits.at(i));
    return sorted;
}

QStringList HelpCatalog::links(const QString &markdown)
{
    static const QRegularExpression re(QStringLiteral("\\]\\(ayuda:([A-Za-z0-9_-]+)\\)"));
    QStringList out;
    auto it = re.globalMatch(markdown);
    while (it.hasNext())
        out.append(it.next().captured(1));
    return out;
}
