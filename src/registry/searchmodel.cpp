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
    if (!index.isValid())
        return QVariant();

    SearchResult *item = static_cast<SearchResult *>(index.internalPointer());

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            if (item->parentName.isEmpty())
                return item->name;
            else
                return QString("%1 (%2)").arg(item->name, item->parentName);
        case 1:
            return QDir(item->docset->documentPath()).absoluteFilePath(item->path);
        default:
            return QVariant();
        }

    case Qt::DecorationRole:
        if (index.column() != 0)
            return QVariant();
        return QIcon(QString("typeIcon:%1.png").arg(item->type));

    case Roles::DocsetIconRole:
        return item->docset->icon();

    default:
        return QVariant();
    }
}

QModelIndex SearchModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || m_dataList.count() <= row || column > 1)
        return QModelIndex();

    return createIndex(row, column, (void *)&m_dataList.at(row));
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

void SearchModel::setResults(const QList<SearchResult> &results)
{
    beginResetModel();
    m_dataList = results;
    endResetModel();
    emit queryCompleted();
}
