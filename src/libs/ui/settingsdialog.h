// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// Copyright (C) 2013-2014 Jerzy Kozera
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ZEAL_WIDGETUI_SETTINGSDIALOG_H
#define ZEAL_WIDGETUI_SETTINGSDIALOG_H

#include <QDialog>

namespace Zeal {
namespace WidgetUi {

namespace Ui {
class SettingsDialog;
} // namespace Ui

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override;

public slots:
    void chooseCustomCssFile();
    void chooseDocsetStoragePath();

private:
    void loadSettings();
    void saveSettings();

private:
    Ui::SettingsDialog *ui = nullptr;

    friend class Ui::SettingsDialog;
};

} // namespace WidgetUi
} // namespace Zeal

#endif // ZEAL_WIDGETUI_SETTINGSDIALOG_H
