/*
    SPDX-FileCopyrightText: 2026 Bree Spektor
    SPDX-FileCopyrightText: 2026 Latte Dock contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

//! Minimal applet for appletsexpandedpropertytest.cpp, loaded by libplasma
//! through the qrc applet path. Unlike the parking harness applet this one
//! carries a compact representation: `expanded` only collapses back to
//! false when there is a compact form to collapse INTO (a full-rep-only
//! applet is pinned expanded), and the configure-mode popup collapse this
//! test backs is exactly about compact applets with open popups.

import QtQuick
import org.kde.plasma.plasmoid

PlasmoidItem {
    switchWidth: 9999
    switchHeight: 9999

    compactRepresentation: Item {}
    fullRepresentation: Item {}
}
