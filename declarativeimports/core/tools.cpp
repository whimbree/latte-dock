/*
    SPDX-FileCopyrightText: 2020 Michail Vourlakos <mvourlakos@gmail.com>
    SPDX-FileCopyrightText: 2026 Bree Spektor
    SPDX-FileCopyrightText: 2026 Latte Dock contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "tools.h"

// local
#include "units/colortools.h"

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
