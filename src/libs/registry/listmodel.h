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

#ifndef LISTMODEL_H
#define LISTMODEL_H

#include <QAbstractItemModel>
#include <QMap>

namespace Zeal {
namespace Registry {

class Docset;
class DocsetRegistry;

class ListModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ListModel(DocsetRegistry *docsetRegistry, QObject *parent = nullptr);
    ~ListModel() override;

    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int columnCount(const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent) const override;

private slots:
    void addDocset(const QString &name);
    void removeDocset(const QString &name);

private:
    enum Level {
        RootLevel,
        DocsetLevel,
        GroupLevel,
        SymbolLevel
    };

    inline static QString pluralize(const QString &s);
    inline static Level indexLevel(const QModelIndex &index);

    DocsetRegistry *m_docsetRegistry = nullptr;

    struct DocsetItem;
    struct GroupItem {
        const Level level = Level::GroupLevel;
        DocsetItem *docsetItem = nullptr;
        QString symbolType;
    };

    struct DocsetItem {
        const Level level = Level::DocsetLevel;
        Docset *docset = nullptr;
        QList<GroupItem *> groups;
    };

    QMap<QString, DocsetItem *> m_docsetItems;
};

} // namespace Registry
} // namespace Zeal

#endif // LISTMODEL_H
