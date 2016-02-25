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

#include "searchmodel.h"

#include "core/application.h"
#include "core/lcs.h"
#include "registry/docsetregistry.h"
#include "registry/docsettoken.h"

#include <QDir>

using namespace Zeal;

SearchModel::SearchModel(QObject *parent) :
    QAbstractItemModel(parent)
{
}

QVariant SearchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    SearchResult *item = static_cast<SearchResult *>(index.internalPointer());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            if (item->token.parentName.isEmpty())
                return item->token.name;
            else
                return QString("%1").arg(item->token.full);
        case 1:
            return QDir(item->docset->documentPath()).absoluteFilePath(item->path);
        default:
            return QVariant();
        }

    case Qt::DecorationRole:
        return item->docset->icon();

    case Roles::TypeIconRole:
        if (index.column() != 0)
            return QVariant();
        return QIcon(QString("typeIcon:%1.png").arg(item->type));

    case Roles::HlPositionsRole: 
        // Return position of characters to display in bold
        return listHlPositions(*item);

    default:
        return QVariant();
    }
}

QModelIndex SearchModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || m_dataList.count() <= row || column > 1)
        return QModelIndex();

    /// FIXME: const_cast
    SearchResult *item = const_cast<SearchResult *>(&m_dataList.at(row));
    return createIndex(row, column, item);
}

QModelIndex SearchModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

int SearchModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_dataList.count();
    return 0;
}

int SearchModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2;
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

QList<QVariant> SearchModel::listHlPositions(const SearchResult &result) const
{
    QList<QVariant> hlPositions;
    Core::LCS lcs;
    int offset = result.token.parentName.length() + result.token.separator.length();

    switch (result.searchRelevancy.matchType) {
    case SearchRelevancy::ParentNameMatch:
        lcs = Core::LCS(result.token.parentName.toLower(), result.query.full);
        for (const int i: lcs.subsequencePositions(0))
            hlPositions << i;
        break;
    case SearchRelevancy::NameMatch:
        lcs = Core::LCS(result.token.name.toLower(), result.query.full);
        for (const int i: lcs.subsequencePositions(0))
            hlPositions << i + offset;
        break;
    case SearchRelevancy::BothMatch: 
        {
            lcs = Core::LCS(result.token.parentName.toLower(), result.query.parentName);
            for (const int i: lcs.subsequencePositions(0))
                hlPositions << i;
            lcs = Core::LCS(result.token.name.toLower(), result.query.name);
            for (const int i: lcs.subsequencePositions(0))
                hlPositions << i + offset;
            break;
        }
    case SearchRelevancy::FullMatch:
        lcs = Core::LCS(result.token.full.toLower(), result.query.full);
        for (const int i: lcs.subsequencePositions(0))
            hlPositions << i;
        break;
    case SearchRelevancy::NoMatch:
        break;
    }

    return hlPositions;
}
