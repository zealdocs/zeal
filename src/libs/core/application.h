// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

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
