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

#include "searchmodel.h"

#include "docset.h"
#include "itemdatarole.h"

using namespace Zeal::Registry;

SearchModel::SearchModel(QObject *parent) :
    QAbstractListModel(parent)
{
}

SearchModel::SearchModel(const SearchModel &other) :
    QAbstractListModel(other.d_ptr->parent),
    m_dataList(other.m_dataList)
{
}

bool SearchModel::isEmpty() const
{
    return m_dataList.isEmpty();
}

QVariant SearchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    SearchResult *item = static_cast<SearchResult *>(index.internalPointer());

    switch (role) {
    case Qt::DisplayRole:
        return item->name;

    case Qt::DecorationRole:
        return item->docset->symbolTypeIcon(item->type);

    case ItemDataRole::DocsetIconRole:
        return item->docset->icon();

    case ItemDataRole::UrlRole:
        return item->url;

    default:
        return QVariant();
    }
}

QModelIndex SearchModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || m_dataList.count() <= row || column > 1)
        return QModelIndex();

    // FIXME: const_cast
    SearchResult *item = const_cast<SearchResult *>(&m_dataList.at(row));
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
    emit queryCompleted();
}
