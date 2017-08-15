/****************************************************************************
**
** Copyright (C) 2015-2016 Oleg Shparber
** Copyright (C) 2013-2014 Jerzy Kozera
** Contact: https://go.zealdocs.org/l/contact
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
** along with Zeal. If not, see <https://www.gnu.org/licenses/>.
**
****************************************************************************/

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <core/application.h>
#include <core/settings.h>

#include <QDir>
#include <QFileDialog>
#include <QWebSettings>

using namespace Zeal;
using namespace Zeal::WidgetUi;

SettingsDialog::SettingsDialog(Core::Application *app, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog()),
    m_application(app)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    // Setup signals & slots
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::saveSettings);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::loadSettings);
    connect(ui->buttonBox, &QDialogButtonBox::clicked, this, [this](QAbstractButton *button) {
        if (button == ui->buttonBox->button(QDialogButtonBox::Apply))
            saveSettings();
    });

    connect(ui->minimumFontSizeSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, [](int value) {
        QWebSettings::globalSettings()->setFontSize(QWebSettings::MinimumFontSize, value);
    });

    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::chooseCustomCssFile()
{
    const QString file = QFileDialog::getOpenFileName(this, tr("Choose CSS File"),
                                                      ui->customCssFileEdit->text(),
                                                      tr("CSS Files (*.css);;All Files (*.*)"));
    if (file.isEmpty())
        return;

    ui->customCssFileEdit->setText(QDir::toNativeSeparators(file));
}

void SettingsDialog::chooseDocsetStoragePath()
{
    const QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                          ui->docsetStorageEdit->text());
    if (dir.isEmpty())
        return;

    ui->docsetStorageEdit->setText(QDir::toNativeSeparators(dir));
}

void SettingsDialog::loadSettings()
{
    const Core::Settings * const settings = m_application->settings();
    // General Tab
    ui->startMinimizedCheckBox->setChecked(settings->startMinimized);
    ui->checkForUpdateCheckBox->setChecked(settings->checkForUpdate);

    ui->systrayGroupBox->setChecked(settings->showSystrayIcon);
    ui->minimizeToSystrayCheckBox->setChecked(settings->minimizeToSystray);
    ui->hideToSystrayCheckBox->setChecked(settings->hideOnClose);

    ui->toolButton->setKeySequence(settings->showShortcut);

    ui->docsetStorageEdit->setText(QDir::toNativeSeparators(settings->docsetPath));

    // Tabs Tab
    ui->openNewTabAfterActive->setChecked(settings->openNewTabAfterActive);

    // Search Tab
    ui->fuzzySearchCheckBox->setChecked(settings->fuzzySearchEnabled);

    // Content Tab
    ui->minimumFontSizeSpinBox->setValue(settings->minimumFontSize);
    ui->darkModeCheckBox->setChecked(settings->darkModeEnabled);
    ui->highlightOnNavigateCheckBox->setChecked(settings->highlightOnNavigateEnabled);
    ui->customCssFileEdit->setText(QDir::toNativeSeparators(settings->customCssFile));
    ui->disableAdCheckBox->setChecked(settings->isAdDisabled);

    // Network Tab
    switch (settings->proxyType) {
    case Core::Settings::ProxyType::None:
        ui->noProxySettings->setChecked(true);
        break;
    case Core::Settings::ProxyType::System:
        ui->systemProxySettings->setChecked(true);
        break;
    case Core::Settings::ProxyType::UserDefined:
        ui->manualProxySettings->setChecked(true);
        break;
    }

    ui->httpProxy->setText(settings->proxyHost);
    ui->httpProxyPort->setValue(settings->proxyPort);
    ui->httpProxyNeedsAuth->setChecked(settings->proxyAuthenticate);
    ui->httpProxyUser->setText(settings->proxyUserName);
    ui->httpProxyPass->setText(settings->proxyPassword);
}

void SettingsDialog::saveSettings()
{
    Core::Settings * const settings = m_application->settings();

    // General Tab
    settings->startMinimized = ui->startMinimizedCheckBox->isChecked();
    settings->checkForUpdate = ui->checkForUpdateCheckBox->isChecked();

    settings->showSystrayIcon = ui->systrayGroupBox->isChecked();
    settings->minimizeToSystray = ui->minimizeToSystrayCheckBox->isChecked();
    settings->hideOnClose = ui->hideToSystrayCheckBox->isChecked();

    settings->showShortcut = ui->toolButton->keySequence();

    settings->docsetPath = QDir::fromNativeSeparators(ui->docsetStorageEdit->text());

    // Tabs Tab
    settings->openNewTabAfterActive = ui->openNewTabAfterActive->isChecked();

    // Search Tab
    settings->fuzzySearchEnabled = ui->fuzzySearchCheckBox->isChecked();

    // Content Tab
    settings->minimumFontSize = ui->minimumFontSizeSpinBox->text().toInt();
    settings->darkModeEnabled = ui->darkModeCheckBox->isChecked();
    settings->highlightOnNavigateEnabled = ui->highlightOnNavigateCheckBox->isChecked();
    settings->customCssFile = QDir::fromNativeSeparators(ui->customCssFileEdit->text());
    settings->isAdDisabled = ui->disableAdCheckBox->isChecked();

    // Network Tab
    // Proxy settings
    if (ui->noProxySettings->isChecked())
        settings->proxyType = Core::Settings::ProxyType::None;
    else if (ui->systemProxySettings->isChecked())
        settings->proxyType = Core::Settings::ProxyType::System;
    else if (ui->manualProxySettings->isChecked())
        settings->proxyType = Core::Settings::ProxyType::UserDefined;

    settings->proxyHost = ui->httpProxy->text();
    settings->proxyPort = ui->httpProxyPort->text().toUShort();
    settings->proxyAuthenticate = ui->httpProxyNeedsAuth->isChecked();
    settings->proxyUserName = ui->httpProxyUser->text();
    settings->proxyPassword = ui->httpProxyPass->text();

    settings->save();
}
