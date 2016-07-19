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

#include "searchquery.h"
#include "searchresult.h"

#include <QDir>
#include <QThread>

using namespace Zeal;

DocsetRegistry::DocsetRegistry(QObject *parent) :
    QObject(parent),
    m_thread(new QThread(this))
{
    // Register for use in signal connections.
    qRegisterMetaType<QList<Zeal::SearchResult>>("QList<SearchResult>");

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

void DocsetRegistry::init(const QString &path)
{
    for (const QString &name : m_docsets.keys())
        remove(name);

    addDocsetsFromFolder(path);
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

void DocsetRegistry::remove(const QString &name)
{
    emit docsetAboutToBeRemoved(name);
    delete m_docsets.take(name);
    emit docsetRemoved(name);
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

void DocsetRegistry::addDocset(const QString &path)
{
    QMetaObject::invokeMethod(this, "_addDocset", Qt::BlockingQueuedConnection,
                              Q_ARG(QString, path));
}

void DocsetRegistry::_addDocset(const QString &path)
{
    Docset *docset = new Docset(path);

    // TODO: Emit error
    if (!docset->isValid()) {
        qWarning("Could not load docset from '%s'. Please reinstall the docset.", qPrintable(path));
        delete docset;
        return;
    }

    const QString name = docset->name();

    if (m_docsets.contains(name))
        remove(name);

    m_docsets[name] = docset;
    emit docsetAdded(name);
}

void DocsetRegistry::search(const QString &query)
{
    // Only invalidate queries
    if (query.isEmpty())
        return;

    QMetaObject::invokeMethod(this, "_runQuery", Qt::QueuedConnection, Q_ARG(QString, query));
}

void DocsetRegistry::_runQuery(const QString &queryStr)
{
    SearchQuery query = getSearchQuery(queryStr);
    QList<SearchResult> results;

    for (Docset *docset : docsets())
        results << docset->search(query);

    std::sort(results.begin(), results.end());

    emit queryCompleted(results);
}

SearchQuery DocsetRegistry::getSearchQuery(const QString &queryStr) const
{
    return SearchQuery::fromString(queryStr, completions());
}

QStringList DocsetRegistry::completions() const
{
    QStringList completionsList;
    for (const Docset * const docset: docsets()) {
        if (docset->keywords().isEmpty())
            continue;

        completionsList << docset->keywords().first() + QLatin1Char(':');
    }

    return completionsList;
}

// Recursively finds and adds all docsets in a given directory.
void DocsetRegistry::addDocsetsFromFolder(const QString &path)
{
    const QDir dir(path);
    for (const QFileInfo &subdir : dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs)) {
        if (subdir.suffix() == QStringLiteral("docset"))
            addDocset(subdir.absoluteFilePath());
        else
            addDocsetsFromFolder(subdir.absoluteFilePath());
    }
}
