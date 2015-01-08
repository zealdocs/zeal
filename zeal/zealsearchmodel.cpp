#include "zealsearchmodel.h"

#include "zealdocsetsregistry.h"

ZealSearchModel::ZealSearchModel(QObject *parent) :
    QAbstractItemModel(parent)
{
}

Qt::ItemFlags ZealSearchModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;
    return QAbstractItemModel::flags(index);
}

QVariant ZealSearchModel::data(const QModelIndex &index, int role) const
{
    if ((role != Qt::DisplayRole && role != Qt::DecorationRole) || !index.isValid())
        return QVariant();

    auto item = static_cast<ZealSearchResult *>(index.internalPointer());

    if (role == Qt::DecorationRole) {
        if (index.column() == 0)
            return QVariant(ZealDocsetsRegistry::instance()->icon(item->docsetName()));
        return QVariant();
    }

    if (index.column() == 0) {
        if (!item->parentName().isEmpty())
            return QVariant(QString("%1 (%2)").arg(item->name(), item->parentName()));
        else
            return QVariant(item->name());

    } else if (index.column() == 1) {
        return QVariant(ZealDocsetsRegistry::instance()->dir(item->docsetName()).absoluteFilePath(item->path()));
    }
    return QVariant();
}

QVariant ZealSearchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section)
    Q_UNUSED(orientation)
    Q_UNUSED(role)
    return QVariant();
}

QModelIndex ZealSearchModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (dataList.count() <= row) return QModelIndex();
        auto &item = dataList.at(row);

        if (column == 0 || column == 1)
            return createIndex(row, column, (void *)&item);
    }
    return QModelIndex();
}

QModelIndex ZealSearchModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child)
    return QModelIndex();
}

int ZealSearchModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return dataList.count();
    return 0;
}

int ZealSearchModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 2;
}

void ZealSearchModel::setQuery(const QString &q)
{
    query = q;
    populateData();
}

void ZealSearchModel::populateData()
{
    if (query.isEmpty())
        ZealDocsetsRegistry::instance()->invalidateQueries();
    else
        ZealDocsetsRegistry::instance()->runQuery(query);
}

void ZealSearchModel::onQueryCompleted(const QList<ZealSearchResult> &results)
{
    beginResetModel();
    dataList = results;
    endResetModel();
    emit queryCompleted();
}
