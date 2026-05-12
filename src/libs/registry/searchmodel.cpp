// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#include "searchmodel.h"

#include "docset.h"
#include "itemdatarole.h"

namespace Zeal::Registry {

SearchModel::SearchModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

SearchModel *SearchModel::clone(QObject *parent)
{
    auto *model = new SearchModel(parent);
    model->m_dataList = m_dataList;
    return model;
}

bool SearchModel::isEmpty() const
{
    return m_dataList.isEmpty();
}

QVariant SearchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_dataList.count()) {
        return QVariant();
    }

    const SearchResult &item = m_dataList.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
        return item.name;

    case Qt::DecorationRole:
        return Docset::symbolTypeIcon(item.type);

    case ItemDataRole::DocsetIconRole:
        return item.docsetIcon;

    case ItemDataRole::MatchPositionsRole:
        return QVariant::fromValue(item.matchPositions);

    case ItemDataRole::UrlRole:
        return item.url;

    default:
        return QVariant();
    }
}

QModelIndex SearchModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row < 0 || row >= m_dataList.count() || column != 0) {
        return {};
    }

    return createIndex(row, column);
}

int SearchModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return m_dataList.count();
    }
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
        if (iterator.next().docsetName == name) {
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

} // namespace Zeal::Registry
