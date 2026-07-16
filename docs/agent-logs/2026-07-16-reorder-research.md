# Agent log: drag-reorder implementation research (2026-07-16)

Dispatched by the stabilization session (Phase 7 cluster) to compare
drag-to-reorder across the port and both reference forks, per the plan
item's instruction to read latte-dock-qt6's actual reorder source
before touching ours. Read-only dispatch; this log written by the
orchestrator from the agent's returned analysis (the agent predates
the agent-log rule by an hour).

## What it read

- Port: containment/package/contents/ui/editmode/ConfigOverlay.qml,
  containment/plugin/layoutmanager.cpp (insertBefore/insertAfter/
  insertAtCoordinates), plasmoid MouseHandler.qml,
  declarativeimports/core/units/dropclassifier.h.
- qt6 fork: same files in their tree, plus their tools.js.
- ng fork: AppletItem.qml sort-drag DragHandler + 16ms poll timer,
  their MouseHandler.qml, the four tuning commits (84e7c9d10,
  c4e7bcb62, 924a8ac41, cf6aa1ec0).

## Verdict (full analysis in the Phase 7 plan item)

- NO CHANGE NEEDED: the port's applet reorder is upstream Qt5's
  placeholder design, algorithmically identical to the qt6 fork's
  (which is why theirs feels clean - they left it alone), and strictly
  better in the two spots they differ (fractional drag coords vs their
  int truncation; repositionable LatteCore.Dialog handle tooltip vs
  their PlasmaCore.Dialog).
- ng's jitter is their own live-item feedback loop (insertBefore on
  the DRAGGED item itself, re-evaluated by a 16ms poll) suppressed by
  stacked dead zones/hysteresis/cooldowns. Never import those knobs.
- Hysteresis-by-construction: after a swap the pointer sits inside the
  placeholder and childAt returns the placeholder, so decisions
  self-suppress. Swaps are instant (Grid has no move transition), so
  no mid-animation re-trigger window exists.
- Two follow-ups surfaced: (a) the `target.animating` guard in the
  port's MouseHandler.qml:130 is dead code in all three trees (icList
  defines no such property; reads undefined); (b) task reorder sets
  dragSource.z = 100 and NO tree ever resets it - prime suspect for
  the "icons stuck behind other elements after repeated reorder" plan
  item; the applet path DOES reset (currentApplet.z = 1).
