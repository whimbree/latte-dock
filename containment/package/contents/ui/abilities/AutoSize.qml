/*
    SPDX-FileCopyrightText: 2019 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-FileCopyrightText: 2026 Bree Spektor
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.8

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore

import org.kde.latte.core 0.2 as LatteCore
import org.kde.latte.private.containment 0.1 as LatteContainment

Item {
    id: sizer

    // when there are only plasma style task managers OR any applets that fill width or height
    // the automatic icon size algorithm should better be disabled
    readonly property bool isActive: root.behaveAsDockWithMask
                                     && Plasmoid.configuration.autoSizeEnabled
                                     && !root.containsOnlyPlasmaTasks
                                     && sizer.layouter.fillApplets<=0
                                     && !(root.inConfigureAppletsMode && Plasmoid.configuration.alignment === LatteCore.Types.Justify) /*block shrinking for justify splitters*/
                                     && latteView
                                     && latteView.visibility
                                     && latteView.visibility.mode !== LatteCore.Types.SidebarOnDemand
                                     && latteView.visibility.mode !== LatteCore.Types.SidebarAutoHide

    property int iconSize: -1 //it is not set, this is the default

    readonly property bool inCalculatedIconSize: ((sizer.metrics.iconSize === sizer.iconSize) || (sizer.metrics.iconSize === sizer.metrics.maxIconSize))
    readonly property bool inAutoSizeAnimation: !sizer.inCalculatedIconSize

    //! required elements
    property Item layouts
    property Item layouter
    property Item metrics
    property Item parabolic
    property Item visibility

    //! The search itself - shrink/grow branch selection, the stepping
    //! loops, the asymmetric limits and the endless-loop protector - lives
    //! in the AutoSizeEngine core (containment/plugin/units/
    //! autosizeengine.h, pinned by tests/units/autosizeenginetest.cpp and
    //! tests/qml/tst_autosize.qml). The stepper owns the prediction
    //! history the protector reads; this file keeps the gates, the timers
    //! and the property reads.
    LatteContainment.AutoSizeStepper {
        id: stepper
    }

    onInAutoSizeAnimationChanged: {
        if (sizer.inAutoSizeAnimation) {
            animations.needBothAxis.addEvent(sizer);
        } else {
            animations.needBothAxis.removeEvent(sizer);
        }
    }

    onIsActiveChanged: {
        stepper.clearHistory();
        sizer.updateIconSize();
    }

    Connections {
        target: root
        function onContainsOnlyPlasmaTasksChanged() {
            sizer.updateIconSize();
        }
        function onMaxLengthChanged() {
            if (latteView && latteView.positioner && !latteView.positioner.isOffScreen) {
                sizer.updateIconSize();
            }
        }
    }

    Connections {
        target: sizer.metrics

        function onPortionIconSizeChanged() {
            if (sizer.metrics.portionIconSize!==-1) {
                sizer.updateIconSize();
            }
        }
    }

    Connections {
        target: latteView
        function onWidthChanged() {
            if (root.isHorizontal && sizer.metrics.portionIconSize!==-1) {
                sizer.updateIconSize();
            }
        }

        function onHeightChanged() {
            if (root.isVertical && sizer.metrics.portionIconSize!==-1) {
                sizer.updateIconSize();
            }
        }
    }

    Connections {
        target: latteView && latteView.positioner ? latteView.positioner : null
        function onIsOffScreenChanged() {
            if (!latteView.positioner.isOffScreen) {
                sizer.updateIconSize();
            }
        }
    }

    Connections {
        target: visibilityManager
        function onInNormalStateChanged() {
            if (visibilityManager.inNormalState) {
                sizer.updateIconSize();
            }
        }
    }

    function updateIconSize() : void {
        if (!sizer.isActive && sizer.iconSize !== -1) {
            // restore original icon size
            sizer.iconSize = -1;
        }

        if (root.maxLength <= 0) {
            //! the view window has no geometry yet (early startup on wayland:
            //! the first call arrives from visibilityChanged before the window
            //! is sized), so every shrink limit would be negative and any
            //! computed size garbage; onMaxLengthChanged re-runs this as soon
            //! as a real length exists
            return;
        }

        if ( !doubleCallAutomaticUpdateIconSize.running && !sizer.visibility.inRelocationHiding /*block too many calls and dont apply during relocatinon hiding*/
                && (sizer.visibility.inNormalState && sizer.isActive) /*in normal and auto size active state*/
                && (sizer.metrics.iconSize === sizer.metrics.maxIconSize || sizer.metrics.iconSize === sizer.iconSize) /*not during animations*/) {

            //!doubler timer
            if (!doubleCallAutomaticUpdateIconSize.secondTimeCallApplied) {
                doubleCallAutomaticUpdateIconSize.start();
            } else {
                doubleCallAutomaticUpdateIconSize.secondTimeCallApplied = false;
            }

            const layoutLength = (Plasmoid.configuration.alignment === LatteCore.Types.Justify) ?
                        sizer.layouts.startLayout.length + sizer.layouts.mainLayout.length + sizer.layouts.endLayout.length : sizer.layouts.mainLayout.length

            const result = stepper.step(layoutLength,
                                        root.maxLength,
                                        sizer.metrics.totals.length,
                                        sizer.metrics.iconSize,
                                        sizer.metrics.maxIconSize,
                                        sizer.parabolic.factor.zoom,
                                        sizer.iconSize);

            if (result.found) {
                //! a found nextIconSize of -1 restores automatic sizing (a
                //! grow reached maxIconSize); the stepper maps the core's
                //! alternatives onto the sizer's own -1 sentinel
                sizer.iconSize = result.nextIconSize;
            }
        }
    }

    //! This functions makes sure to call the updateIconSize(); function which is costly
    //! one more time after its last call to confirm the applied icon size found
    Timer{
        id:doubleCallAutomaticUpdateIconSize
        interval: 1000
        property bool secondTimeCallApplied: false

        onTriggered: {
            if (!doubleCallAutomaticUpdateIconSize.secondTimeCallApplied) {
                doubleCallAutomaticUpdateIconSize.secondTimeCallApplied = true;
                sizer.updateIconSize();
            }
        }
    }
}
