// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_EMPTYSTATELABEL_H
#define ZEAL_WIDGETUI_EMPTYSTATELABEL_H

#include <QLabel>

class QAbstractScrollArea;

namespace Zeal::WidgetUi {

class EmptyStateLabel final : public QLabel
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(EmptyStateLabel)
public:
    explicit EmptyStateLabel(QAbstractScrollArea *view, const QString &text);
    ~EmptyStateLabel() override = default;

    bool eventFilter(QObject *object, QEvent *event) override;

    void setEmpty(bool empty);

private:
    void updateBounds();
};

} // namespace Zeal::WidgetUi

#endif // ZEAL_WIDGETUI_EMPTYSTATELABEL_H
