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

#ifndef ZEAL_REGISTRY_LISTMODEL_H
#define ZEAL_REGISTRY_LISTMODEL_H

#include <util/caseinsensitivemap.h>

#include <QAbstractItemModel>

namespace Zeal {
namespace Registry {

class Docset;
class DocsetRegistry;

class ListModel final : public QAbstractItemModel
{
    Q_OBJECT
    Q_DISABLE_COPY(ListModel)
public:
    ~ListModel() override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int columnCount(const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent) const override;

private slots:
    void addDocset(const QString &name);
    void removeDocset(const QString &name);

private:
    friend class DocsetRegistry;

    enum class IndexLevel {
        Root,
        Docset,
        Group,
        Symbol
    };

    explicit ListModel(DocsetRegistry *docsetRegistry);

    inline static QString pluralize(const QString &s);
    inline static IndexLevel indexLevel(const QModelIndex &index);

    DocsetRegistry *m_docsetRegistry = nullptr;

    struct DocsetItem;
    struct GroupItem {
        const IndexLevel level = IndexLevel::Group;
        DocsetItem *docsetItem = nullptr;
        QString symbolType;
    };

    struct DocsetItem {
        const IndexLevel level = IndexLevel::Docset;
        Docset *docset = nullptr;
        QList<GroupItem *> groups;
    };

    inline DocsetItem *itemInRow(int row) const;

    Util::CaseInsensitiveMap<DocsetItem *> m_docsetItems;
};

} // namespace Registry
} // namespace Zeal

#endif // ZEAL_REGISTRY_LISTMODEL_H
