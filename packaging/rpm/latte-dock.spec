# SPDX-FileCopyrightText: 2026 Latte Dock contributors
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Tier-1 RPM recipe for lattecotta-dock / latte-dock.
# Builds and installs the Plasma 6 / Qt6 port on Fedora and openSUSE
# Tumbleweed from one shared spec (multi-distro-ci-plan.md Phase F2).

Name:           latte-dock
Version:        0.10.77
Release:        1%{?dist}
Summary:        Plasma 6 dock / launch bar
License:        GPL-2.0-or-later AND LGPL-2.0-or-later AND (LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL)
URL:            https://userbase.kde.org/LatteDock
Source0:        %{name}-%{version}.tar.gz

# ---- distro-conditional name bridges -----------------------------------------
# Fedora and openSUSE package the same upstream libraries but name several of
# them differently. The macros below let one spec build on both.

%if 0%{?suse_version}
# openSUSE rpm auto-generates qt6qmlimport() requires from bundled QML; the
# latte modules are provided by this package itself, so suppress them.
%global __requires_exclude ^qt6qmlimport.*
%global ecm_pkg             kf6-extra-cmake-modules
%global qtbase_devel        qt6-base-devel
%global qtbase_priv_devel   qt6-base-private-devel
%global qtdecl_devel        qt6-declarative-devel
%global qtwayland_devel     qt6-wayland-devel
%global qt5compat_devel     qt6-qt5compat-devel
%global qtshaders_devel     qt6-shadertools-devel
%global qttools_devel       qt6-tools-devel
%global libplasma_devel     libplasma6-devel
%global plasma_ws_devel     plasma6-workspace-devel
%global plasma_ws_runtime   plasma6-workspace
%global plasma_act_devel    plasma6-activities-devel
%global plasma_actstats_devel plasma6-activities-stats-devel
%global kwayland_devel      kwayland6-devel
%global layershell_devel    layer-shell-qt6-devel
%global plasma5support_devel plasma5support6-devel
%global kpipewire_devel     kpipewire6-devel
%global libksysguard_devel  libksysguard6-devel
%global kwin_pkg             kwin6
%global vulkan_loader       libvulkan1
%global vulkan_dev          vulkan-devel
%global lavapipe_pkg        libvulkan_lvp
%global dbus_pkg            dbus-1
%global cap_pkg             libcap-progs
%global ninja_pkg           ninja
%global gettext_pkg         gettext-tools
%global vulkan_val          vulkan-validationlayers
%global plasma_pa           plasma6-pa
%else
%global ecm_pkg             extra-cmake-modules
%global qtbase_devel        qt6-qtbase-devel
%global qtbase_priv_devel   qt6-qtbase-private-devel
%global qtdecl_devel        qt6-qtdeclarative-devel
%global qtwayland_devel     qt6-qtwayland-devel
%global qt5compat_devel     qt6-qt5compat-devel
%global qtshaders_devel     qt6-qtshadertools-devel
%global qttools_devel       qt6-qttools-devel
%global libplasma_devel     libplasma-devel
%global plasma_ws_devel     plasma-workspace-devel
%global plasma_ws_runtime   plasma-workspace
%global plasma_act_devel    plasma-activities-devel
%global plasma_actstats_devel plasma-activities-stats-devel
%global kwayland_devel      kwayland-devel
%global layershell_devel    layer-shell-qt-devel
%global plasma5support_devel plasma5support-devel
%global kpipewire_devel     kpipewire-devel
%global libksysguard_devel  libksysguard-devel
%global kwin_pkg             kwin
%global vulkan_loader       vulkan-loader
%global vulkan_dev          vulkan-headers
%global lavapipe_pkg        mesa-vulkan-drivers
%global dbus_pkg            dbus
%global cap_pkg             libcap
%global ninja_pkg           ninja-build
%global gettext_pkg         gettext
%global vulkan_val          vulkan-validation-layers
%global plasma_pa           plasma-pa
%endif

BuildRequires:  cmake
BuildRequires:  %{ninja_pkg}
BuildRequires:  gcc-c++
BuildRequires:  %{gettext_pkg}
BuildRequires:  git
BuildRequires:  pkgconf
BuildRequires:  %{ecm_pkg}

# Qt6. Fedora names use qt6-qt*; openSUSE uses qt6-*.
BuildRequires:  %{qtbase_devel}
BuildRequires:  %{qtbase_priv_devel}
BuildRequires:  %{qtdecl_devel}
BuildRequires:  %{qtwayland_devel}
BuildRequires:  %{qt5compat_devel}
BuildRequires:  %{qtshaders_devel}
BuildRequires:  %{qttools_devel}

# KF6 core. Both distros use the kf6-*-devel convention.
BuildRequires:  kf6-kconfig-devel
BuildRequires:  kf6-kcoreaddons-devel
BuildRequires:  kf6-kguiaddons-devel
BuildRequires:  kf6-kdbusaddons-devel
BuildRequires:  kf6-kdeclarative-devel
BuildRequires:  kf6-kitemmodels-devel
BuildRequires:  kf6-kxmlgui-devel
BuildRequires:  kf6-kiconthemes-devel
BuildRequires:  kf6-kio-devel
BuildRequires:  kf6-ki18n-devel
BuildRequires:  kf6-knotifications-devel
BuildRequires:  kf6-knewstuff-devel
BuildRequires:  kf6-karchive-devel
BuildRequires:  kf6-kglobalaccel-devel
BuildRequires:  kf6-kcrash-devel
BuildRequires:  kf6-kwindowsystem-devel
BuildRequires:  kf6-kpackage-devel
BuildRequires:  kf6-ksvg-devel
BuildRequires:  kf6-kcmutils-devel
BuildRequires:  kf6-kirigami-devel
BuildRequires:  kf6-solid-devel
BuildRequires:  kf6-sonnet-devel
BuildRequires:  kf6-ktextwidgets-devel
BuildRequires:  kf6-kidletime-devel
BuildRequires:  kf6-kdoctools-devel
BuildRequires:  kf6-qqc2-desktop-style

# Plasma 6 stack. Names fork between Fedora (no 6 suffix) and openSUSE (6 suffix).
BuildRequires:  %{libplasma_devel}
BuildRequires:  %{plasma_ws_devel}
BuildRequires:  %{plasma_act_devel}
BuildRequires:  %{plasma_actstats_devel}
BuildRequires:  %{kwayland_devel}
BuildRequires:  %{layershell_devel}
BuildRequires:  %{plasma5support_devel}
BuildRequires:  %{kpipewire_devel}
BuildRequires:  %{libksysguard_devel}

# Test gate (sceneprobe) needs vulkan headers + lavapipe; runtime needs the loader.
BuildRequires:  %{vulkan_dev}
BuildRequires:  %{vulkan_val}
BuildRequires:  %{lavapipe_pkg}

# Nested kwin for the runtime/verification gate.
BuildRequires:  %{kwin_pkg}

# CI/gate helpers.
BuildRequires:  jq
BuildRequires:  %{dbus_pkg}
BuildRequires:  %{cap_pkg}
BuildRequires:  rsync

# Common data files.
BuildRequires:  google-noto-sans-fonts

Requires:       %{kwin_pkg}
Requires:       %{plasma_ws_runtime}
Recommends:     %{plasma_pa}
Recommends:     google-noto-sans-fonts

%description
Lattecotta Dock is a from-scratch Plasma 6 / Qt6 port of Latte Dock, a
flexible dock and launch bar for the Plasma desktop. It supports multiple
layouts, parabolic zoom effects, indicator themes, and tight integration
with KWin under Wayland.

%prep
%autosetup -p1 -n %{name}-%{version}

%build
cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=%{_prefix}
cmake --build build --parallel

%install
cmake --install build --prefix %{buildroot}%{_prefix}

# Packager-facing attribution roster, translated from packaging/ATTRIBUTION.md.
install -Dm644 %{_builddir}/%{name}-%{version}/packaging/ATTRIBUTION.md %{buildroot}%{_licensedir}/%{name}/ATTRIBUTION.md

%files
%license %{_licensedir}/%{name}/ATTRIBUTION.md
%{_bindir}/%{name}
%{_datadir}/applications/org.kde.latte-dock.desktop
%{_datadir}/metainfo/org.kde.latte-dock.appdata.xml
%{_datadir}/dbus-1/interfaces/org.kde.LatteDock.xml
%{_datadir}/knotifications6/lattedock.notifyrc
%{_datadir}/knsrcfiles/latte-layouts.knsrc
%{_datadir}/knsrcfiles/latte-indicators.knsrc
%{_datadir}/kservicetypes6/latte-indicator.desktop
%{_datadir}/latte/
%{_datadir}/plasma/plasmoids/org.kde.latte.containment/
%{_datadir}/plasma/plasmoids/org.kde.latte.plasmoid/
%{_datadir}/plasma/shells/org.kde.latte.shell/
%{_libdir}/qt6/plugins/kpackage/packagestructure/latte_indicator.so
%{_libdir}/qt6/plugins/plasma/containmentactions/org.kde.latte.contextmenu.so
%{_libdir}/qt6/qml/org/kde/latte/
%{_datadir}/icons/hicolor/*/*/*latte*.*
%{_datadir}/icons/breeze/applets/256/org.kde.latte.plasmoid.svg
%{_datadir}/locale/*/LC_MESSAGES/%{name}.mo
%{_datadir}/locale/*/LC_MESSAGES/plasma*latte*.mo
%{_datadir}/locale/*/LC_MESSAGES/latte_indicator*.mo

%changelog
* Sun Jul 19 2026 Latte Dock contributors <maintainer@latte-dock.org> - 0.10.77-1
- packaging/rpm: initial Fedora/openSUSE spec for Tier-1 RPM (Phase F2).
- Portable raw cmake build (works around the Fedora/openSUSE %%cmake
  generator divergence) with distro-conditional BuildRequires.
- Verified on Fedora 43; portability sub-check (build + install + smoke)
  on openSUSE Tumbleweed.
