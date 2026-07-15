/*
    SPDX-FileCopyrightText: 2018 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick 2.7
import Qt5Compat.GraphicalEffects as GraphicalEffects

import org.kde.latte.components 1.0 as LatteComponents

import org.kde.plasma.plasmoid 2.0
import org.kde.plasma.core 2.0 as PlasmaCore

Item {
    //! Leave the scenegraph entirely while fully faded. The wrapper's layer
    //! (ItemWrapper.qml) tracks this item's opacity and drops at exactly 0,
    //! but the ColorOverlay's SourceProxy (qt5compat qgfxsourceproxy.cpp,
    //! same class as MultiEffect's) only repolishes on childrenChanged and
    //! smooth changes - NEVER when the source's layer.enabled flips. An
    //! applet whose proxy chose the direct path while the wrapper was
    //! layered keeps sampling it after the layer is destroyed, and an
    //! opacity-0 item still preprocesses its ShaderEffect node on every
    //! scene repaint (measured live for the task effects, 69baabf0), so
    //! every colorizing disengage armed a dead-provider sampler that warned
    //! per repaint until colorizing re-engaged. visible: false removes the
    //! node; on re-engage the wrapper layer and this gate flip back in the
    //! same frame, ahead of the next render.
    visible: opacity > 0

    //! Qt5-faithful colorizing is ColorOverlay: applyColor painted flat
    //! through the wrapper's alpha, so dark content under a light scheme
    //! becomes light. MultiEffect.colorization is NOT that effect - its
    //! shader multiplies the target color by the source's gray level
    //! (qtdeclarative v6.11.0 multieffect.frag:84), so colorizing dark
    //! text towards a light scheme outputs dark pixels again: a visual
    //! no-op. Both reference forks ported this site to
    //! MultiEffect.colorization and silently lost the feature.
    GraphicalEffects.ColorOverlay {
        id: colorizer
        anchors.fill: parent
        color: colorizerManager.applyColor
        source: wrapper

        //! the applet shadow is a layer EFFECT, not a sibling MultiEffect
        //! sampling this item - same reasoning and shape as ItemWrapper's
        //! shadow site: a sibling redraws the sampled content over the still
        //! visible original with no pixel-exactness guarantee, which
        //! double-struck the colorized text with a shifted ghost copy
        //! (observed live while porting this site). As the layer effect it
        //! REPLACES this item's rendering, so it cannot double-draw.
        layer.enabled: appletItem.environment.isGraphicsSystemAccelerated
                       && Plasmoid.configuration.appletShadowsEnabled
                       && appletColorizer.opacity > 0
        layer.effect: LatteComponents.ShadowedItem {
            shadowColor: appletItem.myView.itemShadow.shadowColor
            shadowSizePx: appletItem.myView.itemShadow.size
            shadowVerticalOffset: forcedShadow ? 0 : 2

            readonly property bool forcedShadow: root.forceTransparentPanel
                                                 && Plasmoid.configuration.appletShadowsEnabled
                                                 && !appletItem.communicator.indexerIsSupported ? true : false
        }
    }
}
