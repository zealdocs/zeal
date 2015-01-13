#ifndef SHORTCUTEDIT_H
#define SHORTCUTEDIT_H

#include <QLineEdit>

class ShortcutEdit : public QLineEdit
{
public:
    explicit ShortcutEdit(QWidget *parent = nullptr);
    explicit ShortcutEdit(const QString &text, QWidget *parent = nullptr);

    bool event(QEvent *event) override;

    QKeySequence keySequence() const;
    void setKeySequence(const QKeySequence &keySequence);

private:
    int translateModifiers(Qt::KeyboardModifiers state, const QString &text);

    int m_key = 0;
};

#endif // SHORTCUTEDIT_H
