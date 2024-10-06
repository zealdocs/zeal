// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_SEARCHEDIT_H
#define ZEAL_WIDGETUI_SEARCHEDIT_H

#include <QLineEdit>

class QCompleter;
class QEvent;
class QLabel;

namespace Zeal {
namespace WidgetUi {

class SearchEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit SearchEdit(QWidget *parent = nullptr);

    void clearQuery();
    void selectQuery();
    void setCompletions(const QStringList &completions);

protected:
    bool event(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;

private slots:
    void showCompletions(const QString &text);

private:
    QString currentCompletion(const QString &text) const;
    int queryStart() const;

    QCompleter *m_prefixCompleter = nullptr;
    QLabel *m_completionLabel = nullptr;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_SEARCHEDIT_H
