#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>

class QLocalServer;

class MainWindow;

class QNetworkAccessManager;
class QNetworkReply;
class QThread;

namespace Zeal {
namespace Core {

class Extractor;
class Settings;

enum class QueryType {
    DASH,
    DASH_PLUGIN
};

class Application : public QObject
{
    Q_OBJECT
public:
    explicit Application(const QString &query = QString(), QObject *parent = nullptr);
    ~Application() override;

    static QString localServerName();

    QNetworkAccessManager *networkManager() const;
    Settings *settings() const;

public slots:
    void extract(const QString &filePath, const QString &destination);
    QNetworkReply *download(const QUrl &url);

signals:
    void extractionCompleted(const QString &filePath);
    void extractionError(const QString &filePath, const QString &errorString);

private slots:
    void applySettings();
    void socketConnected();

private:
    static QString processPluginQuery(QString url);
    static void associateProtocolHandler();

private:
    static Application *m_instance;

    Settings *m_settings = nullptr;

    QLocalServer *m_localServer = nullptr;
    QNetworkAccessManager *m_networkManager = nullptr;

    QThread *m_extractorThread = nullptr;
    Extractor *m_extractor = nullptr;

    MainWindow *m_mainWindow = nullptr;
};

} // namespace Core
} // namespace Zeal

#endif // APPLICATION_H
