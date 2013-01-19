#ifndef ZEALLISTMODEL_H
#define ZEALLISTMODEL_H

#include <Qt>
#include <QAbstractListModel>
#include <QSet>
#include <QHash>
#include <QSharedPointer>
#include <vector>
#include <string>
#include <iostream>
using namespace std;

class ZealListModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit ZealListModel(QObject *parent = 0);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
signals:
    
public slots:

private:
    const QString* i2s(const QModelIndex &index) const;
    QHash<QString, int> *modulesCounts;
    const QHash<QString, int> getModulesCounts() const;
    QSet<QString> *strings;
    const QString* getString(const QString& str) const {
        if(strings->find(str) == strings->end())
            (*strings).insert(str);
        return &*strings->find(str);
    }
};

#endif // ZEALLISTMODEL_H
