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
    if (!index.isValid())
        return QVariant();

    const QStringList parts = i2s(index)->split('/');

    switch (role) {
    case Qt::DecorationRole:
        if (parts.size() == 1)
            return m_docsetRegistry->entry(*i2s(index))->icon();
        else /// TODO: Show Unknown.png for non-existent icons (e.g. specialization)
            return QIcon(QString(QStringLiteral("typeIcon:%1.png")).arg(parts[1]));
    case Qt::DisplayRole:
        if (index.column() > 0)
            return *i2s(index);

        switch (parts.size()) {
        case 1: // Docset name
            return m_docsetRegistry->entry(parts[0])->metadata.title();
        case 2: // Symbol group
            return pluralize(parts[1]);
        default: // Symbol name with slashes (trim only "docset/type")
            QString text = parts.last();
            for (int i = parts.length() - 2; i > 1; --i)
                text = parts[i] + "/" + text;
            return text;
        }
    }
    return QVariant();
}

QModelIndex ListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row >= m_docsetRegistry->count() || row == -1)
            return QModelIndex();

        if (column == 0) {
            return createIndex(row, column, (void *)string(m_docsetRegistry->names().at(row)));
        } else if (column == 1) {
            const Docset * const docset = m_docsetRegistry->entry(m_docsetRegistry->names().at(row));

            QString indexPath = docset->info.indexPath.isEmpty()
                    ? QStringLiteral("index.html")
                    : docset->info.indexPath;

            /// TODO: Check if file exists
            const QDir dir(docset->documentPath());
            return createIndex(row, column, (void *)string(dir.absoluteFilePath(indexPath)));
        }

        return QModelIndex();
    } else {
        const QStringList parts = i2s(parent)->split(QLatin1String("/"));
        const Docset * const docset = m_docsetRegistry->entry(parts[0]);
        if (parts.size() == 1) {
            // i2s(parent) == docsetName
            if (column == 0) {
                const QString type = Docset::symbolTypeToStr(docset->symbolCounts().keys().at(row));
                return createIndex(row, column, (void *)string(*i2s(parent) + "/" + type));
            }
        } else {
            const QString type = parts[1];
            if (row >= docset->symbolCount(type))
                return QModelIndex();

            const QMap<QString, QString> &symbols = docset->symbols(Docset::strToSymbolType(type));
            if (symbols.isEmpty())
                return QModelIndex();
            auto it = symbols.cbegin() + row;
            if (column == 0) {
                return createIndex(row, column, (void *)string(
                                       QString("%1/%2/%3").arg(docset->name(), parts[1], it.key())));
            } else if (column == 1) {
                return createIndex(row, column, (void *)string(it.value()));
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
        const QStringList parts = i2s(parent)->split(QLatin1String("/"));
        const Docset * const docset = m_docsetRegistry->entry(parts[0]);
        if (parts.size() == 1)
            return docset->symbolCounts().size();
        else if (parts.size() == 2)  // parent is docset/type
            return docset->symbolCount(parts[1]);
        else // module - no sub items
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

QString ListModel::pluralize(const QString &s)
{
    if (s.endsWith(QLatin1String("y")))
        return s.left(s.length() - 1) + QLatin1String("ies");
    else
        return s + (s.endsWith('s') ? QStringLiteral("es") : QStringLiteral("s"));
}

const QString *ListModel::i2s(const QModelIndex &index) const
{
    return static_cast<const QString *>(index.internalPointer());
}

const QString *ListModel::string(const QString &str) const
{
    if (m_strings.find(str) == m_strings.end())
        const_cast<QSet<QString> &>(m_strings).insert(str);
    return &*m_strings.find(str);
}
