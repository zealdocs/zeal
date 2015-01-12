#ifndef ZEALSEARCHEDIT_H
#define ZEALSEARCHEDIT_H

#include <QEvent>
#include <QTreeView>
#include <QCompleter>
#include <QLabel>
#include <QLineEdit>

class ZealSearchEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit ZealSearchEdit(QWidget *parent = 0);
    void setTreeView(QTreeView *view);
    void clearQuery();
    void selectQuery();
    void setCompletions(const QStringList &completions);

protected:
    bool eventFilter(QObject *obj, QEvent *ev);
    void focusInEvent(QFocusEvent *);
    void keyPressEvent(QKeyEvent *keyEvent);
    void mousePressEvent(QMouseEvent *ev);
    void resizeEvent(QResizeEvent *ev);

signals:

public slots:
    void clear();
    QString currentCompletion(const QString &text);
    void showCompletions(const QString &text);

private:
    int queryStart();

    QCompleter *prefixCompleter;
    QTreeView *treeView;
    QLabel *completionLabel;
    bool focusing;
};

#endif // ZEALSEARCHEDIT_H
