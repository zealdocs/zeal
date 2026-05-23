// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_SHORTCUTEDIT_H
#define ZEAL_WIDGETUI_SHORTCUTEDIT_H

#include <QKeyCombination>
#include <QLineEdit>

namespace Zeal::WidgetUi {

class ShortcutEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit ShortcutEdit(QWidget *parent = nullptr);
    explicit ShortcutEdit(const QString &text, QWidget *parent = nullptr);

    bool event(QEvent *event) override;

    QKeySequence keySequence() const;
    void setKeySequence(const QKeySequence &keySequence);

private:
    static Qt::KeyboardModifiers translateModifiers(Qt::KeyboardModifiers state, const QString &text);

    QKeyCombination m_key;
    QKeyCombination m_originalKey;
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_SHORTCUTEDIT_H
