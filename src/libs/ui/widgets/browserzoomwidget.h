#ifndef BROWSERZOOMWIDGET_H
#define BROWSERZOOMWIDGET_H

#include <QWidget>

class QPushButton;
class QLabel;

class BrowserZoomWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BrowserZoomWidget(QWidget *parent = nullptr);
    QPushButton *zoomOutButton();
    QPushButton *zoomInButton();
    QPushButton *resetZoomButton();
    QLabel *zoomLevelLabel();

private:
    QPushButton *m_zoomOutButton{nullptr};
    QPushButton *m_zoomInButton{nullptr};
    QPushButton *m_resetZoomButton{nullptr};
    QLabel *m_zoomLevelLabel{nullptr};
};

#endif // BROWSERZOOMWIDGETACTION_H
