#ifndef TOOLBARFRAME_H
#define TOOLBARFRAME_H

#include <QWidget>

class ToolBarFrame : public QWidget
{
    Q_OBJECT
public:
    explicit ToolBarFrame(QWidget *parent = nullptr);

private:
    void paintEvent(QPaintEvent *event) override;
};

#endif // TOOLBARFRAME_H
