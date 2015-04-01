#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>

class QLocalServer;

class MainWindow;

class QNetworkAccessManager;
class QNetworkReply;
class QThread;

namespace Zeal {

class DocsetRegistry;
class SearchQuery;

namespace Core {

class Extractor;
class Settings;

class Application : public QObject
{
    Q_OBJECT
public:
    explicit Application(QObject *parent = nullptr);
    explicit Application(const SearchQuery &query, QObject *parent = nullptr);
    ~Application() override;

    static QString localServerName();

    QNetworkAccessManager *networkManager() const;
    Settings *settings() const;

    static DocsetRegistry *docsetRegistry();

public slots:
    void extract(const QString &filePath, const QString &destination, const QString &root = QString());
    QNetworkReply *download(const QUrl &url);
    void checkUpdate();

signals:
    void extractionCompleted(const QString &filePath);
    void extractionError(const QString &filePath, const QString &errorString);
    void extractionProgress(const QString &filePath, qint64 extracted, qint64 total);
    void updateCheckDone(const QString &version = QString());
    void updateCheckError(const QString &message);

private slots:
    void applySettings();

private:
    static Application *m_instance;

    Settings *m_settings = nullptr;

    QLocalServer *m_localServer = nullptr;
    QNetworkAccessManager *m_networkManager = nullptr;

    QThread *m_extractorThread = nullptr;
    Extractor *m_extractor = nullptr;

    DocsetRegistry *m_docsetRegistry = nullptr;

    MainWindow *m_mainWindow = nullptr;
};

} // namespace Core
} // namespace Zeal

#endif // APPLICATION_H
