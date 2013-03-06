#include "quazipdir.h"

#include <QSet>
#include <QSharedData>

/// \cond internal
class QuaZipDirPrivate: public QSharedData {
    friend class QuaZipDir;
private:
    QuaZipDirPrivate(QuaZip *zip, const QString &dir = QString()):
        zip(zip), dir(dir), caseSensitivity(QuaZip::csDefault),
        filter(QDir::NoFilter), sorting(QDir::NoSort) {}
    QuaZip *zip;
    QString dir;
    QuaZip::CaseSensitivity caseSensitivity;
    QDir::Filters filter;
    QStringList nameFilters;
    QDir::SortFlags sorting;
    template<typename TFileInfoList>
    bool entryInfoList(QStringList nameFilters, QDir::Filters filter,
        QDir::SortFlags sort, TFileInfoList &result) const;
    inline QString simplePath() const {return QDir::cleanPath(dir);}
};
/// \endcond

QuaZipDir::QuaZipDir(const QuaZipDir &that):
    d(that.d)
{
}

QuaZipDir::QuaZipDir(QuaZip *zip, const QString &dir):
    d(new QuaZipDirPrivate(zip, dir))
{
    if (d->dir.startsWith('/'))
        d->dir = d->dir.mid(1);
}

QuaZipDir::~QuaZipDir()
{
}

bool QuaZipDir::operator==(const QuaZipDir &that)
{
    return d->zip == that.d->zip && d->dir == that.d->dir;
}

QuaZipDir& QuaZipDir::operator=(const QuaZipDir &that)
{
    this->d = that.d;
    return *this;
}

QString QuaZipDir::operator[](int pos) const
{
    return entryList().at(pos);
}

QuaZip::CaseSensitivity QuaZipDir::caseSensitivity() const
{
    return d->caseSensitivity;
}

bool QuaZipDir::cd(const QString &directoryName)
{
    if (directoryName == "/") {
        d->dir = "";
        return true;
    }
    QString dirName = directoryName;
    if (dirName.endsWith('/'))
        dirName.chop(1);
    if (dirName.contains('/')) {
        QuaZipDir dir(*this);
        if (dirName.startsWith('/')) {
#ifdef QUAZIP_QUAZIPDIR_DEBUG
            qDebug("QuaZipDir::cd(%s): going to /",
                    dirName.toUtf8().constData());
#endif
            if (!dir.cd("/"))
                return false;
        }
        QStringList path = dirName.split('/', QString::SkipEmptyParts);
        for (QStringList::const_iterator i = path.constBegin();
                i != path.end();
                ++i) {
            const QString &step = *i;
#ifdef QUAZIP_QUAZIPDIR_DEBUG
            qDebug("QuaZipDir::cd(%s): going to %s",
                    dirName.toUtf8().constData(),
                    step.toUtf8().constData());
#endif
            if (!dir.cd(step))
                return false;
        }
        d->dir = dir.path();
        return true;
    } else { // no '/'
        if (dirName == ".") {
            return true;
        } else if (dirName == "..") {
            if (isRoot()) {
                return false;
            } else {
                int slashPos = d->dir.lastIndexOf('/');
                if (slashPos == -1) {
                    d->dir = "";
                } else {
                    d->dir = d->dir.left(slashPos);
                }
                return true;
            }
        } else { // a simple subdirectory
            if (exists(dirName)) {
                if (isRoot())
                    d->dir = dirName;
                else
                    d->dir += "/" + dirName;
                return true;
            } else {
                return false;
            }
        }
    }
}

bool QuaZipDir::cdUp()
{
    return cd("..");
}

uint QuaZipDir::count() const
{
    return entryList().count();
}

QString QuaZipDir::dirName() const
{
    return QDir(d->dir).dirName();
}

QuaZipFileInfo QuaZipDir_getFileInfo(QuaZip *zip, bool *ok, 
                                  const QString &relativeName,
                                  bool isReal)
{
    QuaZipFileInfo info;
    if (isReal) {
        *ok = zip->getCurrentFileInfo(&info);
    } else {
        *ok = true;
        info.compressedSize = 0;
        info.crc = 0;
        info.diskNumberStart = 0;
        info.externalAttr = 0;
        info.flags = 0;
        info.internalAttr = 0;
        info.method = 0;
        info.uncompressedSize = 0;
        info.versionCreated = info.versionNeeded = 0;
    }
    info.name = relativeName;
    return info;
}

template<typename TFileInfoList>
void QuaZipDir_convertInfoList(const QList<QuaZipFileInfo> &from, TFileInfoList &to);

template<>
void QuaZipDir_convertInfoList(const QList<QuaZipFileInfo> &from, QList<QuaZipFileInfo> &to)
{
    to = from;
}

template<>
void QuaZipDir_convertInfoList(const QList<QuaZipFileInfo> &from, QStringList &to)
{
    to.clear();
    for (QList<QuaZipFileInfo>::const_iterator i = from.constBegin();
            i != from.constEnd();
            ++i) {
        to.append(i->name);
    }
}

/// \cond internal
/**
  An utility class to restore the current file.
  */
class QuaZipDirRestoreCurrent {
public:
    inline QuaZipDirRestoreCurrent(QuaZip *zip):
        zip(zip), currentFile(zip->getCurrentFileName()) {}
    inline ~QuaZipDirRestoreCurrent()
    {
        zip->setCurrentFile(currentFile);
    }
private:
    QuaZip *zip;
    QString currentFile;
};
/// \endcond

/// \cond internal
class QuaZipDirComparator
{
    private:
        QDir::SortFlags sort;
        static QString getExtension(const QString &name);
        int compareStrings(const QString &string1, const QString &string2);
    public:
        inline QuaZipDirComparator(QDir::SortFlags sort): sort(sort) {}
        bool operator()(const QuaZipFileInfo &info1, const QuaZipFileInfo &info2);
};

QString QuaZipDirComparator::getExtension(const QString &name)
{
    if (name.endsWith('.') || name.indexOf('.', 1) == -1) {
        return "";
    } else {
        return name.mid(name.lastIndexOf('.') + 1);
    }

}

int QuaZipDirComparator::compareStrings(const QString &string1,
        const QString &string2)
{
    if (sort & QDir::LocaleAware) {
        if (sort & QDir::IgnoreCase) {
            return string1.toLower().localeAwareCompare(string2.toLower());
        } else {
            return string1.localeAwareCompare(string2);
        }
    } else {
        return string1.compare(string2, (sort & QDir::IgnoreCase)
                ? Qt::CaseInsensitive : Qt::CaseSensitive);
    }
}

bool QuaZipDirComparator::operator()(const QuaZipFileInfo &info1,
        const QuaZipFileInfo &info2)
{
    QDir::SortFlags order = sort
        & (QDir::Name | QDir::Time | QDir::Size | QDir::Type);
    if ((sort & QDir::DirsFirst) == QDir::DirsFirst
            || (sort & QDir::DirsLast) == QDir::DirsLast) {
        if (info1.name.endsWith('/') && !info2.name.endsWith('/'))
            return (sort & QDir::DirsFirst) == QDir::DirsFirst;
        else if (!info1.name.endsWith('/') && info2.name.endsWith('/'))
            return (sort & QDir::DirsLast) == QDir::DirsLast;
    }
    bool result;
    int extDiff;
    switch (order) {
        case QDir::Name:
            result = compareStrings(info1.name, info2.name) < 0;
            break;
        case QDir::Type:
            extDiff = compareStrings(getExtension(info1.name),
                    getExtension(info2.name));
            if (extDiff == 0) {
                result = compareStrings(info1.name, info2.name) < 0;
            } else {
                result = extDiff < 0;
            }
            break;
        case QDir::Size:
            if (info1.uncompressedSize == info2.uncompressedSize) {
                result = compareStrings(info1.name, info2.name) < 0;
            } else {
                result = info1.uncompressedSize < info2.uncompressedSize;
            }
            break;
        case QDir::Time:
            if (info1.dateTime == info2.dateTime) {
                result = compareStrings(info1.name, info2.name) < 0;
            } else {
                result = info1.dateTime < info2.dateTime;
            }
            break;
        default:
            qWarning("QuaZipDirComparator(): Invalid sort mode 0x%2X",
                    static_cast<unsigned>(sort));
            return false;
    }
    return (sort & QDir::Reversed) ? !result : result;
}

template<typename TFileInfoList>
bool QuaZipDirPrivate::entryInfoList(QStringList nameFilters, 
    QDir::Filters filter, QDir::SortFlags sort, TFileInfoList &result) const
{
    QString basePath = simplePath();
    if (!basePath.isEmpty())
        basePath += "/";
    int baseLength = basePath.length();
    result.clear();
    QuaZipDirRestoreCurrent saveCurrent(zip);
    if (!zip->goToFirstFile()) {
        return zip->getZipError() == UNZ_OK;
    }
    QDir::Filters fltr = filter;
    if (fltr == QDir::NoFilter)
        fltr = this->filter;
    if (fltr == QDir::NoFilter)
        fltr = QDir::AllEntries;
    QStringList nmfltr = nameFilters;
    if (nmfltr.isEmpty())
        nmfltr = this->nameFilters;
    QSet<QString> dirsFound;
    QList<QuaZipFileInfo> list;
    do {
        QString name = zip->getCurrentFileName();
        if (!name.startsWith(basePath))
            continue;
        QString relativeName = name.mid(baseLength);
        if (relativeName.isEmpty())
            continue;
        bool isDir = false;
        bool isReal = true;
        if (relativeName.contains('/')) {
            int indexOfSlash = relativeName.indexOf('/');
            // something like "subdir/"
            isReal = indexOfSlash == relativeName.length() - 1;
            relativeName = relativeName.left(indexOfSlash + 1);
            if (dirsFound.contains(relativeName))
                continue;
            isDir = true;
        }
        dirsFound.insert(relativeName);
        if ((fltr & QDir::Dirs) == 0 && isDir)
            continue;
        if ((fltr & QDir::Files) == 0 && !isDir)
            continue;
        if (!nmfltr.isEmpty() && QDir::match(nmfltr, relativeName))
            continue;
        bool ok;
        QuaZipFileInfo info = QuaZipDir_getFileInfo(zip, &ok, relativeName,
            isReal);
        if (!ok) {
            return false;
        }
        list.append(info);
    } while (zip->goToNextFile());
    QDir::SortFlags srt = sort;
    if (srt == QDir::NoSort)
        srt = sorting;
#ifdef QUAZIP_QUAZIPDIR_DEBUG
    qDebug("QuaZipDirPrivate::entryInfoList(): before sort:");
    foreach (QuaZipFileInfo info, list) {
        qDebug("%s\t%s", info.name.toUtf8().constData(),
                info.dateTime.toString(Qt::ISODate).toUtf8().constData());
    }
#endif
    if (srt != QDir::NoSort && (srt & QDir::Unsorted) != QDir::Unsorted) {
        if (QuaZip::convertCaseSensitivity(caseSensitivity)
                == Qt::CaseInsensitive)
            srt |= QDir::IgnoreCase;
        QuaZipDirComparator lessThan(srt);
        qSort(list.begin(), list.end(), lessThan);
    }
    QuaZipDir_convertInfoList(list, result);
    return true;
}

/// \endcond

QList<QuaZipFileInfo> QuaZipDir::entryInfoList(const QStringList &nameFilters,
    QDir::Filters filters, QDir::SortFlags sort) const
{
    QList<QuaZipFileInfo> result;
    if (d->entryInfoList(nameFilters, filters, sort, result))
        return result;
    else
        return QList<QuaZipFileInfo>();
}

QList<QuaZipFileInfo> QuaZipDir::entryInfoList(QDir::Filters filters,
    QDir::SortFlags sort) const
{
    return entryInfoList(QStringList(), filters, sort);
}

QStringList QuaZipDir::entryList(const QStringList &nameFilters,
    QDir::Filters filters, QDir::SortFlags sort) const
{
    QStringList result;
    if (d->entryInfoList(nameFilters, filters, sort, result))
        return result;
    else
        return QStringList();
}

QStringList QuaZipDir::entryList(QDir::Filters filters,
    QDir::SortFlags sort) const
{
    return entryList(QStringList(), filters, sort);
}

bool QuaZipDir::exists(const QString &filePath) const
{
    if (filePath == "/")
        return true;
    QString fileName = filePath;
    if (fileName.endsWith('/'))
        fileName.chop(1);
    if (fileName.contains('/')) {
        QFileInfo fileInfo(fileName);
#ifdef QUAZIP_QUAZIPDIR_DEBUG
        qDebug("QuaZipDir::exists(): fileName=%s, fileInfo.fileName()=%s, "
                "fileInfo.path()=%s", fileName.toUtf8().constData(),
                fileInfo.fileName().toUtf8().constData(),
                fileInfo.path().toUtf8().constData());
#endif
        QuaZipDir dir(*this);
        return dir.cd(fileInfo.path()) && dir.exists(fileInfo.fileName());
    } else {
        if (fileName == "..") {
            return !isRoot();
        } else if (fileName == ".") {
            return true;
        } else {
            QStringList entries = entryList(QDir::AllEntries, QDir::NoSort);
#ifdef QUAZIP_QUAZIPDIR_DEBUG
            qDebug("QuaZipDir::exists(): looking for %s",
                    fileName.toUtf8().constData());
            for (QStringList::const_iterator i = entries.constBegin();
                    i != entries.constEnd();
                    ++i) {
                qDebug("QuaZipDir::exists(): entry: %s",
                        i->toUtf8().constData());
            }
#endif
            Qt::CaseSensitivity cs = QuaZip::convertCaseSensitivity(
                    d->caseSensitivity);
            if (filePath.endsWith('/')) {
                return entries.contains(filePath, cs);
            } else {
                return entries.contains(fileName, cs)
                    || entries.contains(fileName + "/", cs);
            }
        }
    }
}

bool QuaZipDir::exists() const
{
    QDir thisDir(d->dir);
    return QuaZipDir(d->zip, thisDir.filePath("..")).exists(thisDir.dirName());
}

QString QuaZipDir::filePath(const QString &fileName) const
{
    return QDir(d->dir).filePath(fileName);
}

QDir::Filters QuaZipDir::filter()
{
    return d->filter;
}

bool QuaZipDir::isRoot() const
{
    return d->simplePath().isEmpty();
}

QStringList QuaZipDir::nameFilters() const
{
    return d->nameFilters;
}

QString QuaZipDir::path() const
{
    return d->dir;
}

QString QuaZipDir::relativeFilePath(const QString &fileName) const
{
    return QDir(d->dir).relativeFilePath(fileName);
}

void QuaZipDir::setCaseSensitivity(QuaZip::CaseSensitivity caseSensitivity)
{
    d->caseSensitivity = caseSensitivity;
}

void QuaZipDir::setFilter(QDir::Filters filters)
{
    d->filter = filters;
}

void QuaZipDir::setNameFilters(const QStringList &nameFilters)
{
    d->nameFilters = nameFilters;
}

void QuaZipDir::setPath(const QString &path)
{
    QString newDir = path;
    if (newDir == "/") {
        d->dir = "";
    } else {
        if (newDir.endsWith('/'))
            newDir.chop(1);
        if (newDir.startsWith('/'))
            newDir = newDir.mid(1);
        d->dir = newDir;
    }
}

void QuaZipDir::setSorting(QDir::SortFlags sort)
{
    d->sorting = sort;
}

QDir::SortFlags QuaZipDir::sorting() const
{
    return d->sorting;
}
