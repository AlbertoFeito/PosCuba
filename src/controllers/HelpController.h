#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

#include "core/HelpCatalog.h"

// Manual de usuario y ayuda contextual, expuesto a QML como "help".
// topic: la seccion que corresponde a lo que el usuario tiene delante (el dialogo abierto
// mas reciente o, si no hay, la pantalla actual). F1 abre el manual en esa seccion.
class HelpController : public QObject
{
    Q_OBJECT
    // [{ id, title }]
    Q_PROPERTY(QVariantList sections READ sections CONSTANT)
    Q_PROPERTY(bool showTips READ showTips WRITE setShowTips NOTIFY showTipsChanged)
    Q_PROPERTY(QString pageTopic READ pageTopic WRITE setPageTopic NOTIFY topicChanged)
    Q_PROPERTY(QString topic READ topic NOTIFY topicChanged)

public:
    explicit HelpController(QSqlDatabase db, QObject *parent = nullptr);

    QVariantList sections() const;
    bool showTips() const { return m_showTips; }
    void setShowTips(bool on);
    QString pageTopic() const { return m_pageTopic; }
    void setPageTopic(const QString &topic);
    QString topic() const;

    Q_INVOKABLE int indexOf(const QString &id) const;
    Q_INVOKABLE QString title(const QString &id) const;
    Q_INVOKABLE QString markdown(const QString &id) const;
    // Seccion convertida a HTML con espaciado de lectura y los colores del tema.
    Q_INVOKABLE QString html(const QString &id, const QString &linkColor, const QString &borderColor,
                             int pixelSize) const;
    // [{ id, title, snippet, matches }]
    Q_INVOKABLE QVariantList search(const QString &query) const;
    // Posicion de "term" en "text" sin importar tildes ni mayusculas (-1 si no esta).
    Q_INVOKABLE int find(const QString &text, const QString &term, int from = 0) const;
    // Manual completo en Documentos\PosCuba\Manual_PosCuba.pdf (y lo abre). Salida: { ok, error, path }
    Q_INVOKABLE QVariantMap exportPdf() const;

    // Los dialogos avisan al abrirse y cerrarse (onAboutToShow / onAboutToHide).
    Q_INVOKABLE void enter(const QString &topic);
    Q_INVOKABLE void leave(const QString &topic);

public slots:
    // Tras restaurar un respaldo: releer la preferencia.
    void reloadSettings();

signals:
    void showTipsChanged();
    void topicChanged();

private:
    QSqlDatabase m_db;
    QVector<HelpSection> m_sections;
    bool m_showTips = true;
    QString m_pageTopic;
    QStringList m_dialogTopics;
};
