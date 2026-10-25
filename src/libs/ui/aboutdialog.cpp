// Copyright (C) Oleg Shparber, et al. <https://zealdocs.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "aboutdialog.h"

#include <core/application.h>

#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLatin1StringView>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace Zeal::WidgetUi {

namespace {
using Qt::Literals::StringLiterals::operator""_L1;

struct Credit
{
    QLatin1StringView name;
    QLatin1StringView url; // Without the https:// prefix, which is added for the link.
    QLatin1StringView license;
};

constexpr Credit Credits[] = {
    {"cpp-httplib"_L1, "github.com/yhirose/cpp-httplib"_L1, "MIT License"_L1},
    {"Google Sans Code"_L1, "github.com/googlefonts/googlesans-code"_L1, "SIL Open Font License 1.1"_L1},
    {"Inter"_L1, "github.com/rsms/inter"_L1, "SIL Open Font License 1.1"_L1},
    {"libarchive"_L1, "www.libarchive.org"_L1, "BSD 2-Clause License"_L1},
    {"LibQxt"_L1, "bitbucket.org/libqxt"_L1, "BSD 3-Clause License"_L1},
    {"Qt"_L1, "www.qt.io"_L1, "GNU Lesser General Public License version 3"_L1},
    {"Simple Icons"_L1, "simpleicons.org"_L1, "CC0 1.0 Universal"_L1},
    {"Source Serif 4"_L1, "github.com/adobe-fonts/source-serif"_L1, "SIL Open Font License 1.1"_L1},
    {"SQLite"_L1, "www.sqlite.org"_L1, "Public Domain"_L1},
    {"Tabler Icons"_L1, "tabler.io/icons"_L1, "MIT License"_L1},
    {"toml++"_L1, "github.com/marzer/tomlplusplus"_L1, "MIT License"_L1},
    {"zlib"_L1, "zlib.net"_L1, "zlib License"_L1},
};
} // namespace

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About Zeal"));
    resize(420, 420);

    // Header.
    auto headerFrame = new QFrame(this);
    headerFrame->setFrameShape(QFrame::StyledPanel);
    headerFrame->setFrameShadow(QFrame::Sunken);

    auto logoLabel = new QLabel(headerFrame);
    const QIcon logo = QIcon::fromTheme(QStringLiteral("zeal"), QIcon(QStringLiteral(":/zeal.svg")));
    logoLabel->setPixmap(logo.pixmap(QSize(64, 64)));

    auto nameLabel = new QLabel(QStringLiteral("<h1>Zeal</h1>"), headerFrame);

    auto versionLabel = new QLabel(Core::Application::versionString(), headerFrame);
    versionLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::TextSelectableByMouse);

    auto titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(0);
    titleLayout->addWidget(nameLabel);
    titleLayout->addWidget(versionLabel);

    auto headerLayout = new QHBoxLayout(headerFrame);
    headerLayout->setSpacing(10);
    headerLayout->addWidget(logoLabel);
    headerLayout->addLayout(titleLayout, 1);

    // About tab.
    auto tabWidget = new QTabWidget(this);

    auto aboutTab = new QWidget();
    auto aboutLabel = new QLabel(aboutTab);
    aboutLabel->setText(tr("Copyright &copy; Oleg Shparber and other contributors."
                           "<br><br>"
                           "<a href=\"https://zealdocs.org\">zealdocs.org</a>"
                           "<br>"
                           "<a href=\"https://go.zealdocs.org/l/contact\">Get in touch</a>"
                           "<br><br>"
                           "Zeal is open source software, available under the GNU General Public License version 3 "
                           "(<a href=\"https://www.gnu.org/licenses/gpl-3.0.html\">GPLv3</a>) or later."
                           "<br><br>"
                           "Docsets are courtesy of <a href=\"https://kapeli.com/dash\">Dash</a>."));
    aboutLabel->setTextFormat(Qt::RichText);
    aboutLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    aboutLabel->setWordWrap(true);
    aboutLabel->setOpenExternalLinks(true);

    auto aboutLayout = new QVBoxLayout(aboutTab);
    aboutLayout->addWidget(aboutLabel);

    tabWidget->addTab(aboutTab, tr("About"));

    // Licenses tab.
    QString licensesHtml = QStringLiteral("<p>%1</p>")
                               .arg(tr("Zeal heavily relies on other open source software listed below."));
    for (const Credit &credit : Credits) {
        licensesHtml += QStringLiteral("<p><b>%1</b> - <a href=\"https://%2\">%2</a><br>%3</p>")
                            .arg(credit.name, credit.url, tr("License: %1").arg(credit.license));
    }

    auto licensesTab = new QWidget();
    auto licensesBrowser = new QTextBrowser(licensesTab);
    licensesBrowser->setOpenExternalLinks(true);
    licensesBrowser->setHtml(licensesHtml);

    auto licensesLayout = new QVBoxLayout(licensesTab);
    licensesLayout->addWidget(licensesBrowser);

    tabWidget->addTab(licensesTab, tr("Licenses"));

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto layout = new QVBoxLayout(this);
    layout->addWidget(headerFrame);
    layout->addWidget(tabWidget);
    layout->addWidget(buttonBox);

    buttonBox->setFocus(Qt::OtherFocusReason);
}

} // namespace Zeal::WidgetUi
