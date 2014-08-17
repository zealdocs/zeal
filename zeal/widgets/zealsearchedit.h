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
    void setCompletions(QStringList completions);

protected:
    bool eventFilter(QObject *obj, QEvent *ev);
    void focusInEvent(QFocusEvent *);
    void mousePressEvent(QMouseEvent *ev);

signals:

public slots:
    void clear();

private:
    QTreeView *treeView;
    bool focusing;
};

#endif // ZEALSEARCHEDIT_H
