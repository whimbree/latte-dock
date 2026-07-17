/*
    SPDX-FileCopyrightText: 2026 Bree Spektor
    SPDX-FileCopyrightText: 2026 Latte Dock contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

//! Containment main script for shortcutshosttest.cpp, loaded by libplasma
//! through the qrc applet path (:/qt/qml/plasma/applet/test/host/main.qml).
//! It mirrors ONLY the tree shape the discovery walk reads - the root
//! ContainmentItem carries the containmentViewLayout objectName itself
//! (the Plasma 6 tree change) and the ability host is its direct child -
//! while the host and its private ARE the real shipped files: the qrc
//! aliases point into containment/package/contents/ui/abilities, so an
//! objectName or function-signature drift in the real QML fails the pin.
//!
//! The properties below are the names the real host chain resolves from
//! the real main.qml's root (readonly aliases there); they are stubbed
//! inert because the test reads the tree and the metaobject, never a live
//! badge pipeline.

import QtQuick
import org.kde.plasma.plasmoid

import "abilities" as Ability

ContainmentItem {
    id: root
    objectName: "containmentViewLayout"

    readonly property QtObject dragOverlay: null
    readonly property QtObject shortcutsEngine: null

    readonly property Item layouter: Item {
        property bool appletsInParentChange: false
    }

    readonly property Item indexer: Item {
        function appletIdForVisibleIndex(itemVisibleIndex: int) : int {
            return -1;
        }
    }

    readonly property Item emptyLayouts: Item {
        readonly property Item startLayout: Item {}
        readonly property Item mainLayout: Item {}
        readonly property Item endLayout: Item {}
    }

    Ability.PositionShortcuts {
        layouts: root.emptyLayouts
    }
}
