#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>

class QLocalServer;

class MainWindow;

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

private:
    QLocalServer *m_localServer = nullptr;
    Settings *m_settings = nullptr;
    MainWindow *m_mainWindow = nullptr;
};

} // namespace Core
} // namespace Zeal

#endif // APPLICATION_H
