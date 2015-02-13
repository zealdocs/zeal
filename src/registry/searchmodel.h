#ifndef SEARCHMODEL_H
#define SEARCHMODEL_H

#include "searchresult.h"

#include <QAbstractItemModel>

namespace Zeal {

class SearchModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit SearchModel(QObject *parent = 0);

    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;

public slots:
    void setResults(const QList<SearchResult> &results);

signals:
    void queryCompleted();

private:
    QList<SearchResult> m_dataList;
};

} // namespace Zeal

#endif // SEARCHMODEL_H
