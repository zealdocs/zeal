#ifndef QUAZIP_QUAZIPDIR_H
#define QUAZIP_QUAZIPDIR_H

class QuaZipDirPrivate;

#include "quazip.h"
#include "quazipfileinfo.h"
#include <QDir>
#include <QList>
#include <QSharedDataPointer>

/// Provides ZIP archive navigation.
/**
* This class is modelled after QDir, and is designed to provide similar
* features for ZIP archives.
*
* The only significant difference from QDir is that the root path is not
* '/', but an empty string since that's how the file paths are stored in
* the archive. However, QuaZipDir understands the paths starting with
* '/'. It is important in a few places:
*
* - In the cd() function.
* - In the constructor.
* - In the exists() function.
*
* Note that since ZIP uses '/' on all platforms, the '\' separator is
* not supported.
*/
class QUAZIP_EXPORT QuaZipDir {
private:
    QSharedDataPointer<QuaZipDirPrivate> d;
public:
    /// The copy constructor.
    QuaZipDir(const QuaZipDir &that);
    /// Constructs a QuaZipDir instance pointing to the specified directory.
    /**
       If \a dir is not specified, points to the root of the archive.
       The same happens if the \a dir is &quot;/&quot;.
     */
    QuaZipDir(QuaZip *zip, const QString &dir = QString());
    /// Destructor.
    ~QuaZipDir();
    /// The assignment operator.
    bool operator==(const QuaZipDir &that);
    /// operator!=
    /**
      \return \c true if either this and \a that use different QuaZip
      instances or if they point to different directories.
      */
    inline bool operator!=(const QuaZipDir &that) {return !operator==(that);}
    /// operator==
    /**
      \return \c true if both this and \a that use the same QuaZip
      instance and point to the same directory.
      */
    QuaZipDir& operator=(const QuaZipDir &that);
    /// Returns the name of the entry at the specified position.
    QString operator[](int pos) const;
    /// Returns the current case sensitivity mode.
    QuaZip::CaseSensitivity caseSensitivity() const;
    /// Changes the 'current' directory.
    /**
      * If the path starts with '/', it is interpreted as an absolute
      * path from the root of the archive. Otherwise, it is interpreted
      * as a path relative to the current directory as was set by the
      * previous cd() or the constructor.
      * 
      * Note that the subsequent path() call will not return a path
      * starting with '/' in all cases.
      */
    bool cd(const QString &dirName);
    /// Goes up.
    bool cdUp();
    /// Returns the number of entries in the directory.
    uint count() const;
    /// Returns the current directory name.
    /**
      The name doesn't include the path.
      */
    QString dirName() const;
    /// Returns the list of the entries in the directory.
    /**
      \param nameFilters The list of file patterns to list, uses the same
      syntax as QDir.
      \param filters The entry type filters, only Files and Dirs are
      accepted.
      \param sort Sorting mode (not supported yet).
      */
    QList<QuaZipFileInfo> entryInfoList(const QStringList &nameFilters,
        QDir::Filters filters = QDir::NoFilter,
        QDir::SortFlags sort = QDir::NoSort) const;
    /// Returns the list of the entries in the directory.
    /**
      \overload

      The same as entryInfoList(QStringList(), filters, sort).
      */
    QList<QuaZipFileInfo> entryInfoList(QDir::Filters filters = QDir::NoFilter,
        QDir::SortFlags sort = QDir::NoSort) const;
    /// Returns the list of the entry names in the directory.
    /**
      The same as entryInfoList(nameFilters, filters, sort), but only
      returns entry names.
      */
    QStringList entryList(const QStringList &nameFilters,
        QDir::Filters filters = QDir::NoFilter,
        QDir::SortFlags sort = QDir::NoSort) const;
    /// Returns the list of the entry names in the directory.
    /**
      \overload

      The same as entryList(QStringList(), filters, sort).
      */
    QStringList entryList(QDir::Filters filters = QDir::NoFilter,
        QDir::SortFlags sort = QDir::NoSort) const;
    /// Returns \c true if the entry with the specified name exists.
    /**
      The &quot;..&quot; is considered to exist if the current directory
      is not root. The &quot;.&quot; and &quot;/&quot; are considered to
      always exist. Paths starting with &quot;/&quot; are relative to
      the archive root, other paths are relative to the current dir.
      */
    bool exists(const QString &fileName) const;
    /// Return \c true if the directory pointed by this QuaZipDir exists.
    bool exists() const;
    /// Returns the full path to the specified file.
    /**
      Doesn't check if the file actually exists.
      */
    QString filePath(const QString &fileName) const;
    /// Returns the default filter.
    QDir::Filters filter();
    /// Returns if the QuaZipDir points to the root of the archive.
    /**
      Not that the root path is the empty string, not '/'.
     */
    bool isRoot() const;
    /// Return the default name filter.
    QStringList nameFilters() const;
    /// Returns the path to the current dir.
    /**
      The path never starts with '/', and the root path is an empty
      string.
      */
    QString path() const;
    /// Returns the path to the specified file relative to the current dir.
    QString relativeFilePath(const QString &fileName) const;
    /// Sets the default case sensitivity mode.
    void setCaseSensitivity(QuaZip::CaseSensitivity caseSensitivity);
    /// Sets the default filter.
    void setFilter(QDir::Filters filters);
    /// Sets the default name filter.
    void setNameFilters(const QStringList &nameFilters);
    /// Goes to the specified path.
    /**
      The difference from cd() is that this function never checks if the
      path actually exists and doesn't use relative paths, so it's
      possible to go to the root directory with setPath(&quot;&quot;).

      Note that this function still chops the trailing and/or leading
      '/' and treats a single '/' as the root path (path() will still
      return an empty string).
      */
    void setPath(const QString &path);
    /// Sets the default sorting mode.
    void setSorting(QDir::SortFlags sort);
    /// Returns the default sorting mode.
    QDir::SortFlags sorting() const;
};

#endif // QUAZIP_QUAZIPDIR_H
