// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDir>

namespace Zeal::Core::Detail {

inline QString expandTildePath(const QString &path, const QString &homePath)
{
#if defined(Q_OS_UNIX)
    if (homePath.isEmpty()) {
        return path;
    }
    if (path == QStringLiteral("~")) {
        return homePath;
    }
    if (path.startsWith(QStringLiteral("~/"))) {
        QString relativePath = path.mid(2);
        while (relativePath.startsWith(QLatin1Char('/'))) {
            relativePath.remove(0, 1);
        }
        return QDir(homePath).filePath(relativePath);
    }
#else
    Q_UNUSED(homePath);
#endif
    return path;
}

} // namespace Zeal::Core::Detail
