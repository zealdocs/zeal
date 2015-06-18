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
        DocsetIconRole = Qt::UserRole
    };

    explicit SearchModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;

public slots:
    void setResults(const QList<SearchResult> &results = QList<SearchResult>());

signals:
    void queryCompleted();

private:
    QList<SearchResult> m_dataList;
};

} // namespace Zeal

#endif // SEARCHMODEL_H
