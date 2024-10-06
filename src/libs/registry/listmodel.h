/****************************************************************************
**
** Copyright (C) Oleg Shparber, et al.
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** SPDX-License-Identifier: GPL-3.0-or-later
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
