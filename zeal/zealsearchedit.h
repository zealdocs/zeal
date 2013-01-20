#ifndef ZEALSEARCHEDIT_H
#define ZEALSEARCHEDIT_H

#include <QEvent>
#include <QTreeView>
#include <QLineEdit>

class ZealSearchEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit ZealSearchEdit(QWidget *parent = 0);
    void setTreeView(QTreeView *view);

protected:
    bool eventFilter(QObject *obj, QEvent *ev);

signals:
    
public slots:

private:
    QTreeView *treeView;
};

#endif // ZEALSEARCHEDIT_H
