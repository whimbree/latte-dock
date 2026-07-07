# Live-only test registry

Per `docs/TESTING.md`, a unit that genuinely cannot be verified headlessly
gets an entry here stating *why*, and becomes a target of live verification
(per-phase live testing, and the Phase 10 e2e harness). The registry exists so
each gap is recorded instead of papered over with a dishonest test.

## Entries

### Wheel delivery into the tasks plasmoid and its badges
- **What:** scrolling over a task cycles/activates its windows; scrolling over
  an audio badge on a task changes that application's volume.
- **Why live-only:** delivery of wheel events from the containment into the
  plasmoid interior depends on the Phase 8 containment wheel bridge (not built
  yet), and the audio-badge path additionally needs a real window that is
  playing audio bound to a task. Neither is reproducible in an offscreen QML
  engine. The in-plasmoid routing itself (badge sub-region winning over the
  whole-task handler) is already correct by z-order and does not need a live
  test; only the cross-boundary delivery does.
- **Verify at:** Phase 8 (wheel bridge), and again in the Phase 10 e2e sweep.
- **Related:** `docs/REVIEW_NOTES.md` "In-plasmoid wheel routing".

### Phase 7 widget management / drag-drop / edit mode (whole subsystem)
- **What:** widget removal actually removes; opening the widget explorer;
  adding a widget by drag and by tap (including the Latte Tasks widget that
  crashed latte-dock-qt6); drag-to-reorder without jitter or stuck icons;
  edit-mode entry/exit detection; hover-zoom smoothness.
- **Why live-only:** every one of these is a pointer gesture (drag, tap,
  reorder) or a perceptual property (jitter, smoothness) that cannot be driven
  or judged in an offscreen engine, and wayland has no scriptable drag-drop.
  The code is in place (widget removal fixed in C++; the add path swapped to
  the confirmed-working fork's QML; reorder/edit-mode already on the
  recommended base), but "done" per the plan's cadence needs a human to drive
  each interaction.
- **Verify at:** the next hands-on session, and again in the Phase 10 e2e
  sweep.
- **Related:** `docs/REVIEW_NOTES.md` "Phase 7 needs a hands-on session".
