#include "CsvReader.h"

namespace {

// Windows-1252 0x80..0x9F (el resto coincide con Latin-1)
const ushort kCp1252[32] = {0x20AC, 0xFFFD, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
                            0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0xFFFD, 0x017D, 0xFFFD,
                            0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
                            0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFD, 0x017E, 0x0178};

bool validUtf8(const QByteArray &b)
{
    int i = 0;
    const int n = b.size();
    while (i < n) {
        const uchar c = uchar(b.at(i));
        int extra = 0;
        if (c < 0x80)
            extra = 0;
        else if ((c & 0xE0) == 0xC0 && c >= 0xC2)
            extra = 1;
        else if ((c & 0xF0) == 0xE0)
            extra = 2;
        else if ((c & 0xF8) == 0xF0 && c <= 0xF4)
            extra = 3;
        else
            return false;
        for (int k = 1; k <= extra; ++k) {
            if (i + k >= n || (uchar(b.at(i + k)) & 0xC0) != 0x80)
                return false;
        }
        i += extra + 1;
    }
    return true;
}

} // namespace

QString CsvReader::decode(const QByteArray &bytes)
{
    QByteArray b = bytes;
    if (b.startsWith("\xEF\xBB\xBF"))
        b = b.mid(3);
    if (validUtf8(b))
        return QString::fromUtf8(b);
    QString out;
    out.reserve(b.size());
    for (const char ch : b) {
        const uchar c = uchar(ch);
        out.append(c >= 0x80 && c < 0xA0 ? QChar(kCp1252[c - 0x80]) : QChar(c));
    }
    return out;
}

QChar CsvReader::detectSeparator(const QString &text)
{
    int semi = 0, comma = 0, tab = 0;
    bool quoted = false;
    for (const QChar c : text) {
        if (c == QLatin1Char('"'))
            quoted = !quoted;
        else if (!quoted && (c == QLatin1Char('\n') || c == QLatin1Char('\r')))
            break;
        else if (!quoted && c == QLatin1Char(';'))
            ++semi;
        else if (!quoted && c == QLatin1Char(','))
            ++comma;
        else if (!quoted && c == QLatin1Char('\t'))
            ++tab;
    }
    if (tab > semi && tab > comma)
        return QLatin1Char('\t');
    return semi >= comma && semi > 0 ? QLatin1Char(';') : (comma > 0 ? QLatin1Char(',') : QLatin1Char(';'));
}

QVector<QStringList> CsvReader::parse(const QString &text, QChar sep)
{
    QVector<QStringList> rows;
    QStringList row;
    QString field;
    bool quoted = false;
    bool rowHasData = false;
    const int n = text.size();
    for (int i = 0; i < n; ++i) {
        const QChar c = text.at(i);
        if (quoted) {
            if (c == QLatin1Char('"')) {
                if (i + 1 < n && text.at(i + 1) == QLatin1Char('"')) {
                    field.append(QLatin1Char('"'));
                    ++i;
                } else {
                    quoted = false;
                }
            } else {
                field.append(c);
            }
            continue;
        }
        if (c == QLatin1Char('"')) {
            quoted = true;
            rowHasData = true;
        } else if (c == sep) {
            row.append(field);
            field.clear();
            rowHasData = true;
        } else if (c == QLatin1Char('\r') || c == QLatin1Char('\n')) {
            if (c == QLatin1Char('\r') && i + 1 < n && text.at(i + 1) == QLatin1Char('\n'))
                ++i;
            if (rowHasData || !field.isEmpty()) {
                row.append(field);
                rows.append(row);
            }
            row.clear();
            field.clear();
            rowHasData = false;
        } else {
            field.append(c);
            rowHasData = true;
        }
    }
    if (rowHasData || !field.isEmpty()) {
        row.append(field);
        rows.append(row);
    }
    return rows;
}
