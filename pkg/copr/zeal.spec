# RPM spec for Zeal, built on Fedora COPR.
#
# The automation rewrites the Version line (and the bundled(cpp-httplib) Provides)
# in this spec before building the source RPM (see .github/workflows/publish-copr.yaml).
# They are baked in rather than passed via --define, because COPR rebuilds the SRPM
# without those defines and the version would reset to 0.0.0. A plain build with no
# rewrite produces 0.0.0. CMake already installs the binary, .desktop file, AppStream
# metainfo, and icons under the prefix on Linux (see assets/freedesktop/CMakeLists.txt),
# so %%cmake_install needs no extra glue. toml++ comes from the distro
# (tomlplusplus-devel); cpp-httplib stays bundled in src/contrib because the system
# versions span too wide a range to rely on.

Name:           zeal
Version:        0.0.0
Release:        1%{?dist}
Summary:        Offline documentation browser

License:        GPL-3.0-or-later
URL:            https://zealdocs.org
# URL form is metadata for release builds only; it does not resolve for nightly
# snapshot versions (e.g. 0.8.2^15.gabcdef). Every build supplies the tarball
# locally, so Source0 is never fetched.
Source0:        https://github.com/zealdocs/zeal/archive/refs/tags/v%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.25.1
BuildRequires:  extra-cmake-modules
BuildRequires:  gcc-c++
BuildRequires:  ninja-build

BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtsvg-devel
BuildRequires:  qt6-qtwebchannel-devel
BuildRequires:  qt6-qtwebengine-devel

BuildRequires:  libarchive-devel
BuildRequires:  tomlplusplus-devel
BuildRequires:  sqlite-devel
BuildRequires:  mesa-libGL-devel
BuildRequires:  vulkan-headers
BuildRequires:  libX11-devel
BuildRequires:  libxcb-devel
BuildRequires:  xcb-util-keysyms-devel
BuildRequires:  libxkbcommon-devel

Requires:       hicolor-icon-theme

Provides:       bundled(cpp-httplib)

%description
Zeal is an offline documentation browser that keeps the references you rely
on right on your computer, ready the moment you need them — on a plane, off
the grid, or just on flaky Wi-Fi. Instant search takes you straight to the
right page before a browser tab would even finish loading.

Build your personal library from over 900 docsets in the same format Dash
uses, covering programming languages, frameworks, and tools. Open results in
tabs and summon Zeal from anywhere with a global shortcut.

  * Search all docsets at once, or limit the search with a docset prefix.
  * Plugins for VS Code, IntelliJ IDEA, Sublime Text, Vim, Emacs, and Kate.

%prep
%autosetup -n %{name}-%{version}

%build
%cmake -GNinja
%cmake_build

%install
%cmake_install

%files
%license COPYING
%doc README.md
%{_bindir}/zeal
%{_datadir}/applications/org.zealdocs.zeal.desktop
%{_datadir}/metainfo/org.zealdocs.zeal.appdata.xml
%{_datadir}/icons/hicolor/*/apps/zeal*

%changelog
* Mon Jun 01 2026 Zeal Release <release@zealdocs.org> - 0.0.0-1
- Placeholder entry; the release automation regenerates this for each upload.
