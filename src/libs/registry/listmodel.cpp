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

#include "listmodel.h"

#include "docset.h"
#include "docsetregistry.h"
#include "itemdatarole.h"

#include <iterator>

using namespace Zeal::Registry;

ListModel::ListModel(DocsetRegistry *docsetRegistry)
    : QAbstractItemModel(docsetRegistry)
    , m_docsetRegistry(docsetRegistry)
{
    connect(m_docsetRegistry, &DocsetRegistry::docsetLoaded, this, &ListModel::addDocset);
    connect(m_docsetRegistry, &DocsetRegistry::docsetAboutToBeUnloaded, this, &ListModel::removeDocset);
}

ListModel::~ListModel()
{
    for (auto &kv : m_docsetItems) {
        qDeleteAll(kv.second->groups);
        delete kv.second;
    }
}

QVariant ListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QAbstractItemModel::headerData(section, orientation, role);
    }

    switch (section) {
    case SectionIndex::Name:
        return tr("Name");
    case SectionIndex::SearchPrefix:
        return tr("Search prefix");
    default:
        return QLatin1String();
    }
}

QVariant ListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    switch (role) {
    case Qt::DecorationRole:
        if (index.column() != SectionIndex::Name) {
            return QVariant();
        }

        switch (indexLevel(index)) {
        case IndexLevel::Docset:
            return itemInRow(index.row())->docset->icon();
        case IndexLevel::Group: {
            auto docsetItem = static_cast<DocsetItem *>(index.internalPointer());
            const QString symbolType = docsetItem->groups.at(index.row())->symbolType;
            return docsetItem->docset->symbolTypeIcon(symbolType);
        }
        case IndexLevel::Symbol: {
            auto groupItem = static_cast<GroupItem *>(index.internalPointer());
            return groupItem->docsetItem->docset->symbolTypeIcon(groupItem->symbolType);
        }
        default:
            return QVariant();
        }
    case Qt::DisplayRole:
        switch (indexLevel(index)) {
        case IndexLevel::Docset:
            switch (index.column()) {
            case SectionIndex::Name:
                return itemInRow(index.row())->docset->title();
            case SectionIndex::SearchPrefix:
                return itemInRow(index.row())->docset->keywords().join(QLatin1String(", "));
            default:
                return QVariant();
            }
        case IndexLevel::Group: {
            auto docsetItem = static_cast<DocsetItem *>(index.internalPointer());
            const QString symbolType = docsetItem->groups.at(index.row())->symbolType;
            return QStringLiteral("%1 (%2)").arg(pluralize(symbolType),
                                                 QString::number(docsetItem->docset->symbolCount(symbolType)));
        }
        case IndexLevel::Symbol: {
            auto groupItem = static_cast<GroupItem *>(index.internalPointer());
            auto it = groupItem->docsetItem->docset->symbols(groupItem->symbolType).cbegin();
            std::advance(it, index.row());
            return it.key();
        }
        default:
            return QVariant();
        }
    case Qt::ToolTipRole:
        if (index.column() != SectionIndex::Name) {
            return QVariant();
        }

        switch (indexLevel(index)) {
        case IndexLevel::Docset: {
            const auto docset = itemInRow(index.row())->docset;
            return tr("Version: %1r%2").arg(docset->version()).arg(docset->revision());
        }
        default:
            return QVariant();
        }
    case ItemDataRole::UrlRole:
        switch (indexLevel(index)) {
        case IndexLevel::Docset:
            return itemInRow(index.row())->docset->indexFileUrl();
        case IndexLevel::Symbol: {
            auto groupItem = static_cast<GroupItem *>(index.internalPointer());
            auto it = groupItem->docsetItem->docset->symbols(groupItem->symbolType).cbegin();
            std::advance(it, index.row());
            return it.value();
        }
        default:
            return QVariant();
        }
    case ItemDataRole::DocsetNameRole:
        if (index.parent().isValid())
            return QVariant();
        return itemInRow(index.row())->docset->name();
    case ItemDataRole::UpdateAvailableRole:
        if (index.parent().isValid())
            return QVariant();
        return itemInRow(index.row())->docset->hasUpdate;
    default:
        return QVariant();
    }
}

QModelIndex ListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return {};
    }

    switch (indexLevel(parent)) {
    case IndexLevel::Root:
        return createIndex(row, column);
    case IndexLevel::Docset:
        return createIndex(row, column, static_cast<void *>(itemInRow(parent.row())));
    case IndexLevel::Group: {
        auto docsetItem = static_cast<DocsetItem *>(parent.internalPointer());
        return createIndex(row, column, docsetItem->groups.at(parent.row()));
    }
    default:
        return {};
    }
}

QModelIndex ListModel::parent(const QModelIndex &child) const
{
    switch (indexLevel(child)) {
    case IndexLevel::Group: {
        auto item = static_cast<DocsetItem *>(child.internalPointer());

        auto it = std::find_if(m_docsetItems.cbegin(), m_docsetItems.cend(), [item](const auto &pair) {
            return pair.second == item;
        });

        if (it == m_docsetItems.cend()) {
            // TODO: Report error, this should never happen.
            return {};
        }

        const int row = static_cast<int>(std::distance(m_docsetItems.begin(), it));
        return createIndex(row, 0);
    }
    case IndexLevel::Symbol: {
        auto item = static_cast<GroupItem *>(child.internalPointer());
        return createIndex(item->docsetItem->groups.indexOf(item), 0, item->docsetItem);
    }
    default:
        return {};
    }
}

int ListModel::columnCount(const QModelIndex &parent) const
{
    if (indexLevel(parent) == IndexLevel::Root) {
        return 3;
    }

    return 1;
}

int ListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) {
        return 0;
    }

    switch (indexLevel(parent)) {
    case IndexLevel::Root:
        return static_cast<int>(m_docsetItems.size());
    case IndexLevel::Docset:
        return itemInRow(parent.row())->docset->symbolCounts().count();
    case IndexLevel::Group: {
        auto docsetItem = static_cast<DocsetItem *>(parent.internalPointer());
        return docsetItem->docset->symbolCount(docsetItem->groups.at(parent.row())->symbolType);
    }
    default:
        return 0;
    }
}

void ListModel::addDocset(const QString &name)
{
    const int row = static_cast<int>(std::distance(m_docsetItems.begin(), m_docsetItems.upper_bound(name)));
    beginInsertRows(QModelIndex(), row, row);

    auto docsetItem = new DocsetItem();
    docsetItem->docset = m_docsetRegistry->docset(name);

    const auto keys = docsetItem->docset->symbolCounts().keys();
    for (const QString &symbolType : keys) {
        auto groupItem = new GroupItem();
        groupItem->docsetItem = docsetItem;
        groupItem->symbolType = symbolType;
        docsetItem->groups.append(groupItem);
    }

    m_docsetItems.insert({name, docsetItem});

    endInsertRows();
}

void ListModel::removeDocset(const QString &name)
{
    auto it = m_docsetItems.find(name);
    if (it == m_docsetItems.cend()) {
        // TODO: Investigate why this can happen (see #420)
        return;
    }

    const int row = static_cast<int>(std::distance(m_docsetItems.begin(), it));
    beginRemoveRows(QModelIndex(), row, row);

    qDeleteAll(it->second->groups);
    delete it->second;
    m_docsetItems.erase(it);

    endRemoveRows();
}

QString ListModel::pluralize(const QString &s)
{
    if (s.endsWith(QLatin1String("y"))) {
        return s.left(s.length() - 1) + QLatin1String("ies");
    }

    return s + (s.endsWith('s') ? QLatin1String("es") : QLatin1String("s"));
}

ListModel::IndexLevel ListModel::indexLevel(const QModelIndex &index)
{
    if (!index.isValid()) {
        return IndexLevel::Root;
    }

    if (!index.internalPointer()) {
        return IndexLevel::Docset;
    }

    if (*static_cast<IndexLevel *>(index.internalPointer()) == IndexLevel::Docset) {
        return IndexLevel::Group;
    }

    return IndexLevel::Symbol;
}

ListModel::DocsetItem *ListModel::itemInRow(int row) const
{
    auto it = m_docsetItems.cbegin();
    std::advance(it, row);
    return it->second;
}
