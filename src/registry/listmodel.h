#ifndef LISTMODEL_H
#define LISTMODEL_H

#include <QAbstractListModel>
#include <QHash>
#include <QSet>

namespace Zeal {

class DocsetRegistry;

class ListModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum {
        DocsetNameRole = Qt::UserRole
    };

    explicit ListModel(DocsetRegistry *docsetRegistry, QObject *parent = nullptr);

    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent) const;
    bool removeRows(int row, int count, const QModelIndex &parent);

private:
    inline static QString pluralize(const QString &s);
    inline static QString singularize(const QString &s);

    const QString *i2s(const QModelIndex &index) const;
    const QString *string(const QString &str = QString()) const;

    DocsetRegistry *m_docsetRegistry;
    QSet<QString> m_strings;
};

} // namespace Zeal

#endif // LISTMODEL_H
