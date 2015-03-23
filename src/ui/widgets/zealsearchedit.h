#ifndef SEARCHEDIT_H
#define SEARCHEDIT_H

#include <QLineEdit>

class QCompleter;
class QEvent;
class QLabel;
class QTreeView;

class ZealSearchEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit ZealSearchEdit(QWidget *parent = nullptr);

    void setTreeView(QTreeView *view);
    void clearQuery();
    void selectQuery();
    void setCompletions(const QStringList &completions);

public slots:
    void clear();
    QString currentCompletion(const QString &text) const;
    void showCompletions(const QString &text);

protected:
    bool event(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    int queryStart() const;

    QCompleter *m_prefixCompleter = nullptr;
    QTreeView *m_treeView = nullptr;
    QLabel *m_completionLabel = nullptr;
    bool m_focusing = false;
};

#endif // SEARCHEDIT_H
