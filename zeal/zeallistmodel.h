#ifndef ZEALLISTMODEL_H
#define ZEALLISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QSet>

class ZealListModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum {
        DocsetNameRole = Qt::UserRole
    };

    explicit ZealListModel(QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    bool removeRows(int row, int count, const QModelIndex &parent);

    void resetModulesCounts();

private:
    inline static QString pluralize(const QString &s);
    inline static QString singularize(const QString &s);

    const QString *i2s(const QModelIndex &index) const;
    const QHash<QPair<QString, QString>, int> modulesCounts() const;
    const QPair<QString, QString> item(const QString &path, int index) const;
    const QString *string(const QString &str) const;

    QHash<QPair<QString, QString>, int> m_modulesCounts;
    QHash<QPair<QString, int>, QPair<QString, QString>> m_items;
    QSet<QString> m_strings;
};

#endif // ZEALLISTMODEL_H
