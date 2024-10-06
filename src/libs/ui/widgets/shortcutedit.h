/****************************************************************************
**
** Copyright (C) Oleg Shparber, et al.
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: https://go.zealdocs.org/l/contact
**
** This file is part of Zeal.
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
****************************************************************************/

#ifndef ZEAL_WIDGETUI_SHORTCUTEDIT_H
#define ZEAL_WIDGETUI_SHORTCUTEDIT_H

#include <QLineEdit>

namespace Zeal {
namespace WidgetUi {

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
    int translateModifiers(Qt::KeyboardModifiers state, const QString &text);

    int m_key = 0;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_SHORTCUTEDIT_H
