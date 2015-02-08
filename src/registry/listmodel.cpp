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
    connect(m_docsetRegistry, &DocsetRegistry::docsetAdded, [this](const QString &name) {
        const int index = m_docsetRegistry->names().indexOf(name);
        beginInsertRows(QModelIndex(), index, index);
        endInsertRows();
    });

    connect(m_docsetRegistry, &DocsetRegistry::docsetRemoved, [this](const QString &name) {
        const int index = m_docsetRegistry->names().indexOf(name);
        beginRemoveRows(QModelIndex(), index, index);
        endRemoveRows();
    });

    connect(m_docsetRegistry, &DocsetRegistry::reset, [this]() {
        beginResetModel();
        endResetModel();
    });
}

QVariant ListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    const QStringList parts = i2s(index)->split('/');
    const Docset * const docset = m_docsetRegistry->docset(parts[0]);

    switch (role) {
    case Qt::DecorationRole:
        if (parts.size() == 1)
            return docset->icon();
        else /// TODO: Show Unknown.png for non-existent icons (e.g. specialization)
            return QIcon(QString(QLatin1String("typeIcon:%1.png")).arg(parts[1]));
    case Qt::DisplayRole:
        if (index.column() > 0)
            return *i2s(index);

        switch (parts.size()) {
        case 1: // Docset name
            return m_docsetRegistry->docset(parts[0])->metadata.title();
        case 2: // Symbol group
            return QString(QLatin1String("%1 (%2)")).arg(pluralize(parts[1]),
                    QString::number(docset->symbolCount(parts[1])));
        default: // Symbol name with slashes (trim only "docset/type")
            QString text = parts.last();
            for (int i = parts.length() - 2; i > 1; --i)
                text = parts[i] + "/" + text;
            return text;
        }
    case DocsetNameRole:
        return parts[0];
    }
    return QVariant();
}

QModelIndex ListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        if (row == -1 || row >= m_docsetRegistry->count() || column > 1)
            return QModelIndex();

        if (column == 0) {
            return createIndex(row, column, (void *)string(m_docsetRegistry->names().at(row)));
        } else if (column == 1) {
            const Docset * const docset = m_docsetRegistry->docset(m_docsetRegistry->names().at(row));

            QString indexPath = docset->info.indexPath.isEmpty()
                    ? QLatin1String("index.html")
                    : docset->info.indexPath;

            /// TODO: Check if file exists
            const QDir dir(docset->documentPath());
            return createIndex(row, column, (void *)string(dir.absoluteFilePath(indexPath)));
        }
    } else {
        const QStringList parts = i2s(parent)->split(QLatin1String("/"));
        const Docset * const docset = m_docsetRegistry->docset(parts[0]);
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
    }
    return QModelIndex();
}

QModelIndex ListModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();

    const QStringList parts = i2s(child)->split(QLatin1String("/"));

    switch (parts.size()) {
    case 1:
        return QModelIndex();
    case 2: // Docset/type
        return createIndex(0, 0, (void *)string(parts[0]));
    default: // Docset/type/item (item can contain slashes)
        return createIndex(0, 0, (void *)string(parts[0] + QLatin1String("/") + parts[1]));
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
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid()) {
        // root
        return m_docsetRegistry->count();
    } else {
        const QStringList parts = i2s(parent)->split(QLatin1String("/"));
        const Docset * const docset = m_docsetRegistry->docset(parts[0]);
        if (parts.size() == 1)
            return docset->symbolCounts().size();
        else if (parts.size() == 2)  // parent is docset/type
            return docset->symbolCount(parts[1]);
        else // module - no sub items
            return 0;
    }
}

QString ListModel::pluralize(const QString &s)
{
    if (s.endsWith(QLatin1String("y")))
        return s.left(s.length() - 1) + QLatin1String("ies");
    else
        return s + (s.endsWith('s') ? QLatin1String("es") : QLatin1String("s"));
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
