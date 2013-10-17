#ifndef ZEALSEARCHEDIT_H
#define ZEALSEARCHEDIT_H

#include <QEvent>
#include <QTreeView>
#include "../lineedit.h"

class ZealSearchEdit : public LineEdit
{
    Q_OBJECT
public:
    explicit ZealSearchEdit(QWidget *parent = 0);
    void setTreeView(QTreeView *view);
    void clearQuery();

protected:
    bool eventFilter(QObject *obj, QEvent *ev);
    void focusInEvent(QFocusEvent *);

signals:

public slots:

private:
    QTreeView *treeView;
};

#endif // ZEALSEARCHEDIT_H
