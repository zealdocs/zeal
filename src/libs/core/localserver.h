/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef ZEAL_CORE_LOCALSERVER_H
#define ZEAL_CORE_LOCALSERVER_H

#include <QObject>

class QLocalServer;

namespace Zeal {

namespace Registry {
class SearchQuery;
} // namespace Registry

namespace Core {

class LocalServer : public QObject
{
    Q_OBJECT
public:
    explicit LocalServer(QObject *parent = 0);

    QString errorString() const;

    bool start(bool force = false);

    static bool sendQuery(const Registry::SearchQuery &query, bool preventActivation);

signals:
    void newQuery(const Registry::SearchQuery &query, bool preventActivation);

private:
    QLocalServer *m_localServer = nullptr;
};

} // namespace Core
} // namespace Zeal

#endif // ZEAL_CORE_LOCALSERVER_H
