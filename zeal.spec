%global _hardened_build 1

Summary: Zeal: Simple offline API documentation browser
Name: zeal
Version: 0.1.1
Release: 1%{?dist}
License: GPLv2+
Group: Development/Tools
URL: http://zealdocs.org/
Source0: https://github.com/zealdocs/zeal/archive/v%{version}.tar.gz
BuildRequires: qt5-qtwebkit-devel libarchive-devel qt5-qtx11extras-devel xcb-util-keysyms-devel
Requires: qt5-qtbase qt5-qtwebkit libarchive qt5-qtx11extras xcb-util-keysyms

%description
Zeal is a simple offline documentation browser inspired by Dash.

%prep
%autosetup

%build
qmake-qt5
make %{?_smp_mflags}

%install
export INSTALL_ROOT=%{buildroot}
make install

%files
/usr/bin/zeal
/usr/share/applications/zeal.desktop
/usr/share/icons/hicolor/*/apps/zeal.png

%changelog
