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
#include "quazip/quazip.h"
#include "JlCompress.h"

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

    loadSettings();

    connect(&naManager, &QNetworkAccessManager::finished, [this](QNetworkReply *reply){DownloadCompleteCb(reply);});
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
    ui->storageEdit->setText(docsets->docsetsDir());
}

void ZealSettingsDialog::progressCb(quint64 recv, quint64 total){
    if(recv > 10240) { // don't show progress for non-docset pages (like Google Drive first request)
        ui->docsetsProgress->setMaximum(total);
        ui->docsetsProgress->setValue(recv);
    }
}

void ZealSettingsDialog::DownloadCompleteCb(QNetworkReply *reply){
    ++naCount;
    if(naCount <= 2) { // 1st and 2nd requests are for github and kapeli.com
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
                if(name != "" && !docsets->names().contains(name)) {
                    urls[name] = url;
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
                    ui->docsetsList->addItem(lwi);
                }
            }
        } else {
            QString list = reply->readAll();
            if(reply->error() != QNetworkReply::NoError) {
                QMessageBox::warning(this, "No docsets found", "Failed retrieving list of docsets: " + reply->errorString());
                ui->downloadButton->show();
            } else {
                for(auto item : list.split("\n")) {
                    QStringList docset = item.split(" ");
                    if(docset.size() < 2) break;
                    if(!docsets->names().contains(docset[0])) {
                        if(!docset[1].startsWith("http")) {
                            urls.clear();
                            QMessageBox::warning(this, "No docsets found", "Failed retrieving https://raw.github.com/jkozera/zeal/master/docsets.txt: " + QString(docset[1]));
                            break;
                        }
                        urls[docset[0]] = docset[1];
                        ui->docsetsList->addItem(docset[0]);
                    }
                }
                if(urls.size() > 0) {
                    ui->downloadableGroup->show();
                } else {
                    QMessageBox::warning(this, "No docsets found", QString("No downloadable docsets found."));
                    ui->downloadButton->show();
                }
            }
        }
        if(naCount == 2) {
            ui->docsetsProgress->hide();
        }
    } else {
        if(reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 302) {
            auto reply3 = naManager.get(QNetworkRequest(QUrl(reply->rawHeader("Location"))));
            connect(reply3, &QNetworkReply::downloadProgress, [this](quint64 recv, quint64 total){progressCb(recv, total);});
        } else {
            if(reply->request().url().path().endsWith("tgz") || reply->request().url().path().endsWith("tar.bz2")) {
                auto dataDir = QDir(docsets->docsetsDir());
                if(!dataDir.exists()) {
                    QMessageBox::critical(this, "No docsets directory found",
                                          QString("'%s' directory not found").arg(docsets->docsetsDir()));
                } else {
#ifdef WIN32
                    QDir tardir(QCoreApplication::applicationDirPath());
                    QString program = tardir.filePath("bsdtar.exe");
#else
                    QString program = "bsdtar";
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
                    tar->waitForFinished();
                    auto line_buf = tar->readLine();
                    auto outDir = QString::fromLocal8Bit(line_buf).split("/")[0];

                    if(reply->request().url().path().endsWith("tar.bz2")) {
                        args = QStringList("-jxf");
                    } else {
                        args = QStringList("-zxf");
                    }
                    args.append(tmp->fileName());
                    connect(tar, (void (QProcess::*)(int,QProcess::ExitStatus))&QProcess::finished, [=](int a, QProcess::ExitStatus b) {
                        auto path = reply->request().url().path().split("/");
                        auto fileName = path[path.count()-1];
                        auto docsetName = fileName.replace(".tgz", ".docset");
                        docsetName = docsetName.replace(".tar.bz2", ".docset");

                        if(outDir != docsetName) {
                            QDir dataDir2(dataDir);
                            dataDir2.rename(outDir, docsetName);
                        }

                        // FIXME C&P (see "FIXME C&P" below)
                        QMetaObject::invokeMethod(docsets, "addDocset", Qt::BlockingQueuedConnection,
                                                  Q_ARG(QString, dataDir.absoluteFilePath(docsetName)));
                        zealList.resetModulesCounts();
                        refreshRequested();
                        ui->listView->reset();
                        for(int i = 0; i < ui->docsetsList->count(); ++i) {
                            if(ui->docsetsList->item(i)->text()+".docset" == docsetName) {
                                ui->docsetsList->takeItem(i);
                                break;
                            }
                        }

                        ui->docsetsList->setEnabled(true);
                        ui->downloadDocsetButton->setText("Download");
                        ui->docsetsProgress->hide();
                    });
                    ui->docsetsProgress->setRange(0, 0);
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
                    } else {
                        QStringList *files = new QStringList;
                        ui->docsetsProgress->setRange(0, 0);
                        auto future = QtConcurrent::run([=] {
                            *files = JlCompress::extractDir(tmp->fileName(), dataDir.absolutePath());
                            delete tmp;
                        });
                        QFutureWatcher<void> *watcher = new QFutureWatcher<void>;
                        watcher->setFuture(future);
                        connect(watcher, &QFutureWatcher<void>::finished, [=] {
                            // extract finished - add docset
                            QDir next((*files)[0]), root = next;
                            delete files;
                            next.cdUp();
                            while(next.absolutePath() != dataDir.absolutePath()) {
                                root = next;
                                next.cdUp();
                            }
                            // FIXME C&P (see "FIXME C&P" above)
                            QMetaObject::invokeMethod(docsets, "addDocset", Qt::BlockingQueuedConnection,
                                                      Q_ARG(QString, root.absolutePath()));
                            zealList.resetModulesCounts();
                            refreshRequested();
                            ui->listView->reset();
                            for(int i = 0; i < ui->docsetsList->count(); ++i) {
                                if(ui->docsetsList->item(i)->text() == root.dirName() ||
                                   ui->docsetsList->item(i)->text()+".docset" == root.dirName()) {
                                    ui->docsetsList->takeItem(i);
                                    break;
                                }
                            }

                            ui->docsetsList->setEnabled(true);
                            ui->downloadDocsetButton->setText("Download");
                            ui->docsetsProgress->hide();

                        });
                    }
                } else if(naCount == 3) { // 3rd request - retry once for Google Drive
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
                    auto reply2 = naManager.get(QNetworkRequest(url));
                    connect(reply2, &QNetworkReply::downloadProgress, [this](quint64 recv, quint64 total){progressCb(recv, total);});
                } else {
                    tmp->close();
                    delete tmp;
                    QMessageBox::warning(this, "Error", "Download failed: invalid ZIP file.");
                }
            }
        }
    }
}

void ZealSettingsDialog::on_downloadButton_clicked()
{
   naCount = 0;
   ui->downloadButton->hide();
   ui->docsetsProgress->show();
   QNetworkRequest listRequest(QUrl("https://raw.github.com/jkozera/zeal/master/docsets.txt"));
   QNetworkRequest listRequest2(QUrl("http://kapeli.com/docset_links"));
   naManager.get(listRequest);
   naManager.get(listRequest2);
}

void ZealSettingsDialog::on_docsetsList_clicked(const QModelIndex &index)
{

    ui->downloadDocsetButton->setEnabled(true);
}

void ZealSettingsDialog::on_downloadDocsetButton_clicked()
{
    ui->docsetsList->setEnabled(false);
    ui->downloadDocsetButton->setEnabled(false);
    ui->downloadDocsetButton->setText("Downloading, please wait...");
    QUrl url(urls[ui->docsetsList->currentItem()->text()]);
    naCount = 2;
    auto reply = naManager.get(QNetworkRequest(url));
    if(url.path().endsWith((".tgz")) || url.path().endsWith((".tar.bz2"))) {
        // Dash's docsets don't redirect, so we can start showing progress instantly
        connect(reply, &QNetworkReply::downloadProgress, [this](quint64 recv, quint64 total){progressCb(recv, total);});
    }
    ui->docsetsProgress->show();
    ui->docsetsProgress->setRange(0, 0);
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
            ui->docsetsProgress->setRange(0, 0);
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
                ui->docsetsProgress->hide();
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

void ZealSettingsDialog::on_tabWidget_currentChanged(int index)
{
    // This is needed so the docset list is up to date when the user views it
    ui->listView->reset();
}

void ZealSettingsDialog::on_buttonBox_accepted()
{
    if(ui->storageEdit->text() != docsets->docsetsDir()) {
        // set new docsets dir
        settings.setValue("docsetsDir", ui->storageEdit->text());
        // reload docsets:
        docsets->initialiseDocsets();
    }
    settings.setValue("minFontSize", ui->minFontSize->text());
    settings.setValue("hidingBehavior",
                      ui->radioSysTray->isChecked() ?
                          "systray" : "minimize");
}

void ZealSettingsDialog::on_minFontSize_valueChanged(int arg1)
{
    minFontSizeChanged(arg1);
}

void ZealSettingsDialog::on_buttonBox_rejected()
{
    loadSettings();
}
