#include "searchmodel.h"

#include "core/application.h"
#include "registry/docsetregistry.h"

#include <QDir>

using namespace Zeal;

SearchModel::SearchModel(QObject *parent) :
    QAbstractItemModel(parent)
{
}

void SearchModel::setQuery(const QString &q)
{
    m_query = q;
    populateData();
}

QVariant SearchModel::data(const QModelIndex &index, int role) const
{
    if ((role != Qt::DisplayRole && role != Qt::DecorationRole) || !index.isValid())
        return QVariant();

    SearchResult *item = static_cast<SearchResult *>(index.internalPointer());

    if (role == Qt::DecorationRole) {
        if (index.column() == 0)
            return Core::Application::docsetRegistry()->docset(item->docsetName())->icon();
        return QVariant();
    }

    if (index.column() == 0) {
        if (!item->parentName().isEmpty())
            return QString("%1 (%2)").arg(item->name(), item->parentName());
        else
            return item->name();

    } else if (index.column() == 1) {
        const QDir dir(Core::Application::docsetRegistry()->docset(item->docsetName())->documentPath());
        return dir.absoluteFilePath(item->path());
    }
    return QVariant();
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

void SearchModel::onQueryCompleted(const QList<SearchResult> &results)
{
    beginResetModel();
    m_dataList = results;
    endResetModel();
    emit queryCompleted();
}

void SearchModel::populateData()
{
    if (m_query.isEmpty())
        Core::Application::docsetRegistry()->invalidateQueries();
    else
        Core::Application::docsetRegistry()->runQuery(m_query);
}
