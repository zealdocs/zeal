/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: https://go.zealdocs.org/l/contact
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
** along with Zeal. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef ZEAL_REGISTRY_DOCSETREGISTRY_H
#define ZEAL_REGISTRY_DOCSETREGISTRY_H

#include "cancellationtoken.h"
#include "searchresult.h"

#include <QMap>
#include <QObject>

class QAbstractItemModel;
class QThread;

namespace Zeal {
namespace Registry {

class Docset;

class DocsetRegistry final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(DocsetRegistry)
public:
    explicit DocsetRegistry(QObject *parent = nullptr);
    ~DocsetRegistry() override;

    QAbstractItemModel *model() const;

    QString storagePath() const;
    void setStoragePath(const QString &path);

    bool isFuzzySearchEnabled() const;
    void setFuzzySearchEnabled(bool enabled);

    int count() const;
    bool contains(const QString &name) const;
    QStringList names() const;

    void loadDocset(const QString &path);
    void unloadDocset(const QString &name);
    void unloadAllDocsets();

    Docset *docset(const QString &name) const;
    Docset *docset(int index) const;
    Docset *docsetForUrl(const QUrl &url);
    QList<Docset *> docsets() const;

    void search(const QString &query);
    const QList<SearchResult> &queryResults();

signals:
    void docsetLoaded(const QString &name);
    void docsetAboutToBeUnloaded(const QString &name);
    void docsetUnloaded(const QString &name);
    void searchCompleted(const QList<SearchResult> &results);

private slots:
    void _runQuery(const QString &query);

private:
    void addDocsetsFromFolder(const QString &path);

    QAbstractItemModel *m_model = nullptr;

    QString m_storagePath;
    bool m_isFuzzySearchEnabled = false;

    QThread *m_thread = nullptr;
    QMap<QString, Docset *> m_docsets;

    CancellationToken m_cancellationToken;
};

} // namespace Registry
} // namespace Zeal

#endif // ZEAL_REGISTRY_DOCSETREGISTRY_H
