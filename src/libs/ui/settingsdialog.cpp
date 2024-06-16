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

#include <qxtglobalshortcut/qxtglobalshortcut.h>

#include <browser/settings.h>
#include <core/application.h>
#include <core/settings.h>

#include <QDir>
#include <QFileDialog>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

using namespace Zeal;
using namespace Zeal::WidgetUi;

namespace {
// QFontDatabase::standardSizes() lacks some sizes, like 13, which QtWK uses by default.
constexpr int AvailableFontSizes[] = {9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
                                      20, 22, 24, 26, 28, 30, 32, 34, 36,
                                      40, 44, 48, 56, 64, 72};
constexpr QWebEngineSettings::FontFamily BasicFontFamilies[] = {QWebEngineSettings::SerifFont,
                                                                QWebEngineSettings::SansSerifFont,
                                                                QWebEngineSettings::FixedFont};
} // namespace

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog())
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

    // Disable global shortcut settings if not supported.
    if (!QxtGlobalShortcut::isSupported()) {
        ui->globalHotKeyGroupBox->setEnabled(false);
        ui->globalHotKeyGroupBox->setToolTip(tr("Global shortcuts are not supported on the current platform."));
    }

    QWebEngineSettings *webSettings = Browser::Settings::defaultProfile()->settings();

    // Avoid casting in each connect.
    auto currentIndexChangedSignal
            = static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged);

    auto syncStandardFont = [this, webSettings](QWebEngineSettings::FontFamily fontFamily,
            const QFont &font) {
        const int index = ui->defaultFontComboBox->currentIndex();
        if (BasicFontFamilies[index] == fontFamily) {
            webSettings->setFontFamily(QWebEngineSettings::StandardFont, font.family());
        }
    };

    connect(ui->defaultFontComboBox, currentIndexChangedSignal,
            this, [webSettings](int index) {
        const QString fontFamily = webSettings->fontFamily(BasicFontFamilies[index]);
        webSettings->setFontFamily(QWebEngineSettings::StandardFont, fontFamily);
    });

    connect(ui->serifFontComboBox, &QFontComboBox::currentFontChanged,
            this, [webSettings, syncStandardFont](const QFont &font) {
        webSettings->setFontFamily(QWebEngineSettings::SerifFont, font.family());
        syncStandardFont(QWebEngineSettings::SerifFont, font);
    });
    connect(ui->sansSerifFontComboBox, &QFontComboBox::currentFontChanged,
            this, [webSettings, syncStandardFont](const QFont &font) {
        webSettings->setFontFamily(QWebEngineSettings::SansSerifFont, font.family());
        syncStandardFont(QWebEngineSettings::SansSerifFont, font);
    });
    connect(ui->fixedFontComboBox, &QFontComboBox::currentFontChanged,
            this, [webSettings, syncStandardFont](const QFont &font) {
        webSettings->setFontFamily(QWebEngineSettings::FixedFont, font.family());
        syncStandardFont(QWebEngineSettings::FixedFont, font);
    });

    connect(ui->fontSizeComboBox, currentIndexChangedSignal, this, [webSettings](int index) {
        webSettings->setFontSize(QWebEngineSettings::DefaultFontSize, AvailableFontSizes[index]);
    });
    connect(ui->fixedFontSizeComboBox, currentIndexChangedSignal, this, [webSettings](int index) {
        webSettings->setFontSize(QWebEngineSettings::DefaultFixedFontSize, AvailableFontSizes[index]);
    });
    connect(ui->minFontSizeComboBox, currentIndexChangedSignal, this, [webSettings](int index) {
        const int fontSize = index == 0 ? 0 : AvailableFontSizes[index-1];
        webSettings->setFontSize(QWebEngineSettings::MinimumFontSize, fontSize);
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
    QString path = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                     ui->docsetStorageEdit->text());
    if (path.isEmpty()) {
        return;
    }

#ifdef PORTABLE_BUILD
    // Use relative path if selected directory is under the application binary path.
    if (path.startsWith(QCoreApplication::applicationDirPath() + QLatin1String("/"))) {
        const QDir appDirPath(QCoreApplication::applicationDirPath());
        path = appDirPath.relativeFilePath(path);
    }
#endif

    ui->docsetStorageEdit->setText(QDir::toNativeSeparators(path));
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
    ui->fuzzySearchCheckBox->setChecked(settings->isFuzzySearchEnabled);

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

    switch (settings->contentAppearance) {
    case Core::Settings::ContentAppearance::Automatic:
        ui->appearanceAutoRadioButton->setChecked(true);
        break;
    case Core::Settings::ContentAppearance::Light:
        ui->appearanceLightRadioButton->setChecked(true);
        break;
    case Core::Settings::ContentAppearance::Dark:
        ui->appearanceDarkRadioButton->setChecked(true);
        break;
    }

    ui->highlightOnNavigateCheckBox->setChecked(settings->isHighlightOnNavigateEnabled);
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

    // Network Tab
    switch (settings->proxyType) {
    case Core::Settings::ProxyType::None:
        ui->noProxySettings->setChecked(true);
        break;
    case Core::Settings::ProxyType::System:
        ui->systemProxySettings->setChecked(true);
        break;
    case Core::Settings::ProxyType::Http:
        ui->manualProxySettings->setChecked(true);
        ui->proxyTypeHttpRadioButton->setChecked(true);
        break;
    case Core::Settings::ProxyType::Socks5:
        ui->manualProxySettings->setChecked(true);
        ui->proxyTypeSocks5RadioButton->setChecked(true);
        break;
    }

    ui->proxyHostEdit->setText(settings->proxyHost);
    ui->proxyPortEdit->setValue(settings->proxyPort);
    ui->proxyRequiresAuthCheckBox->setChecked(settings->proxyAuthenticate);
    ui->proxyUsernameEdit->setText(settings->proxyUserName);
    ui->proxyPasswordEdit->setText(settings->proxyPassword);

    ui->ignoreSslErrorsCheckBox->setChecked(settings->isIgnoreSslErrorsEnabled);
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
    settings->isFuzzySearchEnabled = ui->fuzzySearchCheckBox->isChecked();

    // Content Tab
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0) && QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    // Applying dark mode requires restart.
    ui->appearanceLabel->setText(tr("Appearance (requires restart):"));
#endif

    settings->defaultFontFamily = ui->defaultFontComboBox->currentData().toString();
    settings->serifFontFamily = ui->serifFontComboBox->currentText();
    settings->sansSerifFontFamily = ui->sansSerifFontComboBox->currentText();
    settings->fixedFontFamily = ui->fixedFontComboBox->currentText();

    settings->defaultFontSize = ui->fontSizeComboBox->currentData().toInt();
    settings->defaultFixedFontSize = ui->fixedFontSizeComboBox->currentData().toInt();
    settings->minimumFontSize = ui->minFontSizeComboBox->currentData().toInt();

    if (ui->appearanceAutoRadioButton->isChecked()) {
        settings->contentAppearance = Core::Settings::ContentAppearance::Automatic;
    } else if (ui->appearanceLightRadioButton->isChecked()) {
        settings->contentAppearance = Core::Settings::ContentAppearance::Light;
    } else if (ui->appearanceDarkRadioButton->isChecked()) {
        settings->contentAppearance = Core::Settings::ContentAppearance::Dark;
    }

    settings->isHighlightOnNavigateEnabled = ui->highlightOnNavigateCheckBox->isChecked();
    settings->customCssFile = QDir::fromNativeSeparators(ui->customCssFileEdit->text());

    if (ui->radioExternalLinkAsk->isChecked()) {
        settings->externalLinkPolicy = Core::Settings::ExternalLinkPolicy::Ask;
    } else if (ui->radioExternalLinkOpen->isChecked()) {
        settings->externalLinkPolicy = Core::Settings::ExternalLinkPolicy::Open;
    } else if (ui->radioExternalLinkOpenDesktop->isChecked()) {
        settings->externalLinkPolicy = Core::Settings::ExternalLinkPolicy::OpenInSystemBrowser;
    }

    settings->isSmoothScrollingEnabled = ui->useSmoothScrollingCheckBox->isChecked();

    // Network Tab
    // Proxy settings
    if (ui->noProxySettings->isChecked()) {
        settings->proxyType = Core::Settings::ProxyType::None;
    } else if (ui->systemProxySettings->isChecked()) {
        settings->proxyType = Core::Settings::ProxyType::System;
    } else if (ui->manualProxySettings->isChecked()) {
        if (ui->proxyTypeSocks5RadioButton->isChecked()) {
            settings->proxyType = Core::Settings::ProxyType::Socks5;
        } else {
            settings->proxyType = Core::Settings::ProxyType::Http;
        }
    }

    settings->proxyHost = ui->proxyHostEdit->text();
    settings->proxyPort = ui->proxyPortEdit->text().toUShort();
    settings->proxyAuthenticate = ui->proxyRequiresAuthCheckBox->isChecked();
    settings->proxyUserName = ui->proxyUsernameEdit->text();
    settings->proxyPassword = ui->proxyPasswordEdit->text();

    settings->isIgnoreSslErrorsEnabled = ui->ignoreSslErrorsCheckBox->isChecked();

    settings->save();
}
