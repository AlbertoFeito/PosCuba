#include "CsvWriter.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>

CsvWriter::CsvWriter(Format format)
    : m_format(format)
{
}

QChar CsvWriter::separator() const
{
    return m_format == Spanish ? QLatin1Char(';') : QLatin1Char(',');
}

QString CsvWriter::number(const QString &formatted) const
{
    if (m_format == Standard)
        return formatted;
    QString s = formatted;
    s.replace(QLatin1Char('.'), QLatin1Char(','));
    return s;
}

QString CsvWriter::escape(const QString &field) const
{
    const bool quote = field.contains(separator()) || field.contains(QLatin1Char('"'))
                       || field.contains(QLatin1Char('\n')) || field.contains(QLatin1Char('\r'))
                       || field.startsWith(QLatin1Char(' ')) || field.endsWith(QLatin1Char(' '));
    if (!quote)
        return field;
    QString f = field;
    f.replace(QLatin1Char('"'), QStringLiteral("\"\""));
    return QLatin1Char('"') + f + QLatin1Char('"');
}

QString CsvWriter::line(const QStringList &fields) const
{
    QStringList escaped;
    escaped.reserve(fields.size());
    for (const QString &f : fields)
        escaped << escape(f);
    return escaped.join(separator());
}

QString CsvWriter::build(const QStringList &header, const QVector<QStringList> &rows) const
{
    QString out = line(header) + QStringLiteral("\r\n");
    for (const QStringList &r : rows)
        out += line(r) + QStringLiteral("\r\n");
    return out;
}

bool CsvWriter::write(const QString &path, const QStringList &header, const QVector<QStringList> &rows,
                      QString *error) const
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error)
            *error = QCoreApplication::translate("CsvWriter", "No se pudo crear %1: %2")
                         .arg(QDir::toNativeSeparators(path), file.errorString());
        return false;
    }
    file.write("\xEF\xBB\xBF"); // BOM UTF-8
    file.write(build(header, rows).toUtf8());
    if (!file.commit()) {
        if (error)
            *error = QCoreApplication::translate("CsvWriter", "No se pudo guardar %1: %2")
                         .arg(QDir::toNativeSeparators(path), file.errorString());
        return false;
    }
    return true;
}
