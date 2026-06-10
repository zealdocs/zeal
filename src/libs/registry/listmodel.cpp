// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include "listmodel.h"

#include "docset.h"
#include "docsetregistry.h"
#include "itemdatarole.h"

#include <QLocale>

#include <iterator>

namespace Zeal::Registry {

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
        return {};
    }

    switch (role) {
    case Qt::DecorationRole:
        if (index.column() != SectionIndex::Name) {
            return {};
        }

        switch (indexLevel(index)) {
        case IndexLevel::Docset:
            return itemInRow(index.row())->docset->icon();
        case IndexLevel::Group: {
            auto *docsetItem = static_cast<DocsetItem *>(index.internalPointer());
            const QString &symbolType = docsetItem->groups.at(index.row())->symbolType;
            return Docset::symbolTypeIcon(symbolType);
        }
        case IndexLevel::Symbol: {
            auto *groupItem = static_cast<GroupItem *>(index.internalPointer());
            return Docset::symbolTypeIcon(groupItem->symbolType);
        }
        default:
            return {};
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
                return {};
            }
        case IndexLevel::Group: {
            auto *docsetItem = static_cast<DocsetItem *>(index.internalPointer());
            const QString &symbolType = docsetItem->groups.at(index.row())->symbolType;
            return QStringLiteral("%1 (%2)").arg(pluralize(symbolType),
                                                 QString::number(docsetItem->docset->symbolCount(symbolType)));
        }
        case IndexLevel::Symbol: {
            auto *groupItem = static_cast<GroupItem *>(index.internalPointer());
            return groupItem->docsetItem->docset->symbols(groupItem->symbolType).at(index.row()).first;
        }
        default:
            return {};
        }
    case Qt::ToolTipRole: {
        if (index.column() != SectionIndex::Name || indexLevel(index) != IndexLevel::Docset) {
            return {};
        }

        auto *const docset = itemInRow(index.row())->docset;
        QString tooltip = tr("Version: %1r%2").arg(docset->version()).arg(docset->revision());
        if (docset->hasUpdate()) {
            const Docset::UpdateInfo &update = docset->update().value();
            if (update.size > 0) {
                tooltip += QLatin1Char('\n')
                         + tr("Update available: %1r%2 (%3)")
                               .arg(update.version)
                               .arg(update.revision)
                               .arg(QLocale::system().formattedDataSize(update.size));
            } else {
                tooltip += QLatin1Char('\n') + tr("Update available: %1r%2").arg(update.version).arg(update.revision);
            }
        }

        return tooltip;
    }
    case ItemDataRole::UrlRole:
        switch (indexLevel(index)) {
        case IndexLevel::Docset:
            return itemInRow(index.row())->docset->indexFileUrl();
        case IndexLevel::Symbol: {
            auto *groupItem = static_cast<GroupItem *>(index.internalPointer());
            return groupItem->docsetItem->docset->symbols(groupItem->symbolType).at(index.row()).second;
        }
        default:
            return {};
        }
    case ItemDataRole::DocsetNameRole:
        if (index.parent().isValid()) {
            return {};
        }
        return itemInRow(index.row())->docset->name();
    case ItemDataRole::UpdateAvailableRole:
        if (index.parent().isValid()) {
            return {};
        }

        return itemInRow(index.row())->docset->hasUpdate();
    default:
        return {};
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
        auto *docsetItem = static_cast<DocsetItem *>(parent.internalPointer());
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
        auto *item = static_cast<DocsetItem *>(child.internalPointer());
        return createIndex(item->row, 0);
    }
    case IndexLevel::Symbol: {
        auto *item = static_cast<GroupItem *>(child.internalPointer());
        return createIndex(static_cast<int>(item->docsetItem->groups.indexOf(item)), 0, item->docsetItem);
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
        return static_cast<int>(itemInRow(parent.row())->docset->symbolCounts().count());
    case IndexLevel::Group: {
        auto *docsetItem = static_cast<DocsetItem *>(parent.internalPointer());
        return docsetItem->docset->symbolCount(docsetItem->groups.at(parent.row())->symbolType);
    }
    default:
        return 0;
    }
}

void ListModel::addDocset(const QString &name)
{
    if (m_docsetItems.contains(name)) {
        return;
    }

    const int row = static_cast<int>(std::distance(m_docsetItems.begin(), m_docsetItems.upper_bound(name)));
    beginInsertRows(QModelIndex(), row, row);

    auto *docsetItem = new DocsetItem();
    docsetItem->docset = m_docsetRegistry->docset(name);

    const auto keys = docsetItem->docset->symbolCounts().keys();
    for (const QString &symbolType : keys) {
        auto *groupItem = new GroupItem();
        groupItem->docsetItem = docsetItem;
        groupItem->symbolType = symbolType;
        docsetItem->groups.append(groupItem);
    }

    m_docsetItems.insert({name, docsetItem});
    m_docsetRows.insert(m_docsetRows.begin() + row, docsetItem);
    int rowIndex = 0;
    for (auto *item : m_docsetRows) {
        item->row = rowIndex++;
    }

    endInsertRows();
}

void ListModel::removeDocset(const QString &name)
{
    auto it = m_docsetItems.find(name);
    if (it == m_docsetItems.cend()) {
        // TODO: Investigate why this can happen (see #420)
        return;
    }

    auto *item = it->second;
    const int row = item->row;
    beginRemoveRows(QModelIndex(), row, row);

    m_docsetItems.erase(it);
    m_docsetRows.erase(m_docsetRows.begin() + row);
    int rowIndex = 0;
    for (auto *docsetRow : m_docsetRows) {
        docsetRow->row = rowIndex++;
    }

    qDeleteAll(item->groups);
    delete item;

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

    if (index.internalPointer() == nullptr) {
        return IndexLevel::Docset;
    }

    return static_cast<const ItemBase *>(index.internalPointer())->level == IndexLevel::Docset ? IndexLevel::Group
                                                                                               : IndexLevel::Symbol;
}

ListModel::DocsetItem *ListModel::itemInRow(int row) const
{
    return m_docsetRows.at(row);
}

} // namespace Zeal::Registry
