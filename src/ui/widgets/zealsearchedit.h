#ifndef SEARCHEDIT_H
#define SEARCHEDIT_H

#include <QEvent>
#include <QTreeView>
#include <QCompleter>
#include <QLabel>
#include <QLineEdit>

class ZealSearchEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit ZealSearchEdit(QWidget *parent = nullptr);

    void setTreeView(QTreeView *view);
    void clearQuery();
    void selectQuery();
    void setCompletions(const QStringList &completions);

protected:
    bool event(QEvent *event) override;
    void focusInEvent(QFocusEvent *) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *ev) override;

public slots:
    void clear();
    QString currentCompletion(const QString &text);
    void showCompletions(const QString &text);

private:
    int queryStart() const;

    QCompleter *prefixCompleter;
    QTreeView *treeView;
    QLabel *completionLabel;
    bool focusing;
};

#endif // SEARCHEDIT_H
