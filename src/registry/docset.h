/****************************************************************************
**
** Copyright (C) 2015 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: http://zealdocs.org/contact.html
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef DOCSET_H
#define DOCSET_H

#include "searchresult.h"

#include <QIcon>
#include <QMap>
#include <QMetaObject>
#include <QSqlDatabase>

namespace Zeal {

class Docset
{
public:
    explicit Docset(const QString &path);
    ~Docset();

    bool isValid() const;

    QString name() const;
    QString title() const;
    QStringList keywords() const;

    QString version() const;
    QString revision() const;

    QString path() const;
    QString documentPath() const;
    QIcon icon() const;
    QString indexFilePath() const;

    QMap<QString, int> symbolCounts() const;
    int symbolCount(const QString &symbolType) const;

    const QMap<QString, QString> &symbols(const QString &symbolType) const;

    QList<SearchResult> search(const QString &query) const;
    QList<SearchResult> relatedLinks(const QUrl &url) const;

    /// FIXME: This is an ugly workaround before we have a proper docset sources implementation
    bool hasUpdate = false;

private:
    enum class Type {
        Invalid,
        Dash,
        ZDash
    };

    QSqlDatabase database() const;

    void loadMetadata();
    void countSymbols();
    void loadSymbols(const QString &symbolType) const;
    void loadSymbols(const QString &symbolType, const QString &symbolString) const;
    void createIndex();

    static QString parseSymbolType(const QString &str);

    QString m_sourceId;
    QString m_name;
    QString m_title;
    QStringList m_keywords;
    QString m_version;
    QString m_revision;
    Docset::Type m_type = Type::Invalid;
    QString m_path;
    QIcon m_icon;

    QString m_indexFilePath;

    QMap<QString, QString> m_symbolStrings;
    QMap<QString, int> m_symbolCounts;
    mutable QMap<QString, QMap<QString, QString>> m_symbols;
};

} // namespace Zeal

#endif // DOCSET_H
