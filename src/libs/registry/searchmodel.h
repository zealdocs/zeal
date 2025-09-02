// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_REGISTRY_SEARCHMODEL_H
#define ZEAL_REGISTRY_SEARCHMODEL_H

#include "searchresult.h"

#include <QAbstractListModel>

namespace Zeal {
namespace Registry {

class SearchModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(SearchModel)
public:
    explicit SearchModel(QObject *parent = nullptr);
    SearchModel *clone(QObject *parent = nullptr);

    bool isEmpty() const;

    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;
    void removeSearchResultWithName(const QString &name);

public slots:
    void setResults(const QList<SearchResult> &results = QList<SearchResult>());

signals:
    void updated();

private:
    QList<SearchResult> m_dataList;
};

} // namespace Registry
} // namespace Zeal

#endif // ZEAL_REGISTRY_SEARCHMODEL_H
