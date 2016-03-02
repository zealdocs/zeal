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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "registry/docsetmetadata.h"

#include <QDialog>
#include <QHash>
#include <QMap>

class QAbstractButton;
class QListWidgetItem;
class QNetworkReply;
class QTemporaryFile;
class QUrl;

namespace Ui {
class SettingsDialog;
}

namespace Zeal {

class DocsetRegistry;

namespace Core {
class Application;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(Core::Application *app, QWidget *parent = nullptr);
    ~SettingsDialog() override;

private slots:
    void addDashFeed();
    void updateSelectedDocsets();
    void updateAllDocsets();
    void removeSelectedDocsets();
    void updateDocsetFilter(const QString &filterString);

    void downloadCompleted();
    void downloadProgress(qint64 received, qint64 total);

    void extractionCompleted(const QString &filePath);
    void extractionError(const QString &filePath, const QString &errorString);
    void extractionProgress(const QString &filePath, qint64 extracted, qint64 total);

    void on_downloadDocsetButton_clicked();
    void on_storageButton_clicked();
    void on_tabWidget_currentChanged(int current);

private:
    enum DownloadType {
        DownloadDashFeed,
        DownloadDocset,
        DownloadDocsetList
    };

    Ui::SettingsDialog *ui = nullptr;
    Core::Application *m_application = nullptr;
    DocsetRegistry *m_docsetRegistry = nullptr;

    QList<QNetworkReply *> m_replies;
    qint64 m_combinedTotal = 0;
    qint64 m_combinedReceived = 0;

    /// TODO: Create a special model
    QMap<QString, DocsetMetadata> m_availableDocsets;
    QMap<QString, DocsetMetadata> m_userFeeds;

    QHash<QString, QTemporaryFile *> m_tmpFiles;

    QListWidgetItem *findDocsetListItem(const QString &title) const;
    bool updatesAvailable() const;

    QNetworkReply *download(const QUrl &url);
    void cancelDownloads();

    void downloadDocsetList();
    void processDocsetList(const QJsonArray &list);

    void downloadDashDocset(const QString &name);
    void removeDocsets(const QStringList &names);

    void displayProgress();
    void resetProgress();

    void loadSettings();
    void saveSettings();
    static inline int percent(qint64 fraction, qint64 total);

    static QString cacheLocation(const QString &fileName);
};

} // namespace Zeal

#endif // SETTINGSDIALOG_H
