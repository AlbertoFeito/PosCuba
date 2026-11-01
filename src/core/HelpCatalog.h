#pragma once

#include <QString>
#include <QVector>

// Manual de usuario: un archivo Markdown por seccion en :/docs/manual/NN-id.md.
// El numero ordena las secciones; "id" es el nombre que usan los enlaces (ayuda:id) y la ayuda contextual.
// La primera linea "# Titulo" es el titulo de la seccion.
struct HelpSection
{
    QString id;
    QString title;
    QString markdown;
};

struct HelpHit
{
    int section = -1;   // indice en sections()
    int matches = 0;
    QString snippet;    // texto plano alrededor de la primera coincidencia
};

class HelpCatalog
{
public:
    static QString resourceDir() { return QStringLiteral(":/docs/manual"); }

    // Lee las secciones de una carpeta (por defecto, la de recursos).
    static QVector<HelpSection> load(const QString &dir = resourceDir());

    // Minusculas y sin tildes: "Configuración" -> "configuracion".
    static QString fold(const QString &text);
    // Markdown -> texto plano aproximado (para buscar y mostrar fragmentos).
    static QString plainText(const QString &markdown);

    // Busca todas las palabras de "query" (sin importar tildes ni mayusculas).
    // Orden: coincidencia en el titulo primero, luego por cantidad de coincidencias.
    static QVector<HelpHit> search(const QVector<HelpSection> &sections, const QString &query);

    // Destinos "ayuda:id" de los enlaces del texto.
    static QStringList links(const QString &markdown);
};
