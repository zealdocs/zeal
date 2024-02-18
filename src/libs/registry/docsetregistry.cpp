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
#include "listmodel.h"
#include "searchquery.h"
#include "searchresult.h"

#include <core/application.h>
#include <core/httpserver.h>

#include <QDir>
#include <QThread>

#include <QtConcurrent>

#include <functional>
#include <future>

using namespace Zeal::Registry;

void MergeQueryResults(QList<SearchResult> &finalResult, const QList<SearchResult> &partial)
{
    finalResult << partial;
}

DocsetRegistry::DocsetRegistry(QObject *parent)
    : QObject(parent)
    , m_model(new ListModel(this))
    , m_thread(new QThread(this))
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

QAbstractItemModel *DocsetRegistry::model() const
{
    return m_model;
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
    return m_isFuzzySearchEnabled;
}

void DocsetRegistry::setFuzzySearchEnabled(bool enabled)
{
    if (enabled == m_isFuzzySearchEnabled) {
        return;
    }

    m_isFuzzySearchEnabled = enabled;

    for (Docset *docset : std::as_const(m_docsets)) {
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
    std::future<Docset *> f = std::async(std::launch::async, [path](){
        return new Docset(path);
    });

    f.wait();
    Docset *docset = f.get();
    // TODO: Emit error
    if (!docset->isValid()) {
        qWarning("Could not load docset from '%s'. Reinstall the docset.",
                 qPrintable(docset->path()));
        delete docset;
        return;
    }

    docset->setFuzzySearchEnabled(m_isFuzzySearchEnabled);

    const QString name = docset->name();
    if (m_docsets.contains(name)) {
        unloadDocset(name);
    }

    // Setup HTTP mount.
    QUrl url = Core::Application::instance()->httpServer()->mount(name, docset->documentPath());
    if (url.isEmpty()) {
        qWarning("Could not enable docset from '%s'. Reinstall the docset.",
                 qPrintable(docset->path()));
        delete docset;
        return;
    }

    docset->setBaseUrl(url);

    m_docsets[name] = docset;

    emit docsetLoaded(name);
}

void DocsetRegistry::unloadDocset(const QString &name)
{
    emit docsetAboutToBeUnloaded(name);
    Core::Application::instance()->httpServer()->unmount(name);
    delete m_docsets.take(name);
    emit docsetUnloaded(name);
}

void DocsetRegistry::unloadAllDocsets()
{
    const auto keys = m_docsets.keys();
    for (const QString &name : keys) {
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

    auto it = m_docsets.cbegin();
    std::advance(it, index);
    return *it;
}

Docset *DocsetRegistry::docsetForUrl(const QUrl &url)
{
    for (Docset *docset : std::as_const(m_docsets)) {
        if (docset->baseUrl().isParentOf(url))
            return docset;
    }

    return nullptr;
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
        for (Docset *docset : std::as_const(m_docsets)) {
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
    const auto subDirectories = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs);
    for (const QFileInfo &subdir : subDirectories) {
        if (subdir.suffix() == QLatin1String("docset"))
            loadDocset(subdir.filePath());
        else
            addDocsetsFromFolder(subdir.filePath());
    }
}
