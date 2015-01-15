#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>

class QLocalServer;

class MainWindow;

class QNetworkAccessManager;
class QNetworkReply;

namespace Zeal {
namespace Core {

class Settings;

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
    QNetworkReply *download(const QUrl &url);

private slots:
    void applySettings();

private:
    static Application *m_instance;

    QLocalServer *m_localServer = nullptr;
    Settings *m_settings = nullptr;
    QNetworkAccessManager *m_networkManager = nullptr;
    MainWindow *m_mainWindow = nullptr;
};

} // namespace Core
} // namespace Zeal

#endif // APPLICATION_H
