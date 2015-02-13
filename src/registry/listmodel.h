#ifndef LISTMODEL_H
#define LISTMODEL_H

#include <QAbstractListModel>
#include <QMap>

namespace Zeal {

class Docset;
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

    DocsetRegistry *m_docsetRegistry;

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

} // namespace Zeal

#endif // LISTMODEL_H
