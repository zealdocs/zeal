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

namespace {
// QFontDatabase::standardSizes() lacks some sizes, like 13, which QtWK uses by default.
const int AvailableFontSizes[] = {9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
                                  20, 22, 24, 26, 28, 30, 32, 34, 36,
                                  40, 44, 48, 56, 64, 72};
const QWebSettings::FontFamily BasicFontFamilies[] = {QWebSettings::SerifFont,
                                                      QWebSettings::SansSerifFont,
                                                      QWebSettings::FixedFont};
}

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog())
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

    // Fonts
    ui->defaultFontComboBox->addItem(tr("Serif"), QStringLiteral("serif"));
    ui->defaultFontComboBox->addItem(tr("Sans-serif"), QStringLiteral("sans-serif"));
    ui->defaultFontComboBox->addItem(tr("Monospace"), QStringLiteral("monospace"));

    ui->minFontSizeComboBox->addItem(tr("None"), 0);
    for (int fontSize : AvailableFontSizes) {
        ui->fontSizeComboBox->addItem(QString::number(fontSize), fontSize);
        ui->fixedFontSizeComboBox->addItem(QString::number(fontSize), fontSize);
        ui->minFontSizeComboBox->addItem(QString::number(fontSize), fontSize);
    }

    // Fix tab order.
    setTabOrder(ui->defaultFontComboBox, ui->fontSizeComboBox);
    setTabOrder(ui->fontSizeComboBox, ui->serifFontComboBox);

    QWebSettings *webSettings = QWebSettings::globalSettings();

    // Avoid casting in each connect.
    auto currentIndexChangedSignal
            = static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged);

    auto syncStandardFont = [this, webSettings](QWebSettings::FontFamily fontFamily,
            const QFont &font) {
        const int index = ui->defaultFontComboBox->currentIndex();
        if (BasicFontFamilies[index] == fontFamily) {
            webSettings->setFontFamily(QWebSettings::StandardFont, font.family());
        }
    };

    connect(ui->defaultFontComboBox, currentIndexChangedSignal,
            this, [webSettings](int index) {
        const QString fontFamily = webSettings->fontFamily(BasicFontFamilies[index]);
        webSettings->setFontFamily(QWebSettings::StandardFont, fontFamily);
    });

    connect(ui->serifFontComboBox, &QFontComboBox::currentFontChanged,
            this, [webSettings, syncStandardFont](const QFont &font) {
        webSettings->setFontFamily(QWebSettings::SerifFont, font.family());
        syncStandardFont(QWebSettings::SerifFont, font);
    });
    connect(ui->sansSerifFontComboBox, &QFontComboBox::currentFontChanged,
            this, [webSettings, syncStandardFont](const QFont &font) {
        webSettings->setFontFamily(QWebSettings::SansSerifFont, font.family());
        syncStandardFont(QWebSettings::SansSerifFont, font);
    });
    connect(ui->fixedFontComboBox, &QFontComboBox::currentFontChanged,
            this, [webSettings, syncStandardFont](const QFont &font) {
        webSettings->setFontFamily(QWebSettings::FixedFont, font.family());
        syncStandardFont(QWebSettings::FixedFont, font);
    });

    connect(ui->fontSizeComboBox, currentIndexChangedSignal, this, [webSettings](int index) {
        webSettings->setFontSize(QWebSettings::DefaultFontSize, AvailableFontSizes[index]);
    });
    connect(ui->fixedFontSizeComboBox, currentIndexChangedSignal, this, [webSettings](int index) {
        webSettings->setFontSize(QWebSettings::DefaultFixedFontSize, AvailableFontSizes[index]);
    });
    connect(ui->minFontSizeComboBox, currentIndexChangedSignal, this, [webSettings](int index) {
        const int fontSize = index == 0 ? 0 : AvailableFontSizes[index-1];
        webSettings->setFontSize(QWebSettings::MinimumFontSize, fontSize);
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
    const Core::Settings * const settings = Core::Application::instance()->settings();

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
    for (int i = 0; i < ui->defaultFontComboBox->count(); ++i) {
        if (ui->defaultFontComboBox->itemData(i).toString() == settings->defaultFontFamily) {
            ui->defaultFontComboBox->setCurrentIndex(i);
            break;
        }
    }
    ui->serifFontComboBox->setCurrentText(settings->serifFontFamily);
    ui->sansSerifFontComboBox->setCurrentText(settings->sansSerifFontFamily);
    ui->fixedFontComboBox->setCurrentText(settings->fixedFontFamily);

    ui->fontSizeComboBox->setCurrentText(QString::number(settings->defaultFontSize));
    ui->fixedFontSizeComboBox->setCurrentText(QString::number(settings->defaultFixedFontSize));
    ui->minFontSizeComboBox->setCurrentText(QString::number(settings->minimumFontSize));

    ui->darkModeCheckBox->setChecked(settings->darkModeEnabled);
    ui->highlightOnNavigateCheckBox->setChecked(settings->highlightOnNavigateEnabled);
    ui->customCssFileEdit->setText(QDir::toNativeSeparators(settings->customCssFile));

    switch (settings->externalLinkPolicy) {
    case Core::Settings::ExternalLinkPolicy::Ask:
        ui->radioExternalLinkAsk->setChecked(true);
        break;
    case Core::Settings::ExternalLinkPolicy::Open:
        ui->radioExternalLinkOpen->setChecked(true);
        break;
    case Core::Settings::ExternalLinkPolicy::OpenInSystemBrowser:
        ui->radioExternalLinkOpenDesktop->setChecked(true);
        break;
    }

    ui->useSmoothScrollingCheckBox->setChecked(settings->isSmoothScrollingEnabled);
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
    Core::Settings * const settings = Core::Application::instance()->settings();

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
    settings->defaultFontFamily = ui->defaultFontComboBox->currentData().toString();
    settings->serifFontFamily = ui->serifFontComboBox->currentText();
    settings->sansSerifFontFamily = ui->sansSerifFontComboBox->currentText();
    settings->fixedFontFamily = ui->fixedFontComboBox->currentText();

    settings->defaultFontSize = ui->fontSizeComboBox->currentData().toInt();
    settings->defaultFixedFontSize = ui->fixedFontSizeComboBox->currentData().toInt();
    settings->minimumFontSize = ui->minFontSizeComboBox->currentData().toInt();

    settings->darkModeEnabled = ui->darkModeCheckBox->isChecked();
    settings->highlightOnNavigateEnabled = ui->highlightOnNavigateCheckBox->isChecked();
    settings->customCssFile = QDir::fromNativeSeparators(ui->customCssFileEdit->text());

    if (ui->radioExternalLinkAsk->isChecked()) {
        settings->externalLinkPolicy = Core::Settings::ExternalLinkPolicy::Ask;
    } else if (ui->radioExternalLinkOpen->isChecked()) {
        settings->externalLinkPolicy = Core::Settings::ExternalLinkPolicy::Open;
    } else if (ui->radioExternalLinkOpenDesktop->isChecked()) {
        settings->externalLinkPolicy = Core::Settings::ExternalLinkPolicy::OpenInSystemBrowser;
    }

    settings->isSmoothScrollingEnabled = ui->useSmoothScrollingCheckBox->isChecked();
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
