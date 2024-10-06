// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include "searchmodel.h"

#include "docset.h"
#include "itemdatarole.h"

using namespace Zeal::Registry;

SearchModel::SearchModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

SearchModel *SearchModel::clone(QObject *parent)
{
    auto model = new SearchModel(parent);
    model->m_dataList = m_dataList;
    return model;
}

bool SearchModel::isEmpty() const
{
    return m_dataList.isEmpty();
}

QVariant SearchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    auto item = static_cast<SearchResult *>(index.internalPointer());

    switch (role) {
    case Qt::DisplayRole:
        return item->name;

    case Qt::DecorationRole:
        return item->docset->symbolTypeIcon(item->type);

    case ItemDataRole::DocsetIconRole:
        return item->docset->icon();

    case ItemDataRole::UrlRole:
        return item->docset->searchResultUrl(*item);

    default:
        return QVariant();
    }
}

QModelIndex SearchModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || m_dataList.count() <= row || column > 1)
        return {};

    // FIXME: const_cast
    auto item = const_cast<SearchResult *>(&m_dataList.at(row));
    return createIndex(row, column, item);
}

int SearchModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_dataList.count();
    return 0;
}

bool SearchModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row + count <= m_dataList.size() && !parent.isValid()) {
        beginRemoveRows(parent, row, row + count - 1);
        while (count) {
            m_dataList.removeAt(row);
            --count;
        }
        endRemoveRows();
        return true;
    }
    return false;
}

void SearchModel::removeSearchResultWithName(const QString &name)
{
    QMutableListIterator<SearchResult> iterator(m_dataList);

    int rowNum = 0;
    while (iterator.hasNext()) {
        if (iterator.next().docset->name() == name) {
            beginRemoveRows(QModelIndex(), rowNum, rowNum);
            iterator.remove();
            rowNum -= 1;
            endRemoveRows();
        }
        rowNum += 1;
    }
}

void SearchModel::setResults(const QList<SearchResult> &results)
{
    beginResetModel();
    m_dataList = results;
    endResetModel();
    emit updated();
}
