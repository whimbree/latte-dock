# Human review notes

Things done during the autonomous port that a human should look at when
convenient. Each entry says what was done, why, and what specifically to
check. Nothing here is known-broken - these are judgment calls, transcribed
data, or work that can only be finished/verified with a live session or a
decision the driver shouldn't make alone.

## Open

### Tasks click-action completeness test uses a transcribed "offered" set
`tests/qml/tst_taskactions.qml` guards the enum/handler contract for task
click actions (the plan's "config offers 9, handler handles 3" regression
class). The set of enum values each click combo *offers* is transcribed into
the test from the config UIs, with a source reference on each list:

- left click: `shell/.../configuration/pages/TasksConfig.qml` leftClickAction combo (3 values)
- middle / modifier click: same file, middle/modifierClickAction combos (6 values, index == enum)
- wheel: same file, wheelAction combo (3 values)
- the plasmoid's own `ConfigInteraction.qml` middle-click combo offers a 4-value subset

**Review:** confirm the transcribed lists still match the combo `model:`
arrays. If a combo gains an option, the transcription here must gain it too -
the test only catches a *handler* that fell behind the offered set, not a
*test* that fell behind the UI. A nice future hardening would be to drive the
real combo components and read their `model` arrays directly, but those config
pages have a large ambient-context surface that isn't cheaply mockable yet.

### In-plasmoid wheel routing: audited, no fix needed, one part deferred to Phase 8
Plan item (Phase 6): "Route wheel events to badges/sub-regions inside the
tasks plasmoid explicitly ... DropArea blocks wheel event delivery in Qt6".

Audit result - the current structure is already correct, so no routing code
was added:

- **Sub-region routing (badge vs task) is correct by z-order.** The audio
  badge's `MouseArea` lives inside the icon's `Flow` at `z:10`
  (`declarativeimports/abilities/items/BasicItem.qml:285`); `TaskMouseArea` is
  a plain child at default `z:0`. So a wheel over the badge hits the badge
  handler (volume) and a wheel anywhere else on the task falls through the
  visual-only icon items to `TaskMouseArea` (cycle). No explicit
  hit-test-and-route is needed.
- **The DropArea does not block the tasks' wheel.** `MouseHandler` (which
  holds the `preventStealing` `DropArea`) is declared *before* `ScrollableList`
  in `plasmoid/.../ui/main.qml`, so the task list stacks above it; wheel over a
  task reaches `TaskMouseArea`, not the DropArea beneath.

**Deferred to Phase 8 (live verification):** whether the containment actually
delivers wheel events *into* the tasks plasmoid depends on the Phase 8
containment wheel bridge, which does not exist yet. Once it lands, verify on a
live session that (a) scrolling over a task cycles/activates and (b) scrolling
over an audio badge on a task changes that app's volume. This is a live-only
check (needs a real audio-playing window on a task); it is recorded in
`docs/testing/live-only.md`.

## Resolved
(none yet)
