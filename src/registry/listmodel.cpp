#include "listmodel.h"

#include "docsetregistry.h"

#include <QDir>
#include <QSqlQuery>

using namespace Zeal;

/// TODO: Get rid of const_casts

ListModel::ListModel(DocsetRegistry *docsetRegistry, QObject *parent) :
    QAbstractItemModel(parent),
    m_docsetRegistry(docsetRegistry)
{
}

QVariant ListModel::data(const QModelIndex &index, int role) const
{
    if ((role != Qt::DisplayRole && role != Qt::DecorationRole && role != DocsetNameRole)
            || !index.isValid())
        return QVariant();

    if (role == Qt::DecorationRole) {
        if (i2s(index)->indexOf('/') == -1)
            return QVariant(m_docsetRegistry->entry(*i2s(index)).icon());
        else
            return QVariant();
    }

    if (index.column() == 0) {
        const QStringList retlist = i2s(index)->split('/');
        QString retval = retlist.last();
        if (retlist.size() == 1) { // docset name
            if (role == Qt::DisplayRole)
                retval = m_docsetRegistry->entry(retval).info.bundleName;
        } else if (retlist.size() > 2) {  // name with slashes - trim only "docset/type"
            for (int i = retlist.length() - 2; i > 1; --i)
                retval = retlist[i] + "/" + retval;
        }
        return QVariant(retval);
    } else {
        return QVariant(*i2s(index));
    }
}

QModelIndex ListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row >= m_docsetRegistry->count() || row == -1)
            return QModelIndex();

        if (column == 0) {
            return createIndex(row, column, (void *)string(m_docsetRegistry->names().at(row)));
        } else if (column == 1) {
            const Docset &docset = m_docsetRegistry->entry(m_docsetRegistry->names().at(row));

            QString indexPath = docset.info.indexPath.isEmpty()
                    ? QStringLiteral("index.html")
                    : docset.info.indexPath;

            /// TODO: Check if file exists
            const QDir dir(docset.documentPath());
            return createIndex(row, column, (void *)string(dir.absoluteFilePath(indexPath)));
        }

        return QModelIndex();
    } else {
        QString docsetName;
        for (const QString &name : m_docsetRegistry->names()) {
            if (i2s(parent)->startsWith(name + "/"))
                docsetName = name;
        }
        if (docsetName.isEmpty()) {
            // i2s(parent) == docsetName
            if (column == 0) {
                QList<QString> types;
                for (const QPair<QString, QString> &pair : modulesCounts().keys()) {
                    if (pair.first == *i2s(parent))
                        types.append(pair.second);
                }
                qSort(types);
                return createIndex(row, column,
                                   (void *)string(*i2s(parent) + "/" + pluralize(types[row])));
            }
        } else {
            const QString type = singularize(i2s(parent)->split('/')[1]);
            if (row >= modulesCounts()[QPair<QString, QString>(docsetName, type)])
                return QModelIndex();
            if (column == 0) {
                return createIndex(row, column,
                                   (void *)string(
                                       QString("%1/%2/%3").arg(docsetName, pluralize(type),
                                                               item(*i2s(parent), row).first)));
            } else if (column == 1) {
                return createIndex(row, column, (void *)string(item(*i2s(parent), row).second));
            }
        }
        return QModelIndex();
    }
}

QModelIndex ListModel::parent(const QModelIndex &child) const
{
    if (child.isValid() && i2s(child)->count("/") == 1) {  // docset/type
        return createIndex(0, 0, (void *)string(i2s(child)->split('/')[0]));
    } else if (child.isValid() && i2s(child)->count("/") >= 2) {    // docset/type/item (item can contain slashes)
        return createIndex(0, 0, (void *)string(i2s(child)->split('/')[0] + "/"
                           + i2s(child)->split('/')[1]));
    } else {
        return QModelIndex();
    }
}

int ListModel::columnCount(const QModelIndex &parent) const
{
    if (!parent.isValid() || i2s(parent)->count("/") == 1) // child of docset/type
        return 2;
    else
        return 1;
}

int ListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0 && !i2s(parent)->endsWith("/Modules"))
        return 0;

    if (!parent.isValid()) {
        // root
        return m_docsetRegistry->count();
    } else {
        const QString *parentStr = i2s(parent);
        if (parentStr->indexOf("/") == -1) {
            // docset - show types
            int numTypes = 0;
            const QList<QPair<QString, QString>> keys = modulesCounts().keys();
            for (const QPair<QString, QString> &key : keys) {
                if (parentStr == key.first)
                    numTypes += 1;
            }
            return numTypes;
        } else if (parentStr->count("/") == 1) { // parent is docset/type
            // type count
            QString type = singularize(parentStr->split("/")[1]);
            return modulesCounts()[QPair<QString, QString>(parentStr->split('/')[0], type)];
        }
        // module - no sub items
        return 0;
    }
}

bool ListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;

    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
        m_docsetRegistry->remove(m_docsetRegistry->names()[row + i]);
    endRemoveRows();

    return true;
}

void ListModel::resetModulesCounts()
{
    m_modulesCounts.clear();
}

QString ListModel::pluralize(const QString &s)
{
    return s + (s.endsWith('s') ? QStringLiteral("es") : QStringLiteral("s"));
}

QString ListModel::singularize(const QString &s)
{
    return s.left(s.length() - (s.endsWith(QStringLiteral("ses")) ? 2 : 1));
}

const QString *ListModel::i2s(const QModelIndex &index) const
{
    return static_cast<const QString *>(index.internalPointer());
}

const QHash<QPair<QString, QString>, int> ListModel::modulesCounts() const
{
    if (!m_modulesCounts.isEmpty())
        return m_modulesCounts;

    for (const Docset &docset : m_docsetRegistry->docsets()) {
        QSqlQuery q;
        if (docset.type() == Docset::Type::Dash) {
            q = docset.db.exec("SELECT type, COUNT(*) FROM searchIndex GROUP BY type");
        } else if (docset.type() == Docset::Type::ZDash) {
            q = docset.db.exec("SELECT ztypename, COUNT(*) FROM ztoken JOIN ztokentype"
                               " ON ztoken.ztokentype = ztokentype.z_pk GROUP BY ztypename");
        }

        while (q.next()) {
            int count = q.value(1).toInt();
            QString typeName = q.value(0).toString();
            const_cast<QHash<QPair<QString, QString>, int> &>(m_modulesCounts)
                    [QPair<QString, QString>(docset.name(), typeName)] = count;
        }
    }
    return m_modulesCounts;
}

const QPair<QString, QString> ListModel::item(const QString &path, int index) const
{
    QPair<QString, int> pair(path, index);
    if (m_items.contains(pair))
        return m_items[pair];

    const Docset &docset = m_docsetRegistry->entry(path.split('/')[0]);

    const QString type = singularize(path.split('/')[1]);

    QString queryStr;
    switch (docset.type()) {
    case Docset::Type::Dash:
        queryStr = QString("SELECT name, path FROM searchIndex WHERE type='%1' ORDER BY name ASC")
                .arg(type);
        break;
    case Docset::Type::ZDash:
        queryStr = QString("SELECT ztokenname, zpath, zanchor FROM ztoken "
                           "JOIN ztokenmetainformation ON ztoken.zmetainformation = ztokenmetainformation.z_pk "
                           "JOIN zfilepath ON ztokenmetainformation.zfile = zfilepath.z_pk "
                           "JOIN ztokentype ON ztoken.ztokentype = ztokentype.z_pk WHERE ztypename='%1' "
                           "ORDER BY ztokenname ASC").arg(type);
        break;
    }

    QSqlQuery query = docset.db.exec(queryStr);

    int i = 0;
    while (query.next()) {
        QPair<QString, QString> item;
        item.first = query.value(0).toString();
        QString filePath = query.value(1).toString();

        /// FIXME: refactoring to use common code in ZealListModel and DocsetRegistry
        /// TODO: parent name, splitting by '.', as in DocsetRegistry
        if (docset.type() == Docset::Type::ZDash)
            filePath += QStringLiteral("#") + query.value(2).toString();
        item.second = QDir(docset.documentPath()).absoluteFilePath(filePath);
        const_cast<QHash<QPair<QString, int>, QPair<QString, QString>> &>(m_items)
                [QPair<QString, int>(path, i)] = item;
        ++i;
    }
    return m_items[pair];
}

const QString *ListModel::string(const QString &str) const
{
    if (m_strings.find(str) == m_strings.end())
        const_cast<QSet<QString> &>(m_strings).insert(str);
    return &*m_strings.find(str);
}
