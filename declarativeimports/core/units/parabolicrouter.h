/*
    SPDX-FileCopyrightText: 2026 Bree Spektor
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PARABOLICROUTER_H
#define PARABOLICROUTER_H

// local
#include "parabolicmath.h"

// Qt
#include <QVector>
#include <QtGlobal>

namespace Latte {

//! The parabolic scale-propagation ROUTING as one pure walk (EX-02 in
//! docs/QML_EXTRACTION_PLAN.md; the design section there carries the full
//! semantics inventory read out of the distributed chain). The chain this
//! replaces was synchronous per-item signal recursion; the walk computes
//! the identical outcome in one call. Equality-tested against the real
//! shipped chain driven offscreen (parabolicroutertest's table).
//!
//! Semantics reproduced exactly:
//! - every position outward from the entry is exactly-targeted once while
//!   the stack is live; separators/hidden items are transparent (forward
//!   without consuming); normals consume stack[0];
//! - a bridge client receives the stack AS-RECEIVED and the live walk
//!   STOPS at this level (the client's row routes with this same core and
//!   its overflow re-enters through the existing bridge surface);
//! - an edge spacer targeted with a live stack absorbs
//!   sum of (s-1) over the first spreadSteps entries and re-emits the
//!   clear-tail beyond;
//! - when the stack exhausts to the clear-tail [1], the next position is
//!   exactly-targeted with it (a spacer exactly there absorbs 0, i.e.
//!   clears; further spacers chain) and everything beyond clears via the
//!   broadcast: normals AND transparents apply 1 (the chain's broadcast
//!   arm calls updateScale on separators too - harmless, preserved),
//!   clients receive [1], spacers NOT exactly targeted are untouched
//!   (they have no broadcast arm; the stale-length behavior this leaves
//!   at edges is Qt5-inherited and pinned by the table's stale case).
namespace ParabolicRouter {

enum class ItemKind {
    Normal,      //!< consumes one scale
    Transparent, //!< separator / margins-area separator / hidden: forwards
    EdgeSpacer,  //!< absorbs on exact targeting only
    BridgeClient //!< receives the stack as-is; live walk stops here
};

struct RowItem {
    ItemKind kind = ItemKind::Normal;
    //! spacers only: false when the view alignment makes the spacer inert
    //! (non-centered alignments set length 0 today); the shell owns the
    //! alignment read
    bool absorbing = true;
};

enum class ActionKind { ApplyScale, SpacerAbsorb, ClientHandoff };

struct Action {
    int pos = -1;
    ActionKind kind = ActionKind::ApplyScale;
    double scale = 1.0;             //!< ApplyScale
    double absorbFactor = 0.0;      //!< SpacerAbsorb: length = factor * totals
    QVector<double> stack;          //!< ClientHandoff: as-received
};

struct RouteResult {
    QVector<Action> actions;
    //! stack remaining when the walk left the row edge (the plasmoid twin
    //! exports it through the bridge; the containment twin has no outside)
    QVector<double> overflow;
    //! true when a clear-tail emission happened at an in-row position;
    //! the plasmoid twin exports [1] through the bridge in that case (the
    //! chain's sltTrack* forwarded every in-row clear-tail emission out)
    bool clearTailExported = false;
};

inline bool isClearTail(const QVector<double> &stack)
{
    return stack.size() == 1 && stack.first() == 1.0;
}

//! the terminal clear-tail emission targeting pos: exact-apply at pos
//! (spacers chain their re-emission), then the broadcast beyond
inline void emitClearTail(const QVector<RowItem> &row, int pos, int step, RouteResult &result)
{
    //! spacers exactly targeted absorb 0 and re-emit one position further
    while (pos >= 0 && pos < row.size() && row[pos].kind == ItemKind::EdgeSpacer) {
        result.clearTailExported = true;
        result.actions.append({pos, ActionKind::SpacerAbsorb, 1.0, 0.0, {}});
        pos += step;
    }

    if (pos < 0 || pos >= row.size()) {
        //! the emission left the row (or the row ended in spacers)
        result.overflow = {1.0};
        return;
    }

    result.clearTailExported = true;

    //! exact match at pos plus the broadcast beyond it, one pass: normals
    //! and transparents apply 1, clients receive [1], spacers beyond are
    //! never touched
    for (int p = pos; p >= 0 && p < row.size(); p += step) {
        switch (row[p].kind) {
        case ItemKind::Normal:
        case ItemKind::Transparent:
            result.actions.append({p, ActionKind::ApplyScale, 1.0, 0.0, {}});
            break;
        case ItemKind::BridgeClient:
            result.actions.append({p, ActionKind::ClientHandoff, 1.0, 0.0, {1.0}});
            break;
        case ItemKind::EdgeSpacer:
            break;
        }
    }
}

//! route a scale stack entering the row at entryPos, traveling by step
//! (+1 toward higher indexes, -1 toward lower). spreadSteps bounds the
//! spacer absorption window ((spread-1)/2, the spacer's hiddenItemsCount).
inline RouteResult routeStack(const QVector<RowItem> &row, int entryPos, int step,
                              QVector<double> stack, int spreadSteps)
{
    RouteResult result;

    if (stack.isEmpty()) {
        return result;
    }

    int pos = entryPos;

    while (pos >= 0 && pos < row.size()) {
        if (stack.isEmpty()) {
            //! the chain drops empty stacks on match (newScales.length<=0);
            //! unreachable from applyParabolicEffect's 1-terminated stacks
            return result;
        }

        if (isClearTail(stack)) {
            emitClearTail(row, pos, step, result);
            return result;
        }

        const RowItem &item = row[pos];

        switch (item.kind) {
        case ItemKind::Normal:
            result.actions.append({pos, ActionKind::ApplyScale, stack.first(), 0.0, {}});
            stack.removeFirst();
            break;
        case ItemKind::Transparent:
            break;
        case ItemKind::BridgeClient:
            result.actions.append({pos, ActionKind::ClientHandoff, 1.0, 0.0, stack});
            return result;
        case ItemKind::EdgeSpacer: {
            double factor = 0.0;
            if (item.absorbing) {
                const int entries = qMin(spreadSteps, static_cast<int>(stack.size()));
                for (int i = 0; i < entries; ++i) {
                    factor += stack.at(i) - 1.0;
                }
            }
            result.actions.append({pos, ActionKind::SpacerAbsorb, 1.0, factor, {}});
            emitClearTail(row, pos + step, step, result);
            return result;
        }
        }

        pos += step;
    }

    //! walk left the row edge with the stack still live
    result.overflow = stack;
    return result;
}

//! both directions from the hovered position, from EX-03's stacks
struct Assignment {
    RouteResult lower;
    RouteResult higher;
};

inline Assignment assignScales(const QVector<RowItem> &row, int hoveredPos,
                               const ParabolicMath::ScaleStacks &stacks, int spreadSteps)
{
    Assignment a;
    a.lower = routeStack(row, hoveredPos - 1, -1, stacks.left, spreadSteps);
    a.higher = routeStack(row, hoveredPos + 1, +1, stacks.right, spreadSteps);
    return a;
}

}
}

#endif
