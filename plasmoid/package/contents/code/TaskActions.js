/*
    SPDX-FileCopyrightText: 2026 Bree Spektor
    SPDX-License-Identifier: GPL-2.0-or-later
*/

//! Single source of truth for mapping a configured TaskAction / TaskScrollAction
//! enum value onto the operation the click handler performs. TaskMouseArea's
//! click/scroll handlers dispatch through these functions rather than parallel
//! if-chains, so a config combo can never offer an enum value the handler
//! silently ignores (the bug latte-dock-ng shipped: 9 offered, 3 handled).
//! tests/qml/tst_taskactions.qml pins that every value the config UI offers per
//! click type resolves to a real (non-empty) command here.
//!
//! Tokens are plain strings the QML executor switches on; "" means "no
//! operation" and is the legitimate result only for NoneAction.

.pragma library

.import org.kde.latte.private.tasks 0.1 as LatteTasks

//! The five window-operation actions plus NoneAction that the middle-click and
//! modifier-click combos share. Returns "" for NoneAction (a real no-op) and
//! for any value not in the offered set.
function standardCommandFor(action) {
    switch (action) {
    case LatteTasks.Types.Close:            return "close";
    case LatteTasks.Types.NewInstance:      return "newInstance";
    case LatteTasks.Types.ToggleMinimized:  return "toggleMinimized";
    case LatteTasks.Types.CycleThroughTasks:return "cycleOrActivate";
    case LatteTasks.Types.ToggleGrouping:   return "toggleGrouping";
    case LatteTasks.Types.NoneAction:       return "";
    default:                                return "";
    }
}

//! The three actions the left-click combo offers. Its execution is more
//! nuanced than the standard set (present/preview depend on group + compositor
//! state, handled in QML), so this only classifies the recognized action.
function leftCommandFor(action) {
    switch (action) {
    case LatteTasks.Types.PresentWindows:    return "present";
    case LatteTasks.Types.CycleThroughTasks: return "cycleOrActivate";
    case LatteTasks.Types.PreviewWindows:    return "preview";
    default:                                 return "";
    }
}

//! The three values the wheel/scroll combo offers.
function scrollCommandFor(scrollAction) {
    switch (scrollAction) {
    case LatteTasks.Types.ScrollNone:            return "";
    case LatteTasks.Types.ScrollTasks:           return "scrollTasks";
    case LatteTasks.Types.ScrollToggleMinimized: return "scrollToggleMinimized";
    default:                                     return "";
    }
}
