// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_CORE_SESSION_H
#define ZEAL_CORE_SESSION_H

#include <QByteArray>
#include <QList>
#include <QString>

namespace Zeal::Core {

struct WindowState final
{
    QByteArray geometry;
    QByteArray splitterState;
    QByteArray tocSplitterState;
};

struct Session final
{
    QList<WindowState> windows;

    // Returns a reference to the first window's state, creating an empty entry
    // if the list is empty. Until multi-window support lands, all callers go
    // through this accessor.
    WindowState &primaryWindow();

    void load();
    bool save() const;

    void loadFromFile(const QString &path);
    bool saveToFile(const QString &path) const;

    static QString defaultFilePath();
};

} // namespace Zeal::Core

#endif // ZEAL_CORE_SESSION_H
