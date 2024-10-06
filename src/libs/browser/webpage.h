// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2019 Kay Gawlik
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_BROWSER_WEBPAGE_H
#define ZEAL_BROWSER_WEBPAGE_H

#include <QWebEnginePage>

namespace Zeal {
namespace Browser {

class WebPage final : public QWebEnginePage
{
    Q_OBJECT
    Q_DISABLE_COPY(WebPage)
public:
    explicit WebPage(QObject *parent = nullptr);

protected:
    bool acceptNavigationRequest(const QUrl &requestUrl, NavigationType type, bool isMainFrame) override;
    void javaScriptConsoleMessage(QWebEnginePage::JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceId) override;
};

} // namespace Browser
} // namespace Zeal

#endif // ZEAL_BROWSER_WEBPAGE_H
