#include "zealsettingsdialog.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QWebView>
#include <QWebSettings>
#include <QWebFrame>
#include <QWebElementCollection>
#include <QDir>
#include <QUrl>
#include <QTemporaryFile>
#include <QProcess>
#include <QMessageBox>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QFileDialog>
#include <QtNetwork/QNetworkProxy>
#include "quazip/quazip.h"
#include "JlCompress.h"
#include "progressitemdelegate.h"

#include <QDebug>

ZealSettingsDialog::ZealSettingsDialog(ZealListModel &zList, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ZealSettingsDialog),
    zealList( zList ),
    settings("Zeal", "Zeal")
{
    ui->setupUi(this);

    ui->downloadableGroup->hide();
    ui->docsetsProgress->hide();

    ui->listView->setModel( &zealList );


    ProgressItemDelegate *progressDelegate = new ProgressItemDelegate();
    ui->docsetsList->setItemDelegate( progressDelegate );
    ui->listView->setItemDelegate(progressDelegate);

    tasksRunning = 0;
    totalDownload = 0;
    currentDownload = 0;
    downloadedDocsetsList = false;
    loadSettings();
}

ZealSettingsDialog::~ZealSettingsDialog()
{
    delete ui;
}

void ZealSettingsDialog::setHotKey(const QKeySequence &keySequence)
{
    ui->toolButton->setKeySequence(keySequence);
}

QKeySequence ZealSettingsDialog::hotKey()
{
    return ui->toolButton->keySequence();
}

void ZealSettingsDialog::loadSettings(){
    ui->minFontSize->setValue(settings.value("minFontSize").toInt());
    QString hiding = settings.value("hidingBehavior", "systray").toString();
    if(hiding == "systray") {
        ui->radioSysTray->setChecked(true);
    } else {
        ui->radioMinimize->setChecked(true);
    }
    QString startup = settings.value("startupBehavior", "window").toString();
    if(startup == "systray") {
        ui->radioStartTray->setChecked(true);
    } else {
        ui->radioStartMax->setChecked(true);
    }
    ui->storageEdit->setText(docsets->docsetsDir());

    ui->m_noProxySettings->setChecked(false);
    ui->m_systemProxySettings->setChecked(false);
    ui->m_manualProxySettings->setChecked(false);

    ZealSettingsDialog::ProxyType proxyType = ZealSettingsDialog::NoProxy;
    const QVariant variant = settings.value("proxyType", ZealSettingsDialog::NoProxy);
    if (variant.canConvert<ZealSettingsDialog::ProxyType>()) {
        proxyType = variant.value<ZealSettingsDialog::ProxyType>();
    }

    QString httpProxy = settings.value("httpProxy").toString();
    quint16 httpProxyPort = settings.value("httpProxyPort", 0).toInt();
    QString httpProxyUser = settings.value("httpProxyUser").toString();
    QString httpProxyPass = settings.value("httpProxyPass").toString();

    if (proxyType == ZealSettingsDialog::NoProxy) {
        ui->m_noProxySettings->setChecked(true);
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    } else if (proxyType == ZealSettingsDialog::SystemProxy) {
        ui->m_systemProxySettings->setChecked(true);
        QNetworkProxyFactory::setUseSystemConfiguration(true);
    } else {
        ui->m_manualProxySettings->setChecked(true);
        ui->m_httpProxy->setText(httpProxy);
        ui->m_httpProxyPort->setValue(httpProxyPort);
        ui->m_httpProxyUser->setText(httpProxyUser);
        ui->m_httpProxyPass->setText(httpProxyPass);
        ui->m_httpProxyNeedsAuth->setChecked(!httpProxyUser.isEmpty() | !httpProxyPass.isEmpty());
        QNetworkProxy::setApplicationProxy(this->httpProxy());
    }
}

// creates a total download progress for multiple QNetworkReplies
void ZealSettingsDialog::on_downloadProgress(quint64 recv, quint64 total){
    if(recv > 10240) { // don't show progress for non-docset pages (like Google Drive first request)
        QNetworkReply *reply = (QNetworkReply*) sender();
        // Try to get the item associated to the request
        QVariant itemId = reply->property("listItem");
        QListWidgetItem *item = ui->docsetsList->item(itemId.toInt());
        QPair<qint32, qint32> *previousProgress = progress[reply];
        if (previousProgress == nullptr) {
            previousProgress = new QPair<qint32, qint32>(0, 0);
            progress[reply] = previousProgress;
        }

        if( item != NULL ){
            item->setData( ProgressItemDelegate::ProgressMaxRole, total );
            item->setData( ProgressItemDelegate::ProgressRole, recv );
        }
        currentDownload += recv - previousProgress->first;
        totalDownload += total - previousProgress->second;
        previousProgress->first = recv;
        previousProgress->second = total;
        displayProgress();
    }
}

void ZealSettingsDialog::displayProgress()
{
    ui->docsetsProgress->setValue(currentDownload);
    ui->docsetsProgress->setMaximum(totalDownload);
    ui->docsetsProgress->setVisible(tasksRunning > 0);
}

void ZealSettingsDialog::startTasks(qint8 tasks = 1)
{
    tasksRunning += tasks;
    if (tasksRunning == 0) {
        resetProgress();
    }

    displayProgress();
}

void ZealSettingsDialog::endTasks(qint8 tasks = 1)
{
    startTasks(-tasks);

   if( tasksRunning <= 0){
       // Remove completed items
       for(int i=ui->docsetsList->count()-1;i>=0;--i){
           QListWidgetItem *tmp = ui->docsetsList->item(i);
           if(tmp->data(ZealDocsetDoneInstalling).toBool() ){
               tmp->setCheckState( Qt::Unchecked );
               tmp->setHidden( true );
           }
       }
   }

}

void ZealSettingsDialog::updateDocsets()
{
    ui->downloadableGroup->show();
    QStringList docsetNames = docsets->names();
    bool missingMetadata = false;
    foreach(auto name, docsetNames){
        ZealDocsetMetadata metadata = docsets->meta(name);
        if(!metadata.isValid()){
            missingMetadata = true;
        }

        QString feedUrl = metadata.getFeedURL();
        if(!feedUrl.isEmpty()){
            auto reply = startDownload(feedUrl);

            QList<QListWidgetItem*> items = ui->docsetsList->findItems( QString(name), Qt::MatchFixedString);
            if(items.count() > 0){
                reply->setProperty("listItem", ui->docsetsList->row( items[0] ));
            } else {
                QListWidgetItem *item = new QListWidgetItem( name, ui->docsetsList );
                item->setCheckState( Qt::Checked );
                item->setHidden(true);
                ui->docsetsList->addItem( item );
                reply->setProperty("listItem", ui->docsetsList->row( item ));
            }

            reply->setProperty("metadata", QVariant::fromValue(metadata));
            connect(reply, SIGNAL(finished()), SLOT(extractDocset()));
        }
    }
    if(missingMetadata){
        int r = QMessageBox::information(this, "Zeal", "Some docsets are missing metadata, would you like to redownload all docsets with missing metadata?",
                                         QMessageBox::Yes | QMessageBox::No);
        if(r == QMessageBox::Yes){
            if(!downloadedDocsetsList){
                downloadDocsetLists();
            }
            // There must be a better way to do this.
            auto future = QtConcurrent::run([=]{
                while(!downloadedDocsetsList || replies.size()){
                    QThread::yieldCurrentThread();
                }
            });
            QFutureWatcher<void> *watcher = new QFutureWatcher<void>;
            watcher->setFuture(future);
            connect( watcher, &QFutureWatcher<void>::finished, [=]{
                foreach(auto name, docsetNames){
                    ZealDocsetMetadata metadata = docsets->meta(name);
                    if(!metadata.isValid() && availMetadata.contains(name)){
                        auto reply = startDownload(availMetadata[name]);

                        QList<QListWidgetItem*> items = ui->docsetsList->findItems( QString(name), Qt::MatchFixedString);
                        if(items.count() > 0){
                            items[0]->setCheckState( Qt::Checked );
                            items[0]->setHidden(false);
                            reply->setProperty("listItem", ui->docsetsList->row( items[0] ));
                        } else {
                            QListWidgetItem *item = new QListWidgetItem( name, ui->docsetsList );
                            item->setCheckState( Qt::Checked );
                            ui->docsetsList->addItem( item );
                            reply->setProperty("listItem", ui->docsetsList->row( item ));
                        }

                        connect(reply, SIGNAL(finished()), SLOT(extractDocset()));
                    }
                }
            });
        }

    }
}

void ZealSettingsDialog::downloadDocsetList()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());

    qint8 remainingRetries = replies.take(reply);
    if (reply->error() != QNetworkReply::NoError) {
        endTasks();
        if (reply->request().url().host() == "raw.github.com") {
            // allow github to fail
            // downloadedDocsetsList will be set either here, or in "if(replies.isEmpty())" below
            downloadedDocsetsList = ui->docsetsList->count() > 0;
            return;
        }
        if (reply->error() != QNetworkReply::OperationCanceledError) {
            QMessageBox::warning(this, "No docsets found", "Failed retrieving list of docsets: " + reply->errorString());
        }
        return;
    }

    if(reply->request().url().host() == "kapeli.com") {
        QWebView view;
        view.settings()->setAttribute(QWebSettings::JavascriptEnabled, false);
        view.setContent(reply->readAll());
        auto collection = view.page()->mainFrame()->findAllElements(".drowx");
        for(auto drowx : collection) {
            auto anchor = drowx.findFirst("a");
            auto url = anchor.attribute("href");
            auto name_list = url.split("/");
            auto name = name_list[name_list.count()-1].replace(".tgz", "");
            name = name.replace(".tar.bz2", "");
            if(name != "") {
                auto feedUrl = url;
                if( url.contains("feeds") ){ // TODO: There must be a better way to do this, or a valid list of available docset feeds.
                    feedUrl = url.section("/",0,-2) + "/" + name + ".xml"; // Attempt to generate a docset feed url.
                }
                //urls[name] = feedUrl;
                ZealDocsetMetadata meta;
                meta.setFeedURL(feedUrl);
                availMetadata[name] = meta;
                auto url_list = url.split("/");
                auto iconfile = url_list[url_list.count()-1].replace(".tgz", ".png");
                iconfile = iconfile.replace(".tar.bz2", ".png");
#ifdef WIN32
                QDir icondir(QCoreApplication::applicationDirPath());
                icondir.cd("icons");
#else
                QDir icondir("/usr/share/pixmaps/zeal");
#endif
                auto *lwi = new QListWidgetItem(QIcon(icondir.filePath(iconfile)), name);
                lwi->setCheckState(Qt::Unchecked);

                if(docsets->names().contains(name)){
                    ui->docsetsList->insertItem(0, lwi);
                    lwi->setHidden( true );
                } else {
                    ui->docsetsList->addItem(lwi);
                }
            }
        }
        if(availMetadata.size() > 0) {
            ui->downloadableGroup->show();
        }
    } else {
        QString list = reply->readAll();
        for(auto item : list.split("\n")) {
            QStringList docset = item.split(" ");
            if(docset.size() < 2) break;
            ZealDocsetMetadata meta;
            meta.addUrl(docset[1]);
            meta.setVersion(docset[2]);
            availMetadata[docset[0]] = meta;
            if(!docset[1].startsWith("http")) {
                availMetadata.clear();
                QMessageBox::warning(this, "No docsets found", "Failed retrieving https://raw.github.com/jkozera/zeal/master/docsets.txt: " + QString(docset[1]));
                break;
            }
            auto *lwi = new QListWidgetItem( docset[0], ui->docsetsList );
            lwi->setCheckState( Qt::Unchecked );
            if(docsets->names().contains(docset[0])){
                ui->docsetsList->insertItem(0, lwi);
                lwi->setHidden( true );
            } else {
                ui->docsetsList->addItem(lwi);
            }
        }
        if(availMetadata.size() > 0) {
            ui->downloadableGroup->show();
        } else {
            QMessageBox::warning(this, "No docsets found", QString("No downloadable docsets found."));
        }
    }

    endTasks();

    // if all enqueued downloads have finished executing
    if(replies.isEmpty()) {
        downloadedDocsetsList = ui->docsetsList->count() > 0;
        resetProgress();
    }

    reply->deleteLater();
}

void ZealSettingsDialog::extractDocset()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    qint8 remainingRetries = replies.take(reply);
    if (reply->error() != QNetworkReply::NoError) {
        endTasks();
        if (reply->request().url().host() == "raw.github.com") {
            // allow github to fail
            return;
        }
        if (reply->error() != QNetworkReply::OperationCanceledError) {
            QMessageBox::warning(this, "Network Error", "Failed to retrieve docset: " + reply->errorString());
        }
        return;
    }

    // Try to get the item associated to the request
    QVariant itemId = reply->property("listItem");
    QListWidgetItem *listItem = ui->docsetsList->item(itemId.toInt());
    if(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 302) {
        QUrl url(reply->rawHeader("Location"));
        if(url.host() == "") url.setHost(reply->request().url().host());
        if(url.scheme() == "") url.setScheme(reply->request().url().scheme());
        auto reply3 = startDownload(url, 1);
        endTasks();

        reply3->setProperty("listItem", itemId);
        reply3->setProperty("metadata", reply->property("metadata"));
        connect(reply3, SIGNAL(finished()), SLOT(extractDocset()));
    } else {
        if(reply->request().url().path().endsWith("xml")){
            endTasks();
            QXmlStreamReader feed(reply->readAll() );
            ZealDocsetMetadata metadata;
            ZealDocsetMetadata oldMetadata;
            metadata.read(feed);

            if(!metadata.getNumUrls()){
                QMessageBox::critical(this, "Zeal", "Could not read docset feed!");
            } else {

                QVariant oldMeta = reply->property("metadata");
                if(oldMeta.isValid()){
                    oldMetadata = oldMeta.value<ZealDocsetMetadata>();
                }

                if(metadata.getVersion().isEmpty() || oldMetadata.getVersion() != metadata.getVersion()){
                    metadata.setFeedURL(reply->request().url().toString());
                    auto reply2 = startDownload(metadata.getPrimaryUrl(), 1);

                    if(listItem != NULL){
                        listItem->setHidden(false);

                        listItem->setData( ProgressItemDelegate::ProgressVisibleRole, true);
                        listItem->setData( ProgressItemDelegate::ProgressRole, 0);
                        listItem->setData( ProgressItemDelegate::ProgressMaxRole, 1);
                    }
                    reply2->setProperty("listItem", itemId);
                    reply2->setProperty("metadata", QVariant::fromValue(metadata));
                    connect(reply2, SIGNAL(finished()), SLOT(extractDocset()));
                }
            }
        } else if(reply->request().url().path().endsWith("tgz") || reply->request().url().path().endsWith("tar.bz2")) {
            auto dataDir = QDir(docsets->docsetsDir());
            if(!dataDir.exists()) {
                QMessageBox::critical(this, "No docsets directory found",
                                      QString("'%1' directory not found").arg(docsets->docsetsDir()));
                endTasks();
            } else {
#ifdef WIN32
                QDir tardir(QCoreApplication::applicationDirPath());
                const QString program = tardir.filePath("bsdtar.exe");
#else
                const QString program = "bsdtar";
#endif
                QTemporaryFile *tmp = new QTemporaryFile;
                tmp->open();
                tmp->write(reply->readAll());
                tmp->flush();

                QProcess *tar = new QProcess();
                tar->setWorkingDirectory(dataDir.absolutePath());
                QStringList args;
                if(reply->request().url().path().endsWith("tar.bz2")) {
                    args = QStringList("-jqtf");
                } else {
                    args = QStringList("-zqtf");
                }
                args.append(tmp->fileName());
                args.append("*docset");
                tar->start(program, args);

                // TODO: check if bsdtar exists earlier (on startup or
                // before archive downloading)
                if (!tar->waitForStarted()) {
                    QMessageBox::critical(this, "bsdtar executable not found",
                                          (QString("'%1' executable not found. It is required to allow extracting docsets. ") + QString(
                                               "Please install it if you want to extract docsets from within Zeal.")).arg(program));
                    endTasks();
                }
                tar->waitForFinished();
                auto line_buf = tar->readLine();
                auto outDir = QString::fromLocal8Bit(line_buf).split("/")[0];

                if(reply->request().url().path().endsWith("tar.bz2")) {
                    args = QStringList("-jxf");
                } else {
                    args = QStringList("-zxf");
                }
                args.append(tmp->fileName());

                ZealDocsetMetadata metadata;
                QVariant metavariant = reply->property("metadata");
                if(metavariant.isValid()){
                    metadata = metavariant.value<ZealDocsetMetadata>();
                }
                auto path = reply->request().url().path().split("/");

                connect(tar, (void (QProcess::*)(int,QProcess::ExitStatus))&QProcess::finished, [=](int a, QProcess::ExitStatus b) {
                    auto fileName = path[path.count()-1];
                    auto docsetName = fileName.replace(".tgz", ".docset");
                    docsetName = docsetName.replace(".tar.bz2", ".docset");

                    if(outDir != docsetName) {
                        QDir dataDir2(dataDir);
                        dataDir2.rename(outDir, docsetName);
                    }

                    // Write metadata about docset
                    metadata.write(dataDir.absoluteFilePath(docsetName)+"/meta.json");

                    // FIXME C&P (see "FIXME C&P" below)
                    QMetaObject::invokeMethod(docsets, "addDocset", Qt::BlockingQueuedConnection,
                                              Q_ARG(QString, dataDir.absoluteFilePath(docsetName)));
                    zealList.resetModulesCounts();
                    refreshRequested();
                    ui->listView->reset();
                    for(int i = 0; i < ui->docsetsList->count(); ++i) {
                        if(ui->docsetsList->item(i)->text()+".docset" == docsetName) {
                            listItem->setData(ZealDocsetDoneInstalling, true);
                            listItem->setData(ProgressItemDelegate::ProgressFormatRole, "Done");
                            listItem->setData(ProgressItemDelegate::ProgressRole, 1);
                            listItem->setData(ProgressItemDelegate::ProgressMaxRole, 1);
                            break;
                        }
                    }
                    endTasks();
                });
                if(listItem){
                    listItem->setData(ProgressItemDelegate::ProgressRole, 0);
                    listItem->setData(ProgressItemDelegate::ProgressMaxRole, 0);
                }
                tar->start(program, args);
            }
        } else {
            QTemporaryFile *tmp = new QTemporaryFile;
            tmp->open();
            tmp->write(reply->readAll());
            tmp->seek(0);
            QuaZip zipfile(tmp);
            if(zipfile.open(QuaZip::mdUnzip)) {
                tmp->close();
                auto dataDir = QDir(docsets->docsetsDir());
                if(!dataDir.exists()) {
                    QMessageBox::critical(this, "No docsets directory found",
                                          QString("'%1' directory not found").arg(docsets->docsetsDir()));
                    endTasks();
                } else {
                    QStringList *files = new QStringList;
                    if(listItem){
                        listItem->setData(ProgressItemDelegate::ProgressRole, 0);
                        listItem->setData(ProgressItemDelegate::ProgressMaxRole, 0);
                    }
                    auto future = QtConcurrent::run([=] {
                        *files = JlCompress::extractDir(tmp->fileName(), dataDir.absolutePath());
                        delete tmp;
                    });
                    QFutureWatcher<void> *watcher = new QFutureWatcher<void>;
                    watcher->setFuture(future);
                    ZealDocsetMetadata metadata;
                    QVariant metavariant = reply->property("metadata");
                    if(metavariant.isValid()){
                        metadata = metavariant.value<ZealDocsetMetadata>();
                    }
                    connect(watcher, &QFutureWatcher<void>::finished, [=] {
                        // extract finished - add docset
                        QDir next((*files)[0]), root = next;
                        delete files;
                        next.cdUp();
                        while(next.absolutePath() != dataDir.absolutePath()) {
                            root = next;
                            next.cdUp();
                        }
                        metadata.write(dataDir.absoluteFilePath(root.dirName())+"/meta.json");
                        // FIXME C&P (see "FIXME C&P" above)
                        QMetaObject::invokeMethod(docsets, "addDocset", Qt::BlockingQueuedConnection,
                                                  Q_ARG(QString, root.absolutePath()));
                        zealList.resetModulesCounts();
                        refreshRequested();
                        ui->listView->reset();
                        for(int i = 0; i < ui->docsetsList->count(); ++i) {
                            if(ui->docsetsList->item(i)->text() == root.dirName() ||
                                    ui->docsetsList->item(i)->text()+".docset" == root.dirName()) {
                                listItem->setData(ZealDocsetDoneInstalling, true);
                                listItem->setData(ProgressItemDelegate::ProgressFormatRole, "Done");
                                listItem->setData(ProgressItemDelegate::ProgressRole, 1);
                                listItem->setData(ProgressItemDelegate::ProgressMaxRole, 1);
                                break;
                            }
                        }
                        endTasks();
                    });
                }
            } else if(remainingRetries > 0) { // 3rd request - retry once for Google Drive
                QFile file(tmp->fileName());
                file.open(QIODevice::ReadOnly);
                QWebView view;
                view.setContent(file.readAll());
                tmp->close();
                delete tmp;

                QString href = view.page()->mainFrame()->findFirstElement("#uc-download-link").attribute("href");
                QString path = href.split("?")[0], query = href.split("?")[1];
                QUrl url = reply->url();
                url.setPath(path);
                url.setQuery(query);
                // retry with #uc-download-link - "Google Drive can't scan this file for viruses."
                auto reply2 = startDownload(url, remainingRetries - 1); //naManager.get(QNetworkRequest(url));
                endTasks();
                reply2->setProperty("listItem", itemId);
                reply2->setProperty("metadata", reply->property("metadata"));
                connect(reply2, SIGNAL(finished()), SLOT(extractDocset()));
            } else {
                tmp->close();
                delete tmp;
                QMessageBox::warning(this, "Error", "Download failed: invalid ZIP file.");
                endTasks();
            }
        }
    }

    // if all enqueued downloads have finished executing
    if(replies.isEmpty()) {
        resetProgress();
    }

    reply->deleteLater();
}

void ZealSettingsDialog::downloadDocsetLists(){
   downloadedDocsetsList = false;
   ui->downloadButton->hide();
   ui->docsetsList->clear();
   auto reply = startDownload(QUrl("https://raw.github.com/jkozera/zeal/master/docsets.txt"));
   auto reply2 = startDownload(QUrl("http://kapeli.com/docset_links"));

   connect(reply, SIGNAL(finished()), SLOT(downloadDocsetList()));
   connect(reply2, SIGNAL(finished()), SLOT(downloadDocsetList()));
}

void ZealSettingsDialog::on_downloadButton_clicked()
{
    downloadDocsetLists();
}

void ZealSettingsDialog::on_docsetsList_itemSelectionChanged()
{
    ui->downloadDocsetButton->setEnabled(ui->docsetsList->selectedItems().count() > 0);
}

void ZealSettingsDialog::on_downloadDocsetButton_clicked()
{
    if (replies.count() > 0) {
        stopDownloads();
        return;
    }

    // Find each checked item, and create a NetworkRequest for it.
    for(int i=0;i<ui->docsetsList->count();++i){
        QListWidgetItem *tmp = ui->docsetsList->item(i);
        if(tmp->checkState() == Qt::Checked){

            //QUrl url(urls[tmp->text()]);

            auto reply = startDownload(availMetadata[tmp->text()], 1);
            QUrl url = reply->url();
            reply->setProperty("listItem", i);
            connect(reply, SIGNAL(finished()), SLOT(extractDocset()));

            tmp->setData( ProgressItemDelegate::ProgressVisibleRole, true);
            tmp->setData( ProgressItemDelegate::ProgressRole, 0);
            tmp->setData( ProgressItemDelegate::ProgressMaxRole, 1);
            if(url.path().endsWith((".tgz")) || url.path().endsWith((".tar.bz2"))) {
                // Dash's docsets don't redirect, so we can start showing progress instantly
                connect(reply, &QNetworkReply::downloadProgress, this, &ZealSettingsDialog::on_downloadProgress);
            }
        }
    }

    if( replies.count() > 0 ){
        ui->downloadDocsetButton->setText("Stop downloads");
    }
}

void ZealSettingsDialog::on_storageButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(0, "Open Directory");
    if(!dir.isEmpty()) {
        ui->storageEdit->setText(dir);
    }

}

void ZealSettingsDialog::on_deleteButton_clicked()
{
    auto answer = QMessageBox::question(this, "Are you sure",
        QString("Are you sure you want to permanently delete the '%1' docest? "
                "Clicking 'Cancel' in this dialog box will not revert the deletion.").arg(
                              ui->listView->currentIndex().data().toString()));
    if(answer == QMessageBox::Yes) {
        auto dataDir = QDir(docsets->docsetsDir());
        auto docsetName = ui->listView->currentIndex().data().toString();
        zealList.removeRow(ui->listView->currentIndex().row());
        if(dataDir.exists()) {
            ui->docsetsProgress->show();
            ui->deleteButton->hide();
            startTasks();
            auto future = QtConcurrent::run([=] {
                QDir docsetDir(dataDir);
                if(docsetDir.cd(docsetName)) {
                    docsetDir.removeRecursively();
                } else if(docsetDir.cd(docsetName + ".docset")) {
                    docsetDir.removeRecursively();
                }
            });
            QFutureWatcher<void> *watcher = new QFutureWatcher<void>;
            watcher->setFuture(future);
            connect(watcher, &QFutureWatcher<void>::finished, [=]() {
                endTasks();
                ui->deleteButton->show();
                watcher->deleteLater();
            });
        }
    }

}

void ZealSettingsDialog::on_listView_clicked(const QModelIndex &index)
{
    ui->deleteButton->setEnabled(true);
}

void ZealSettingsDialog::resetProgress()
{
    progress.clear();
    totalDownload = 0;
    currentDownload = 0;
    ui->downloadButton->setVisible(!downloadedDocsetsList);
    ui->downloadDocsetButton->setText("Download");
    ui->downloadButton->setEnabled(true);
    ui->updateButton->setEnabled(true);
    ui->addFeedButton->setEnabled(true);
    ui->docsetsList->setEnabled(true);
    displayProgress();
}

QNetworkReply *ZealSettingsDialog::startDownload(const QUrl &url, qint8 retries){
    startTasks(1);
    naManager.setProxy(this->httpProxy());
    QNetworkReply *reply = naManager.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::downloadProgress, this, &ZealSettingsDialog::on_downloadProgress);
    replies.insert(reply, retries);

    if( replies.count() > 0 ){
        ui->downloadDocsetButton->setText("Stop downloads");
        ui->downloadButton->setEnabled(false);
        ui->updateButton->setEnabled(false);
        ui->addFeedButton->setEnabled(false);
    }

    return reply;
}

QNetworkReply *ZealSettingsDialog::startDownload(const ZealDocsetMetadata &meta, qint8 retries){
    QString url = meta.getFeedURL();
    if(url.isEmpty()){
        url = meta.getPrimaryUrl();
    }
    QNetworkReply *reply = startDownload( url, retries );
    reply->setProperty("metadata", QVariant::fromValue(meta));
    return reply;
}

void ZealSettingsDialog::stopDownloads()
{
    for (QNetworkReply *reply: replies.keys()){
        // Hide progress bar
        QVariant itemId = reply->property("listItem");
        QListWidgetItem *listItem = ui->docsetsList->item(itemId.toInt());

        listItem->setData( ProgressItemDelegate::ProgressVisibleRole, false );

        reply->abort();
    }
}

void ZealSettingsDialog::saveSettings(){
    if(ui->storageEdit->text() != docsets->docsetsDir()) {
        // set new docsets dir
        settings.setValue("docsetsDir", ui->storageEdit->text());
        // reload docsets:
        docsets->initialiseDocsets();
        refreshRequested();
    }
    settings.setValue("minFontSize", ui->minFontSize->text());
    settings.setValue("hidingBehavior",
                      ui->radioSysTray->isChecked() ?
                          "systray" : "minimize");
    settings.setValue("startupBehavior",
                      ui->radioStartTray->isChecked() ?
                          "systray" : "window");

    // Proxy settings
    settings.setValue("proxyType", proxyType());
    settings.setValue("httpProxy", ui->m_httpProxy->text());
    settings.setValue("httpProxyPort", ui->m_httpProxyPort->value());
    settings.setValue("httpProxyUser", ui->m_httpProxyUser->text());
    settings.setValue("httpProxy", ui->m_httpProxyPass->text());
}

void ZealSettingsDialog::on_tabWidget_currentChanged()
{
    // Ensure the list is completely up to date
    QModelIndex index = ui->listView->currentIndex();
    ui->listView->reset();


    if (index.isValid()) {
        ui->listView->setCurrentIndex(index);
    }
}

void ZealSettingsDialog::showEvent(QShowEvent *) {
    on_tabWidget_currentChanged();
}

void ZealSettingsDialog::on_buttonBox_accepted()
{
    saveSettings();
}

void ZealSettingsDialog::on_minFontSize_valueChanged(int arg1)
{
    minFontSizeChanged(arg1);
}

void ZealSettingsDialog::on_buttonBox_rejected()
{
    loadSettings();
}

void ZealSettingsDialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if( button == ui->buttonBox->button(QDialogButtonBox::Apply) ){
        saveSettings();
    }
}

void ZealSettingsDialog::on_updateButton_clicked()
{
    updateDocsets();
}

void ZealSettingsDialog::on_addFeedButton_clicked()
{
    QClipboard *clipboard = QApplication::clipboard();
    QString txt = clipboard->text();
    if(!txt.startsWith("dash-feed://")){
        txt = "";
    }
    QString feedUrl = QInputDialog::getText(this, "Zeal", "Feed URL:", QLineEdit::Normal, txt);
    if(feedUrl.isEmpty()){
        return;
    }
    if(feedUrl.startsWith("dash-feed://")){
        feedUrl = feedUrl.remove(0,12);
        feedUrl = QUrl::fromPercentEncoding(feedUrl.toUtf8());
    }
    auto reply = startDownload(feedUrl);
    connect(reply, SIGNAL(finished()), SLOT(extractDocset()));
}

ZealSettingsDialog::ProxyType ZealSettingsDialog::proxyType() const
{
    return ZealSettingsDialog::ProxyType(settings.value("proxyType", ZealSettingsDialog::NoProxy).toInt());
}

QNetworkProxy ZealSettingsDialog::httpProxy() const
{
    QNetworkProxy proxy;

    switch (proxyType()) {
        case ZealSettingsDialog::NoProxy:
            {
                proxy = QNetworkProxy::NoProxy;
            }
            break;

        case ZealSettingsDialog::SystemProxy:
            {
                QNetworkProxyQuery npq(QUrl("http://www.google.com"));
                QList<QNetworkProxy> listOfProxies = QNetworkProxyFactory::systemProxyForQuery(npq);
                if (listOfProxies.size()) {
                    proxy = listOfProxies[0];
                }
            }
            break;

        case ZealSettingsDialog::UserDefinedProxy:
            {
                proxy = QNetworkProxy(QNetworkProxy::HttpProxy, ui->m_httpProxy->text(), ui->m_httpProxyPort->text().toShort());
                if (ui->m_httpProxyNeedsAuth->isChecked()) {
                    proxy.setUser(ui->m_httpProxyUser->text());
                    proxy.setPassword(ui->m_httpProxyPass->text());
                }
            }
            break;

        default:
            Q_ASSERT_X(false, Q_FUNC_INFO, "Unknown proxy type given!");
            break;
    }

    return proxy;
}
