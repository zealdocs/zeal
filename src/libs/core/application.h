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

#ifndef ZEAL_CORE_APPLICATION_H
#define ZEAL_CORE_APPLICATION_H

#include <QObject>
#include <QVersionNumber>

class QNetworkAccessManager;
class QNetworkReply;
class QThread;

namespace Zeal {

namespace Registry {
class DocsetRegistry;
class SearchQuery;
} // namespace Registry

namespace WidgetUi {
class MainWindow;
} // namespace WidgetUi

namespace Core {

class Extractor;
class FileManager;
class HttpServer;
class Settings;

class Application final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Application)
public:
    explicit Application(QObject *parent = nullptr);
    ~Application() override;

    static Application *instance();

    WidgetUi::MainWindow *mainWindow() const;

    QNetworkAccessManager *networkManager() const;
    Settings *settings() const;

    Registry::DocsetRegistry *docsetRegistry();
    FileManager *fileManager() const;
    HttpServer *httpServer() const;

    static QString cacheLocation();
    static QString configLocation();
    static QVersionNumber version();
    static QString versionString();

public slots:
    void executeQuery(const Registry::SearchQuery &query, bool preventActivation);
    void extract(const QString &filePath, const QString &destination, const QString &root = QString());
    QNetworkReply *download(const QUrl &url);
    void checkForUpdates(bool quiet = false);

signals:
    void extractionCompleted(const QString &filePath);
    void extractionError(const QString &filePath, const QString &errorString);
    void extractionProgress(const QString &filePath, qint64 extracted, qint64 total);
    void updateCheckDone(const QString &version = QString());
    void updateCheckError(const QString &message);

private slots:
    void applySettings();

private:
    static inline QString userAgent();
    QString userAgentJson() const;

    static Application *m_instance;

    Settings *m_settings = nullptr;

    QNetworkAccessManager *m_networkManager = nullptr;

    FileManager *m_fileManager = nullptr;
    HttpServer *m_httpServer = nullptr;

    QThread *m_extractorThread = nullptr;
    Extractor *m_extractor = nullptr;

    Registry::DocsetRegistry *m_docsetRegistry = nullptr;

    WidgetUi::MainWindow *m_mainWindow = nullptr;
};

} // namespace Core
} // namespace Zeal

#endif // ZEAL_CORE_APPLICATION_H
