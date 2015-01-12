#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>

class QLocalServer;

class MainWindow;

namespace Zeal {
namespace Core {

class Application : public QObject
{
    Q_OBJECT
public:
    explicit Application(const QString &query = QString(), QObject *parent = nullptr);
    ~Application() override;

    static QString localServerName();

signals:

public slots:

private:
    MainWindow *m_mainWindow;
    QLocalServer *m_localServer = nullptr;
};

} // namespace Core
} // namespace Zeal

#endif // APPLICATION_H
