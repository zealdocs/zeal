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

#include "docsetregistry.h"

#include "docset.h"
#include "searchquery.h"
#include "searchresult.h"

#include <QDir>
#include <QThread>

#include <QtConcurrent>

#include <functional>

using namespace Zeal::Registry;

void MergeQueryResults(QList<SearchResult> &finalResult, const QList<SearchResult> &partial)
{
    finalResult << partial;
}

DocsetRegistry::DocsetRegistry(QObject *parent) :
    QObject(parent),
    m_thread(new QThread(this))
{
    // Register for use in signal connections.
    qRegisterMetaType<QList<SearchResult>>("QList<SearchResult>");

    // FIXME: Only search should be performed in a separate thread
    moveToThread(m_thread);
    m_thread->start();
}

DocsetRegistry::~DocsetRegistry()
{
    m_thread->exit();
    m_thread->wait();
    qDeleteAll(m_docsets);
}

QString DocsetRegistry::storagePath() const
{
    return m_storagePath;
}

void DocsetRegistry::setStoragePath(const QString &path)
{
    if (path == m_storagePath) {
        return;
    }

    m_storagePath = path;

    unloadAllDocsets();
    addDocsetsFromFolder(path);
}

bool DocsetRegistry::isFuzzySearchEnabled() const
{
    return m_fuzzySearchEnabled;
}

void DocsetRegistry::setFuzzySearchEnabled(bool enabled)
{
    if (enabled == m_fuzzySearchEnabled) {
        return;
    }

    m_fuzzySearchEnabled = enabled;

    for (Docset *docset : m_docsets) {
        docset->setFuzzySearchEnabled(enabled);
    }
}

int DocsetRegistry::count() const
{
    return m_docsets.count();
}

bool DocsetRegistry::contains(const QString &name) const
{
    return m_docsets.contains(name);
}

QStringList DocsetRegistry::names() const
{
    return m_docsets.keys();
}

void DocsetRegistry::loadDocset(const QString &path)
{
    QFutureWatcher<Docset *> *watcher = new QFutureWatcher<Docset *>();
    connect(watcher, &QFutureWatcher<Docset *>::finished, this, [this, watcher] {
        QScopedPointer<QFutureWatcher<Docset *>, QScopedPointerDeleteLater> guard(watcher);

        Docset *docset = watcher->result();
        // TODO: Emit error
        if (!docset->isValid()) {
            qWarning("Could not load docset from '%s'. Reinstall the docset.",
                     qPrintable(docset->path()));
            delete docset;
            return;
        }

        docset->setFuzzySearchEnabled(m_fuzzySearchEnabled);

        const QString name = docset->name();
        if (m_docsets.contains(name)) {
            unloadDocset(name);
        }

        m_docsets[name] = docset;
        emit docsetLoaded(name);
    });

    watcher->setFuture(QtConcurrent::run([path] {
        return new Docset(path);
    }));
}

void DocsetRegistry::unloadDocset(const QString &name)
{
    emit docsetAboutToBeUnloaded(name);
    delete m_docsets.take(name);
    emit docsetUnloaded(name);
}

void DocsetRegistry::unloadAllDocsets()
{
    for (const QString &name : m_docsets.keys()) {
        unloadDocset(name);
    }
}

Docset *DocsetRegistry::docset(const QString &name) const
{
    return m_docsets[name];
}

Docset *DocsetRegistry::docset(int index) const
{
    if (index < 0 || index >= m_docsets.size())
        return nullptr;
    return (m_docsets.cbegin() + index).value();
}

QList<Docset *> DocsetRegistry::docsets() const
{
    return m_docsets.values();
}

void DocsetRegistry::search(const QString &query)
{
    m_cancellationToken.cancel();

    if (query.isEmpty()) {
        emit searchCompleted({});
        return;
    }

    QMetaObject::invokeMethod(this, "_runQuery", Qt::QueuedConnection, Q_ARG(QString, query));
}

void DocsetRegistry::_runQuery(const QString &query)
{
    m_cancellationToken.reset();

    QList<Docset *> enabledDocsets;

    const SearchQuery searchQuery = SearchQuery::fromString(query);
    if (searchQuery.hasKeywords()) {
        for (Docset *docset : m_docsets) {
            if (searchQuery.hasKeywords(docset->keywords()))
                enabledDocsets << docset;
        }
    } else {
        enabledDocsets = docsets();
    }

    QFuture<QList<SearchResult>> queryResultsFuture
            = QtConcurrent::mappedReduced(enabledDocsets,
                                          std::bind(&Docset::search,
                                                    std::placeholders::_1,
                                                    searchQuery.query(),
                                                    std::ref(m_cancellationToken)),
                                          &MergeQueryResults);
    QList<SearchResult> results = queryResultsFuture.result();

    if (m_cancellationToken.isCanceled())
        return;

    std::sort(results.begin(), results.end());

    if (m_cancellationToken.isCanceled())
        return;

    emit searchCompleted(results);
}

// Recursively finds and adds all docsets in a given directory.
void DocsetRegistry::addDocsetsFromFolder(const QString &path)
{
    const QDir dir(path);
    for (const QFileInfo &subdir : dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs)) {
        if (subdir.suffix() == QLatin1String("docset"))
            loadDocset(subdir.filePath());
        else
            addDocsetsFromFolder(subdir.filePath());
    }
}
