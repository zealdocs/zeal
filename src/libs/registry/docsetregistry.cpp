// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include "docsetregistry.h"

#include "docset.h"
#include "listmodel.h"
#include "searchquery.h"
#include "searchresult.h"

#include <core/httpserver.h>

#include <QDir>
#include <QLoggingCategory>
#include <QScopeGuard>
#include <QStack>
#include <QThread>
#include <QtConcurrent>

#include <algorithm>

namespace Zeal::Registry {

namespace {
Q_LOGGING_CATEGORY(log, "zeal.registry.docsetregistry")

void MergeQueryResults(QList<SearchResult> &finalResult, const QList<SearchResult> &partial)
{
    finalResult << partial;
}

// Construct a Docset, logging and returning nullptr on any exception so the
// caller can skip rather than propagate out of QtConcurrent or signal slots.
Docset *constructDocset(const QString &path)
{
    try {
        return new Docset(path);
    } catch (const std::exception &e) {
        qCWarning(log, "Failed to construct docset from '%s': %s.", qPrintable(path), e.what());
    } catch (...) {
        qCWarning(log, "Failed to construct docset from '%s'.", qPrintable(path));
    }
    return nullptr;
}
} // namespace

DocsetRegistry::DocsetRegistry(Core::HttpServer *httpServer, QObject *parent)
    : QObject(parent)
    , m_model(new ListModel(this))
    , m_httpServer(httpServer)
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

    // Unmount before deleting so the still-running HTTP server cannot invoke a
    // content provider that captured a docset being destroyed.
    const auto names = m_docsets.keys();
    for (const QString &name : names) {
        m_httpServer->unmount(name);
    }
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

    QMetaObject::invokeMethod(this, [this, path]() {
        m_isLoadingDocsets.store(true, std::memory_order_relaxed);
        emit docsetLoadingStarted();

        const QScopeGuard loadingFinished([this]() {
            m_isLoadingDocsets.store(false, std::memory_order_relaxed);
            emit docsetLoadingFinished();
        });

        unloadAllDocsets();
        addDocsetsFromFolder(path);
        m_storagePath = path;
    });
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
    return static_cast<int>(m_docsets.count());
}

bool DocsetRegistry::isLoading() const
{
    return m_isLoadingDocsets.load(std::memory_order_relaxed);
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
    if (Docset *docset = constructDocset(path)) {
        registerDocset(docset);
    }
}

void DocsetRegistry::registerDocset(Docset *docset)
{
    // TODO: Emit error
    if (!docset->isValid()) {
        qCWarning(log,
                  "Could not load docset '%s' from '%s'. Reinstall the docset.",
                  qPrintable(docset->name()),
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
    QUrl url;
    if (docset->isArchived()) {
        url = m_httpServer->mount(name, [docset](const QString &path) {
            return docset->readDocument(path);
        });
    } else {
        url = m_httpServer->mount(name, docset->documentPath());
    }
    if (url.isEmpty()) {
        qCWarning(log,
                  "Could not enable docset '%s' from '%s'. Reinstall the docset.",
                  qPrintable(docset->name()),
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
    m_httpServer->unmount(name);
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
    if (index < 0 || index >= m_docsets.size()) {
        return nullptr;
    }

    auto it = m_docsets.cbegin();
    std::advance(it, index);
    return *it;
}

Docset *DocsetRegistry::docsetForUrl(const QUrl &url)
{
    for (Docset *docset : std::as_const(m_docsets)) {
        if (docset->baseUrl().isParentOf(url)) {
            return docset;
        }
    }

    return nullptr;
}

QList<Docset *> DocsetRegistry::docsets() const
{
    return m_docsets.values();
}

void DocsetRegistry::search(const QString &query)
{
    m_cancelSearch.store(true, std::memory_order_relaxed);

    if (query.isEmpty()) {
        emit searchCompleted({});
        return;
    }

    QMetaObject::invokeMethod(this, [this, query]() {
        runQuery(query);
    }, Qt::QueuedConnection);
}

// Recursively finds and adds all docsets in a given directory. Docset
// constructors run in parallel (each owns its own SQLite connection); the
// post-construction registration (HTTP mount, model insertion, signal
// emission) runs serially on the registry thread.
void DocsetRegistry::addDocsetsFromFolder(const QString &path)
{
    const QStringList docsetPaths = collectDocsetPaths(path);

    if (docsetPaths.isEmpty()) {
        return;
    }

    const QList<Docset *> docsets = QtConcurrent::blockingMapped(docsetPaths, &constructDocset);

    for (Docset *docset : docsets) {
        if (docset != nullptr) {
            registerDocset(docset);
        }
    }
}

QStringList DocsetRegistry::collectDocsetPaths(const QString &path)
{
    QStringList result;
    QStack<QString> stack;
    stack.push(path);
    while (!stack.isEmpty()) {
        const QDir dir(stack.pop());
        const auto entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs);
        for (const QFileInfo &entry : entries) {
            if (entry.suffix() == QLatin1String("docset")) {
                result << entry.filePath();
            } else {
                stack.push(entry.filePath());
            }
        }
    }
    return result;
}

void DocsetRegistry::runQuery(const QString &query)
{
    m_cancelSearch.store(false, std::memory_order_relaxed);

    QList<Docset *> enabledDocsets;

    const SearchQuery searchQuery = SearchQuery::fromString(query);
    if (searchQuery.hasKeywords()) {
        for (Docset *docset : std::as_const(m_docsets)) {
            if (searchQuery.hasKeywords(docset->keywords())) {
                enabledDocsets << docset;
            }
        }
    } else {
        enabledDocsets = docsets();
    }

    const QString queryString = searchQuery.query();
    const QFuture<QList<SearchResult>> queryFuture = QtConcurrent::mappedReduced(enabledDocsets,
                                                                                 [this, &queryString](Docset *docset) {
        return docset->search(queryString, m_cancelSearch);
    },
                                                                                 &MergeQueryResults);
    QList<SearchResult> results = queryFuture.result();

    if (m_cancelSearch.load(std::memory_order_relaxed)) {
        return;
    }

    std::ranges::sort(results);

    if (m_cancelSearch.load(std::memory_order_relaxed)) {
        return;
    }

    emit searchCompleted(results);
}

} // namespace Zeal::Registry
