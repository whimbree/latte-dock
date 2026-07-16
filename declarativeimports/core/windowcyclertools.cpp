/*
    SPDX-FileCopyrightText: 2026 Bree Spektor
    SPDX-FileCopyrightText: 2026 Latte Dock contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "windowcyclertools.h"

// local
#include "units/windowcycler.h"

// Qt
#include <QDebug>
#include <QVariantMap>

// C++
#include <optional>

namespace Latte {

WindowCyclerTools::WindowCyclerTools(QObject *parent)
    : QObject(parent)
{
}

namespace {

//! The Qt5 shells carried -1 in lastActiveWinInGroup for "none"; it dies
//! here. Safe because a real id can never read "-1": X11 XIDs are unsigned
//! decimals, wayland ids are UUID strings (8e8cdf31).
std::optional<QString> toLastActiveWinId(const QVariant &lastActiveWinId)
{
    if (!lastActiveWinId.isValid()) {
        return std::nullopt;
    }

    const QString id = lastActiveWinId.toString();
    if (id.isEmpty() || id == QLatin1String("-1")) {
        return std::nullopt;
    }

    return id;
}

//! nullopt = malformed input (the caller refuses loudly). A missing key is
//! a shell bug, never something to select around silently.
std::optional<QList<WindowCycler::GroupWindow>> toGroupWindows(const QVariantList &windows,
                                                               const char *caller)
{
    QList<WindowCycler::GroupWindow> coreWindows;
    coreWindows.reserve(windows.size());

    for (const QVariant &entry : windows) {
        const QVariantMap map = entry.toMap();
        if (!map.contains(QStringLiteral("winId"))
                || !map.contains(QStringLiteral("isActive"))
                || !map.contains(QStringLiteral("isMinimized"))) {
            qCritical() << caller << ": malformed window entry" << entry
                        << "- refusing to select";
            return std::nullopt;
        }

        coreWindows.append(WindowCycler::GroupWindow{
            map.value(QStringLiteral("winId")).toString(),
            map.value(QStringLiteral("isActive")).toBool(),
            map.value(QStringLiteral("isMinimized")).toBool()});
    }

    return coreWindows;
}

}

int WindowCyclerTools::selectNext(const QVariantList &windows, const QVariant &lastActiveWinId) const
{
    const auto coreWindows = toGroupWindows(windows, "WindowCycler.selectNext");
    if (!coreWindows) {
        return -1;
    }

    return WindowCycler::selectNext(*coreWindows, toLastActiveWinId(lastActiveWinId)).value_or(-1);
}

int WindowCyclerTools::selectPrevious(const QVariantList &windows, const QVariant &lastActiveWinId) const
{
    const auto coreWindows = toGroupWindows(windows, "WindowCycler.selectPrevious");
    if (!coreWindows) {
        return -1;
    }

    return WindowCycler::selectPrevious(*coreWindows, toLastActiveWinId(lastActiveWinId)).value_or(-1);
}

int WindowCyclerTools::selectMinimizeTarget(const QVariantList &windows, const QVariant &lastActiveWinId) const
{
    const auto coreWindows = toGroupWindows(windows, "WindowCycler.selectMinimizeTarget");
    if (!coreWindows) {
        return -1;
    }

    return WindowCycler::selectMinimizeTarget(*coreWindows, toLastActiveWinId(lastActiveWinId)).value_or(-1);
}

QVariantList WindowCyclerTools::flattenTasksForCycling(const QVariantList &entries) const
{
    QList<WindowCycler::TaskEntry> coreEntries;
    coreEntries.reserve(entries.size());

    for (const QVariant &entry : entries) {
        const QVariantMap map = entry.toMap();
        bool childCountIsInt = false;
        const int childCount = map.value(QStringLiteral("childCount")).toInt(&childCountIsInt);

        if (!map.contains(QStringLiteral("isLauncher"))
                || !map.contains(QStringLiteral("isStartup"))
                || !map.contains(QStringLiteral("isGroupParent"))
                || !childCountIsInt || childCount < 0) {
            qCritical() << "WindowCycler.flattenTasksForCycling: malformed task entry" << entry
                        << "- refusing to flatten";
            return QVariantList();
        }

        coreEntries.append(WindowCycler::TaskEntry{
            map.value(QStringLiteral("isLauncher")).toBool(),
            map.value(QStringLiteral("isStartup")).toBool(),
            map.value(QStringLiteral("isGroupParent")).toBool(),
            childCount});
    }

    QVariantList positions;
    for (const WindowCycler::TaskPosition &position : WindowCycler::flattenTasksForCycling(coreEntries)) {
        QVariantMap map;
        map.insert(QStringLiteral("row"), position.row);
        //! QML keeps the -1-means-none convention at the boundary only
        map.insert(QStringLiteral("childRow"), position.childRow.value_or(-1));
        positions.append(map);
    }

    return positions;
}

int WindowCyclerTools::selectAdjacentTask(int count, int activeIndex, bool next) const
{
    if (count < 0 || activeIndex < -1 || activeIndex >= count) {
        qCritical() << "WindowCycler.selectAdjacentTask: out-of-range input, count" << count
                    << "activeIndex" << activeIndex << "- refusing to select";
        return -1;
    }

    const std::optional<int> active = (activeIndex >= 0) ? std::optional<int>(activeIndex)
                                                         : std::nullopt;
    const auto direction = next ? WindowCycler::CycleDirection::Next
                                : WindowCycler::CycleDirection::Previous;

    return WindowCycler::selectAdjacentTask(count, active, direction).value_or(-1);
}

}
