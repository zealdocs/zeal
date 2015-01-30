#include "searchmodel.h"

#include "core/application.h"
#include "registry/docsetregistry.h"

#include <QDir>

using namespace Zeal;

SearchModel::SearchModel(QObject *parent) :
    QAbstractItemModel(parent)
{
}

QVariant SearchModel::data(const QModelIndex &index, int role) const
{
    if ((role != Qt::DisplayRole && role != Qt::DecorationRole) || !index.isValid())
        return QVariant();

    SearchResult *item = static_cast<SearchResult *>(index.internalPointer());

    if (role == Qt::DecorationRole) {
        if (index.column() == 0)
            return QVariant(Core::Application::docsetRegistry()->entry(item->docsetName()).icon());
        return QVariant();
    }

    if (index.column() == 0) {
        if (!item->parentName().isEmpty())
            return QVariant(QString("%1 (%2)").arg(item->name(), item->parentName()));
        else
            return QVariant(item->name());

    } else if (index.column() == 1) {
        const QDir dir(Core::Application::docsetRegistry()->entry(item->docsetName()).documentPath());
        return QVariant(dir.absoluteFilePath(item->path()));
    }
    return QVariant();
}

QModelIndex SearchModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (dataList.count() <= row) return QModelIndex();
        const SearchResult &item = dataList.at(row);

        if (column == 0 || column == 1)
            return createIndex(row, column, (void *)&item);
    }
    return QModelIndex();
}

QModelIndex SearchModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

int SearchModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return dataList.count();
    return 0;
}

int SearchModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2;
}

void SearchModel::setQuery(const QString &q)
{
    query = q;
    populateData();
}

void SearchModel::populateData()
{
    if (query.isEmpty())
        Core::Application::docsetRegistry()->invalidateQueries();
    else
        Core::Application::docsetRegistry()->runQuery(query);
}

void SearchModel::onQueryCompleted(const QList<SearchResult> &results)
{
    beginResetModel();
    dataList = results;
    endResetModel();
    emit queryCompleted();
}
