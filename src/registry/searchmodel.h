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

#ifndef SEARCHMODEL_H
#define SEARCHMODEL_H

#include "searchresult.h"

#include <QAbstractItemModel>

namespace Zeal {

class SearchModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Roles {
        TypeIconRole = Qt::UserRole
    };

    explicit SearchModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    void removeSearchResultWithName(const QString &name);

public slots:
    void setResults(const QList<SearchResult> &results = QList<SearchResult>());

signals:
    void queryCompleted();

private:
    QList<SearchResult> m_dataList;
};

} // namespace Zeal

#endif // SEARCHMODEL_H
