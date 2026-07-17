/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-FileCopyrightText: 2026 Bree Spektor
    SPDX-FileCopyrightText: 2026 Latte Dock contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "tools.h"

// local
#include "units/colortools.h"

// KNOWN BENIGN SOURCE of the refusals below, root-caused 2026-07-16 with a
// V4 caller trace (session-handoff has the method): during item creation the
// first evaluation of bindings reading Kirigami.Theme colors (directly, or
// through the colorizer chain they feed) can run before the attached
// PlatformTheme has resolved its palette, so the getter hands over a
// default-constructed invalid QColor. Every traced consumer is a live
// binding, so the theme's change notify recomputes it with the real color a
// beat later - the refusal's fallback value is only ever a first-evaluation
// interim. Expect a burst of these per view creation under --debug; a
// STEADY stream at idle is NOT this and deserves a fresh hunt.

namespace Latte{

Tools::Tools(QObject *parent)
    : QObject(parent)
{
}

double Tools::colorBrightness(QColor color)
{
    if (!color.isValid()) {
        qCritical("Tools.colorBrightness: invalid color from QML, returning 0 (dark)");
        return 0.0;
    }

    return ColorTools::colorBrightness(color);
}

double Tools::colorLumina(QColor color)
{
    if (!color.isValid()) {
        qCritical("Tools.colorLumina: invalid color from QML, returning 0 (dark)");
        return 0.0;
    }

    return ColorTools::colorLumina(color);
}

bool Tools::isLight(QColor color, double threshold)
{
    if (!color.isValid()) {
        qCritical("Tools.isLight: invalid color from QML, returning false (dark)");
        return false;
    }

    return ColorTools::isLight(color, threshold);
}

}
