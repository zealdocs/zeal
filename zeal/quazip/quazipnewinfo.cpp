/*
Copyright (C) 2005-2011 Sergey A. Tachenov

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

See COPYING file for the full LGPL text.

Original ZIP package is copyrighted by Gilles Vollant, see
quazip/(un)zip.h files for details, basically it's zlib license.
*/

#include <QFileInfo>

#include "quazipnewinfo.h"

static void QuaZipNewInfo_setPermissions(QuaZipNewInfo *info,
        QFile::Permissions perm, bool isDir)
{
    quint32 uPerm = isDir ? 0040000 : 0100000;
    if ((perm & QFile::ReadOwner) != 0)
        uPerm |= 0400;
    if ((perm & QFile::WriteOwner) != 0)
        uPerm |= 0200;
    if ((perm & QFile::ExeOwner) != 0)
        uPerm |= 0100;
    if ((perm & QFile::ReadGroup) != 0)
        uPerm |= 0040;
    if ((perm & QFile::WriteGroup) != 0)
        uPerm |= 0020;
    if ((perm & QFile::ExeGroup) != 0)
        uPerm |= 0010;
    if ((perm & QFile::ReadOther) != 0)
        uPerm |= 0004;
    if ((perm & QFile::WriteOther) != 0)
        uPerm |= 0002;
    if ((perm & QFile::ExeOther) != 0)
        uPerm |= 0001;
    info->externalAttr = (info->externalAttr & ~0xFFFF0000u) | (uPerm << 16);
}

QuaZipNewInfo::QuaZipNewInfo(const QString& name):
  name(name), dateTime(QDateTime::currentDateTime()), internalAttr(0), externalAttr(0)
{
}

QuaZipNewInfo::QuaZipNewInfo(const QString& name, const QString& file):
  name(name), internalAttr(0), externalAttr(0)
{
  QFileInfo info(file);
  QDateTime lm = info.lastModified();
  if (!info.exists()) {
    dateTime = QDateTime::currentDateTime();
  } else {
    dateTime = lm;
    QuaZipNewInfo_setPermissions(this, info.permissions(), info.isDir());
  }
}

void QuaZipNewInfo::setFileDateTime(const QString& file)
{
  QFileInfo info(file);
  QDateTime lm = info.lastModified();
  if (info.exists())
    dateTime = lm;
}

void QuaZipNewInfo::setFilePermissions(const QString &file)
{
    QFileInfo info = QFileInfo(file);
    QFile::Permissions perm = info.permissions();
    QuaZipNewInfo_setPermissions(this, perm, info.isDir());
}

void QuaZipNewInfo::setPermissions(QFile::Permissions permissions)
{
    QuaZipNewInfo_setPermissions(this, permissions, name.endsWith('/'));
}
