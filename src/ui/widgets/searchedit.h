/****************************************************************************
**
** Copyright (C) 2015 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: http://zealdocs.org/contact.html
**
** This file is part of Zeal.
**
** Zeal is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** Zeal is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Zeal. If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#ifndef SEARCHEDIT_H
#define SEARCHEDIT_H

#include <memory>
#include <QLineEdit>

class QCompleter;
class QEvent;
class QLabel;
class QResizeEvent;
class QTreeView;

class SearchEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit SearchEdit(QWidget *parent = nullptr);
    ~SearchEdit();

    void setTreeView(QTreeView *view);
    void clearQuery();
    void selectQuery();
    void setCompletions(const QStringList &completions);

signals:
    void focusIn();
    void focusOut();

protected:
    bool event(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void showCompletions(const QString &text);

private:
    QString currentCompletion(const QString &text) const;
    int queryStart() const;
    void displaySearchIcon();

    std::unique_ptr<QCompleter> m_prefixCompleter;
    QTreeView *m_treeView = nullptr;
    std::unique_ptr<QLabel> m_completionLabel;
    std::unique_ptr<QLabel> m_searchLabel;
    bool m_focusing = false;
};

#endif // SEARCHEDIT_H
