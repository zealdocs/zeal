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

#include "listmodel.h"

#include "docset.h"
#include "docsetregistry.h"

using namespace Zeal;

ListModel::ListModel(DocsetRegistry *docsetRegistry, QObject *parent) :
    QAbstractItemModel(parent),
    m_docsetRegistry(docsetRegistry)
{
    connect(m_docsetRegistry, &DocsetRegistry::docsetAdded, this, &ListModel::addDocset);
    connect(m_docsetRegistry, &DocsetRegistry::docsetAboutToBeRemoved, this, &ListModel::removeDocset);

    for (const QString &name : m_docsetRegistry->names())
        addDocset(name);
}

ListModel::~ListModel()
{
    for (DocsetItem *item : m_docsetItems) {
        qDeleteAll(item->groups);
        delete item;
    }
}

QVariant ListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch (role) {
    case Qt::DecorationRole:
        switch (indexLevel(index)) {
        case Level::DocsetLevel:
            return m_docsetRegistry->docset(index.row())->icon();
        case Level::GroupLevel: {
            DocsetItem *docsetItem = reinterpret_cast<DocsetItem *>(index.internalPointer());
            const QString symbolType = docsetItem->groups.at(index.row())->symbolType;
            return QIcon(QString("typeIcon:%1.png").arg(symbolType));
        }
        case Level::SymbolLevel: {
            GroupItem *groupItem = reinterpret_cast<GroupItem *>(index.internalPointer());
            return QIcon(QString("typeIcon:%1.png").arg(groupItem->symbolType));
        }
        default:
            return QVariant();
        }
    case Qt::DisplayRole:
        switch (indexLevel(index)) {
        case Level::DocsetLevel:
            if (!index.column())
                return m_docsetRegistry->docset(index.row())->title();
            else
                return m_docsetRegistry->docset(index.row())->indexFilePath();
        case Level::GroupLevel: {
            DocsetItem *docsetItem = reinterpret_cast<DocsetItem *>(index.internalPointer());
            const QString symbolType = docsetItem->groups.at(index.row())->symbolType;
            return QString(QLatin1String("%1 (%2)")).arg(pluralize(symbolType),
                                                         QString::number(docsetItem->docset->symbolCount(symbolType)));
        }
        case Level::SymbolLevel: {
            GroupItem *groupItem = reinterpret_cast<GroupItem *>(index.internalPointer());
            auto it = groupItem->docsetItem->docset->symbols(groupItem->symbolType).cbegin() + index.row();
            if (!index.column())
                return it.key();
            else
                return it.value();
        }
        default:
            return QVariant();
        }
    case DocsetNameRole:
        if (index.parent().isValid())
            return QVariant();
        return m_docsetRegistry->docset(index.row())->name();
    case UpdateAvailableRole:
        if (index.parent().isValid())
            return QVariant();
        return m_docsetRegistry->docset(index.row())->hasUpdate;
    default:
        return QVariant();
    }
}

QModelIndex ListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    switch (indexLevel(parent)) {
    case Level::RootLevel:
        return createIndex(row, column);
    case Level::DocsetLevel: {
        auto it = m_docsetItems.begin() + parent.row();
        return createIndex(row, column, reinterpret_cast<void *>(it.value()));
    }
    case Level::GroupLevel: {
        DocsetItem *docsetItem = reinterpret_cast<DocsetItem *>(parent.internalPointer());
        return createIndex(row, column, docsetItem->groups.at(parent.row()));
    }
    default:
        return QModelIndex();
    }
}

QModelIndex ListModel::parent(const QModelIndex &child) const
{
    switch (indexLevel(child)) {
    case Level::GroupLevel: {
        DocsetItem *item = reinterpret_cast<DocsetItem *>(child.internalPointer());
        return createIndex(m_docsetItems.keys().indexOf(item->docset->name()), 0);
    }
    case SymbolLevel: {
        GroupItem *item = reinterpret_cast<GroupItem *>(child.internalPointer());
        return createIndex(item->docsetItem->groups.indexOf(item), 0, item->docsetItem);
    }
    default:
        return QModelIndex();
    }
}

int ListModel::columnCount(const QModelIndex &parent) const
{
    if (indexLevel(parent) == Level::DocsetLevel)
        return 1;
    else
        return 2;
}

int ListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;

    switch (indexLevel(parent)) {
    case Level::RootLevel:
        return m_docsetRegistry->count();
    case Level::DocsetLevel:
        return m_docsetRegistry->docset(parent.row())->symbolCounts().count();
    case Level::GroupLevel: {
        DocsetItem *docsetItem = reinterpret_cast<DocsetItem *>(parent.internalPointer());
        return docsetItem->docset->symbolCount(docsetItem->groups.at(parent.row())->symbolType);
    }
    default:
        return 0;
    }
}

void ListModel::addDocset(const QString &name)
{
    const int index = m_docsetRegistry->names().indexOf(name);
    beginInsertRows(QModelIndex(), index, index);

    DocsetItem *docsetItem = new DocsetItem();
    docsetItem->docset = m_docsetRegistry->docset(name);

    for (const QString &symbolType : docsetItem->docset->symbolCounts().keys()) {
        GroupItem *groupItem = new GroupItem();
        groupItem->docsetItem = docsetItem;
        groupItem->symbolType = symbolType;
        docsetItem->groups.append(groupItem);
    }

    m_docsetItems.insert(name, docsetItem);

    endInsertRows();
}

void ListModel::removeDocset(const QString &name)
{
    const int index = m_docsetItems.keys().indexOf(name);
    /// TODO: Investigate why this can happen (see #420)
    if (index == -1)
        return;

    beginRemoveRows(QModelIndex(), index, index);

    DocsetItem *docsetItem = m_docsetItems.take(name);
    qDeleteAll(docsetItem->groups);
    delete docsetItem;

    endRemoveRows();
}

QString ListModel::pluralize(const QString &s)
{
    if (s.endsWith(QLatin1String("y")))
        return s.left(s.length() - 1) + QLatin1String("ies");
    else
        return s + (s.endsWith('s') ? QLatin1String("es") : QLatin1String("s"));
}

ListModel::Level ListModel::indexLevel(const QModelIndex &index)
{
    if (!index.isValid())
        return Level::RootLevel;
    else if (!index.internalPointer())
        return Level::DocsetLevel;
    else if (*reinterpret_cast<Level *>(index.internalPointer()) == Level::DocsetLevel)
        return Level::GroupLevel;
    else
        return Level::SymbolLevel;
}
