# QML extraction plan

Planning artifact for moving behavioral logic out of QML into
strongly-typed, unit-testable C++. Written 2026-07-15 against HEAD
5e1c2b12 by the strong-model planning session named in
docs/prompts/qml-extraction-planning-prompt.md; every commit hash, file
path, and line range cited below was verified with git/grep during that
session. This plan executes across a model transition: specs tagged
`delegate-safe` are written to be executable cold by a weaker model;
specs tagged `strong-model-only` must land inside the remaining
strong-model window or be deferred, never delegated.

Posture (decided in CLAUDE.md as of ce94bb1d, not relitigated here):
maintained continuation. Upstream mergeability is not a constraint.
Non-negotiable: small bisectable commits, Qt5 behavioral fidelity per
extraction (the Qt5 source is in our own history at tag f0ad7b23,
v0.10.8: `git show f0ad7b23:<path>`), re-implementation with
understanding. CaptSilver/latte-dock-qt6 (reviewed through 81384003) is
a blueprint of WHAT to extract and WHICH invariants to pin, never a
source to paste.

## Completeness ledger

Kept current as sections land. A PENDING spec names its scope in one
line; DONE means the full spec is written to the section-C template.

Inventory (section A):
- [x] containment/ - 62 QML files classified
- [x] plasmoid/ - 36 QML files classified
- [x] shell/ - 28 QML files classified
- [x] indicators/ - 9 QML files classified
- [x] declarativeimports/ - 104 QML files classified
- [x] JS logic files addendum - 11 files

Ranking (section B): [x] done.

Per-unit specs (section C), in rank order:
- [ ] EX-01 PreviewSwitchEngine - preview adoption/debounce/LRU decision core
- [ ] EX-02 ParabolicRouter - neighbor scale-stack propagation chains
- [ ] EX-03 ParabolicMathCore - the zoom curve math
- [ ] EX-04 AutoSizeEngine - iconSize shrink/grow feedback loop
- [ ] EX-05 FillLengthDistributor - Justify/fill two-pass space distribution
- [ ] EX-06 VisibleIndexEngine - visible-index math + separator neighbor walks
- [ ] EX-07 StorageIdRemapper - layout-file id remapping (capt blueprint)
- [ ] EX-08 ScreenGeometryCalculator - available screen rect/region (capt blueprint)
- [ ] EX-09 PositionerGeometry - view sizing/placement math (capt blueprint)
- [ ] EX-10 MaskInputGeometry - visibility mask + input region rect math
- [ ] EX-11 LauncherListOps - launcher order algebra, registries, stored-list parsing
- [ ] EX-12 ColorizerDecisionCore - applyTheme/scheme selection tree
- [ ] EX-13 ViewTypeAndBackgroundPredicates - Panel-vs-Dock chain + background states
- [ ] EX-14 DropEventClassifier - drag mime classification + insert index
- [ ] EX-15 WheelAccumulator - wheel delta accumulation/threshold semantics
- [ ] EX-16 GroupWindowCycler - next/previous/minimize target selection
- [ ] EX-17 TooltipTextComposer - preview title/subtext string transforms
- [ ] EX-18 LengthOffsetClamp - maxLength/offset mutual clamp (dedup)
- [ ] EX-19 ColorLuminance - shared brightness/luminance helpers (dedup)
- [ ] EX-20 BadgeMath - badge parsing, proportion, arc geometry
- [ ] EX-21 ScrollOverflowMath - scrollable list overflow/autoscroll math
- [ ] EX-22 ActivitySetAlgebra - activity set filtering (capt blueprint)
- [ ] EX-23 WindowTrackingPredicates - window predicate + extra-view-hints pass (capt blueprint)
- [ ] EX-24 IconSourceClassifier - icon source classification (capt blueprint)
- [ ] EX-25 PanelBackgroundScan - panel background scanline math (capt blueprint)

Section D (coverage + ratchet): [ ] pending.
Section E (waves): [ ] pending.
Section F (risks + non-goals): [ ] pending.
Executive summary: [ ] pending.
PORTING_PLAN cross-reference item: [ ] pending.

## Method

- Extraction shape (the capt shape, proven in our own tree by
  d12baff2's `declarativeimports/core/dialog.cpp` seam and by
  `containment/plugin/layoutmanager.cpp`): a pure core - plain value
  structs in, plain values out, no QObject/scenegraph/binding
  dependencies - plus, where QML must call it, a thin registered
  wrapper in the subsystem's existing C++ plugin. Tests include the
  core header directly; the existing qmltest/contract harness keeps
  driving the real shipped QML for the thin-shell wiring.
- Cutover per unit, never two live copies: the commit that lands the
  C++ core also switches the QML call site to it and deletes the QML
  body. Bisectability is the rollback story; a revert of one commit
  restores the QML logic wholesale.
- Qt5 fidelity per unit: each spec names the f0ad7b23 file the C++
  must match. Where the port already fixed a Qt5-era defect (e.g.
  ad9b823f's loop termination), the spec says which behavior wins and
  why; everything else matches Qt5 exactly, tested by cases derived
  from reading the Qt5 body at execution time.
- No bandaids carried over: where the QML logic "works" via a polling
  timer, silent early-return, or value-hiding clamp, the spec flags it
  as a defect to fix during extraction. The known inventory of
  assessed silent guards is in docs/session-handoff.md (the 2026-07-15
  loops/degenerate-indexes sweep); specs reference it rather than
  re-litigating each guard.

## A. QML logic inventory

Classification taxonomy: geometry-math / state-machine / ordering /
model-transform / event-routing / pure-presentation. Size is the
extractable behavioral-logic volume: S under 40 lines, M 40-150, L
over 150. Verdict BEHAVIORAL means extraction candidate (section B/C
decides extract vs pin-in-place); PRESENTATIONAL means leave in QML.
File classifications were produced by four read-only inventory
subagent sweeps over every file; function names quoted here were
re-verified by grep in the main session wherever a spec cites them.
Line numbers appear only in section C, individually verified.

### containment/ (62 files, 13159 lines)

Behavioral files:

| File (containment/package/contents/ui/) | Lines | Categories | Size |
| --- | --- | --- | --- |
| main.qml | 1232 | geometry-math, state-machine, event-routing, ordering | L |
| VisibilityManager.qml | 661 | geometry-math, state-machine | L |
| BindingsExternal.qml | 399 | geometry-math, model-transform | L (borderline; most bindings are passthrough) |
| DragDropArea.qml | 206 | event-routing, state-machine | M |
| applet/AppletItem.qml | 1122 | geometry-math, state-machine, ordering, event-routing | L |
| applet/ItemWrapper.qml | 742 | geometry-math, state-machine | L |
| applet/ParabolicArea.qml | 244 | geometry-math, event-routing | L |
| applet/EventsSink.qml | 203 | geometry-math, event-routing | M (borderline) |
| applet/HiddenSpacer.qml | 72 | geometry-math | S (borderline) |
| applet/IndicatorLevel.qml | 77 | geometry-math, event-routing | S (borderline) |
| applet/ShortcutBadge.qml | 102 | model-transform | S (borderline) |
| applet/communicator/Actions.qml | 56 | event-routing | S (borderline) |
| abilities/AutoSize.qml | 258 | geometry-math, state-machine, event-routing | L |
| abilities/Metrics.qml | 103 | geometry-math | M |
| abilities/ParabolicEffect.qml | 38 | geometry-math | S (borderline) |
| abilities/Layouter.qml | 72 | state-machine, event-routing | S |
| abilities/Indexer.qml | 64 | ordering, model-transform | M |
| abilities/Animations.qml | 53 | state-machine | S (borderline) |
| abilities/PositionShortcuts.qml | 52 | event-routing | S |
| abilities/Indicators.qml | 125 | model-transform | M (borderline; capability probing) |
| abilities/privates/IndexerPrivate.qml | 312 | ordering, model-transform | L |
| abilities/privates/LayouterPrivate.qml | 440 | geometry-math, ordering | L |
| abilities/privates/layouter/AppletsContainer.qml | 214 | model-transform, ordering | L |
| abilities/privates/MetricsPrivate.qml | 144 | geometry-math | M |
| abilities/privates/ParabolicEffectPrivate.qml | 158 | state-machine, event-routing | L |
| abilities/privates/AnimationsPrivate.qml | 64 | model-transform | S |
| abilities/privates/LaunchersPrivate.qml | 113 | model-transform | M (borderline; duplicated 3-layout scans) |
| abilities/privates/MyViewPrivate.qml | 95 | model-transform, state-machine | M (borderline) |
| abilities/privates/PositionShortcutsPrivate.qml | 78 | model-transform | S (borderline) |
| abilities/privates/ThinTooltipPrivate.qml | 56 | model-transform | S (borderline) |
| background/MultiLayered.qml | 951 | geometry-math, state-machine | L (the ~300-line states block is presentational) |
| background/types/Paddings.qml | 40 | geometry-math | S (borderline) |
| background/types/Shadows.qml | 49 | geometry-math | S (borderline) |
| background/types/Totals.qml | 51 | geometry-math | S (borderline) |
| colorizer/Manager.qml | 206 | state-machine, model-transform | L |
| colorizer/CustomBackground.qml | 237 | geometry-math | M |
| editmode/ConfigOverlay.qml | 564 | geometry-math, state-machine, event-routing, ordering | L |
| layouts/LayoutsContainer.qml | 537 | geometry-math, state-machine, event-routing | L |
| layouts/EnvironmentActions.qml | 360 | geometry-math, event-routing | L |
| layouts/ParabolicEdgeSpacer.qml | 122 | geometry-math, event-routing | M |
| layouts/loaders/Tasks.qml | 102 | ordering, model-transform | M |

Presentational (leave in QML): abilities/Launchers.qml (thin proxy to
layoutsManager.syncedLaunchers), abilities/MyView.qml, abilities/Debug.qml,
abilities/ThinTooltip.qml, abilities/UserRequests.qml,
abilities/privates/IndicatorsPrivate.qml (mirror bindings),
abilities/privates/metrics/Fraction.qml, applet/PaddingsInConfigureApplets.qml,
applet/TitleTooltipParent.qml, applet/EventsSinkOriginArea.qml,
applet/colorizer/Applet.qml, applet/communicator/LatteBridge.qml,
applet/communicator/Engine.qml (logic lives in AppletIdentifier.js),
background/BackgroundProperties.qml, colorizer/KirigamiShadowedRectangle.qml,
colorizer/NormalRectangle.qml, debugger/DebugWindow.qml (861 lines of
read-only diagnostics display), debugger/Tag.qml, layouts/AppletsContainer.qml,
Upgrader.qml (one-shot v0.10 config migration), ContextMenuLayer.qml.

### plasmoid/ (36 files, 9858 lines)

Behavioral files:

| File (plasmoid/package/contents/ui/) | Lines | Categories | Size |
| --- | --- | --- | --- |
| main.qml | 1698 | state-machine, model-transform, ordering, geometry-math, event-routing | L |
| TasksExtendedManager.qml | 404 | model-transform, ordering, state-machine | L |
| PulseAudio.qml | 127 | model-transform, geometry-math | M |
| ContextMenu.qml | 909 | model-transform, event-routing, ordering | L |
| task/TaskItem.qml | 996 | state-machine, model-transform, geometry-math, event-routing | L |
| task/TaskIcon.qml | 593 | state-machine, pure-presentation | M (borderline; effect gating) |
| task/TaskMouseArea.qml | 375 | event-routing, state-machine | L |
| task/SubWindows.qml | 310 | model-transform, ordering, state-machine | L |
| task/AudioStream.qml | 147 | event-routing, geometry-math | M |
| task/ProgressOverlay.qml | 114 | model-transform | S (borderline) |
| task/animations/RealRemovalAnimation.qml | 226 | state-machine, ordering, event-routing | M |
| task/animations/ShowWindowAnimation.qml | 201 | state-machine, ordering | M |
| task/animations/RemoveWindowFromGroupAnimation.qml | 147 | state-machine, geometry-math | S |
| task/animations/NewWindowAnimation.qml | 104 | state-machine | S |
| task/animations/LauncherAnimation.qml | 113 | state-machine | S |
| taskslayout/ScrollableList.qml | 382 | geometry-math, ordering, state-machine | M |
| taskslayout/MouseHandler.qml | 264 | event-routing, ordering, model-transform | M |
| previews/ToolTipInstance.qml | 524 | model-transform, geometry-math, event-routing | L |
| previews/ToolTipDelegate2.qml | 234 | model-transform, geometry-math, event-routing | M |
| previews/ToolTipWindowMouseArea.qml | 51 | event-routing | S |
| abilities/Launchers.qml | 404 | model-transform, ordering, event-routing, state-machine | L |
| abilities/launchers/Validator.qml | 137 | ordering, model-transform | M |
| abilities/launchers/Syncer.qml | 109 | event-routing, state-machine | S |

Presentational (leave in QML): task/animations/launcher/BounceAnimation.qml,
task/animations/newwindow/BounceAnimation.qml,
task/animations/ClickedAnimation.qml, taskslayout/ScrollEdgeShadows.qml,
taskslayout/ScrollOpacityMask.qml, taskslayout/ScrollPositioner.qml,
previews/PipeWireThumbnail.qml, previews/PlasmaCoreThumbnail.qml,
AppletAbilities.qml, config/ConfigAppearance.qml (index-value combo
mapping is UI-local), config/ConfigInteraction.qml, config/ConfigPanel.qml,
and plasmoid/package/contents/config/config.qml.

### shell/ (28 files, 7720 lines)

Behavioral files:

| File (shell/package/contents/) | Lines | Categories | Size |
| --- | --- | --- | --- |
| applet/CompactApplet.qml | 444 | state-machine, geometry-math, event-routing | L |
| configuration/CanvasConfiguration.qml | 190 | geometry-math, event-routing | M |
| configuration/LatteDockConfiguration.qml | 656 | geometry-math, model-transform, state-machine, event-routing | L |
| configuration/canvas/HeaderSettings.qml | 170 | geometry-math | M |
| configuration/canvas/maxlength/RulerMouseArea.qml | 77 | geometry-math, event-routing | M |
| configuration/canvas/maxlength/Ruler.qml | 324 | geometry-math | M |
| configuration/pages/AppearanceConfig.qml | 1232 | geometry-math, model-transform, event-routing | L |
| configuration/pages/BehaviorConfig.qml | 937 | model-transform, ordering, event-routing | M |
| controls/CustomIndicatorButton.qml | 215 | model-transform, event-routing, state-machine | L |
| controls/CustomVisibilityModeButton.qml | 129 | model-transform, ordering, event-routing | M |
| controls/DragCorner.qml | 167 | geometry-math, event-routing | M |
| controls/IndicatorConfigUiManager.qml | 135 | state-machine, ordering, event-routing | M |
| controls/TypeSelection.qml | 136 | model-transform, event-routing | M |
| views/AppletDelegate.qml | 229 | event-routing | S |
| views/Panel.qml | 128 | state-machine, event-routing | M |
| views/WidgetExplorer.qml | 537 | model-transform, event-routing, state-machine | L |

Presentational (leave in QML): configuration/config.qml,
configuration/LatteDockSecondaryConfiguration.qml,
configuration/canvas/SettingsOverlay.qml,
configuration/canvas/controls/Button.qml, GraphicIcon.qml,
RearrangeIcon.qml, StickIcon.qml, configuration/pages/EffectsConfig.qml
(control-to-config plumbing), configuration/pages/TasksConfig.qml
(control-to-config plumbing; the Plasma 6 config-access route it uses
is pinned by 32df5b47), controls/InnerShadow.qml,
explorer/AppletAlternatives.qml (the 56549d73 package-local copy;
deliberately kept a minimal-diff mirror of plasma-desktop's file),
views/InfoView.qml.

### indicators/ (9 files, 1513 lines)

Behavioral: default/package/ui/main.qml (318 lines; geometry-math,
state-machine; L - mask thickness math, W3C luminance color selection,
line-style grow/shrink animation state machine),
org.kde.latte.plasma/package/ui/FrontLayer.qml (267 lines;
geometry-math, event-routing; M - clicked-animation radius math,
per-edge press-coordinate conversion),
org.kde.latte.plasma/package/ui/main.qml (145 lines; model-transform,
geometry-math; M - progress clip math, SVG prefix arrays).

Presentational: default/package/config/config.qml (percent-conversion
plumbing; carries the 33fa17d7 latteIndicator alias),
org.kde.latte.plasma/package/config/config.qml,
org.kde.latte.plasma/package/ui/AppletBackLayer.qml,
org.kde.latte.plasma/package/ui/TaskBackLayer.qml,
org.kde.latte.plasmatabstyle/package/ui/BackLayer.qml,
org.kde.latte.plasmatabstyle/package/ui/main.qml.

### declarativeimports/ (104 files, 7671 lines)

declarativeimports/core is C++ only (no QML). Behavioral files:

| File (declarativeimports/) | Lines | Categories | Size |
| --- | --- | --- | --- |
| components/ComboBox.qml | 476 | model-transform, event-routing, geometry-math | M |
| components/BadgeText.qml | 179 | geometry-math, event-routing | M |
| components/Slider.qml | 127 | geometry-math | S |
| components/SpriteRectangle.qml | 113 | geometry-math | M |
| components/GlowPoint.qml | 344 | geometry-math | S |
| components/ComboBoxButton.qml | 157 | event-routing | S |
| components/TextField.qml | 129 | state-machine, geometry-math | S |
| components/IndicatorItem.qml | 138 | state-machine | S |
| components/ShadowedItem.qml | 50 | geometry-math (delegates to code/EffectMath.js) | S |
| abilities/bridge/PositionShortcuts.qml | 53 | event-routing | S |
| abilities/bridge/ParabolicEffect.qml | 42 | event-routing | S |
| abilities/bridge/Launchers.qml | 37 | event-routing | S |
| abilities/client/Indexer.qml | 243 | ordering, model-transform | L |
| abilities/client/ParabolicEffect.qml | 201 | event-routing, state-machine | M |
| abilities/client/indicators/LatteIndicator.qml | 307 | geometry-math, state-machine | M |
| abilities/client/AppletAbilities.qml | 152 | model-transform, ordering | S |
| abilities/client/PositionShortcuts.qml | 69 | ordering | S |
| abilities/client/UserRequests.qml | 48 | event-routing | S |
| abilities/client/Requirements.qml | 62 | event-routing | S |
| abilities/definition/ParabolicEffect.qml | 82 | geometry-math, ordering | M (the parabolic math core) |
| abilities/definition/animations/Tracker.qml | 26 | model-transform | S |
| abilities/host/ThinTooltip.qml | 135 | state-machine, event-routing | M |
| abilities/host/Containment.qml | 46 | ordering | S |
| abilities/items/BasicItem.qml | 444 | ordering, state-machine, geometry-math, event-routing | L |
| abilities/items/basicitem/ParabolicItem.qml | 285 | geometry-math, state-machine, event-routing | L |
| abilities/items/basicitem/ParabolicEventsArea.qml | 225 | event-routing, geometry-math, ordering | L |
| abilities/items/IndicatorObject.qml | 119 | state-machine | S |
| abilities/items/basicitem/HiddenSpacer.qml | 102 | geometry-math, state-machine | S |
| abilities/items/basicitem/ShortcutBadge.qml | 88 | ordering | S |
| abilities/items/basicitem/IndicatorLevel.qml | 54 | event-routing, geometry-math | S |

Presentational (leave in QML), 74 files: components/HeaderSwitch.qml,
ItemDelegate.qml, ExternalShadow.qml, AddItem.qml, ScrollArea.qml,
SpinBox.qml, Header.qml, SubHeader.qml, Label.qml, CheckBox.qml,
CheckBoxesColumn.qml, Switch.qml, ToolTip.qml, AddingArea.qml, all six
components/private/ files, abilities/bridge/BridgeItem.qml, Indexer.qml,
Animations.qml, MyView.qml, ThinTooltip.qml, abilities/client/
Animations.qml, MyView.qml, Metrics.qml, Indicators.qml, ThinTooltip.qml,
Containment.qml, Debug.qml, Environment.qml, all three
appletabilities/Container*Bindings.qml, indicators/LatteConfiguration.qml,
all 27 abilities/definition/ interface files except the two behavioral
ones above, all abilities/host/ publicApi surfaces except ThinTooltip
and Containment, abilities/items/basicitem/SeparatorItem.qml,
TitleTooltipParent.qml, RestoreAnimation.qml,
abilities/items/IndicatorLevel.qml, indicators/LevelOptions.qml.

### JS logic files addendum (11 files, 1206 lines)

The .qml sweeps exclude imported .js libraries; they are part of the
same extraction surface:

- containment/package/contents/code/autosize.js (58) - shrinkStep/
  growStep math for EX-04.
- plasmoid/package/contents/code/layout.js (193) - plasmoid layout
  helpers.
- plasmoid/package/contents/code/tools.js (121) - task helper
  predicates.
- plasmoid/package/contents/code/TaskActions.js (56) - task action
  token dispatch tables.
- plasmoid/package/contents/code/activitiesTools.js (357) - launcher
  activity migration helpers.
- containment/package/contents/code/AppletIdentifier.js (304) -
  applet-specific icon discovery heuristics.
- three copies of ColorizerTools.js (34+34+28: declarativeimports/
  components/code/, containment/package/contents/code/,
  plasmoid/package/contents/code/) - the luminance math EX-19
  deduplicates.
- declarativeimports/components/code/EffectMath.js (10) - shadow blur
  curve.
- containment/package/contents/code/MathTools.js (11).

## B. Hot-spot ranking

Three axes. Bug-density counts verified fix commits whose diffs touch
the unit's logic (hashes cited; counted over f0ad7b23..HEAD with
`git log --grep='^fix' --name-only`). Testability-gain estimates how
much currently-unpinnable behavior becomes table-testable.
Feel-risk orders live-verification weight; high feel-risk sequences
later within its wave and gets a mandatory live recipe.

1. Preview adoption/anchoring pipeline (plasmoid main.qml previews
   block, TaskItem preview functions). Bug density 15+, the densest in
   the tree, all 2026-07-13..15: c6eeeb20, 4f96acb8, 4b533b8d,
   54ed1974, 0913bbee, 235753b8, d56a26aa, f1edd103, d619ae08,
   15558f40, e6c5ae76, c622da1b, d98bff98, 77aac4b4, df747ebf. Ten
   line-level invariants currently pinned only by grep
   (scripts/preview-contract-rules.sh, b4f5621c). Testability-gain:
   highest. Feel-risk: highest (hover feel, measured in ms).
   -> EX-01, strong-model-only.
2. Parabolic zoom engine (definition/ParabolicEffect.qml math;
   propagation chains in ParabolicEventsArea/ParabolicArea/
   ParabolicEdgeSpacer/ParabolicEffectPrivate). Direct fix density low
   in this port (the zoom math itself is untouched since fork), but
   the propagation index arithmetic is the exact class the
   loops/degenerate-values sweep hunted, the glide-vs-jump
   verification hazard cost hours of phantom flakiness (2026-07-15,
   recorded in latte-live-verification), and every hover bug transits
   this code. Feel-risk: maximum of the whole plan.
   -> EX-02 (router, strong-model-only) + EX-03 (math, delegate-safe).
3. AutoSize feedback loop (abilities/AutoSize.qml + code/autosize.js).
   Density: ad9b823f (infinite loop, 100% CPU hang, inherited
   upstream defect from 747d4870-era code). Already partially pinned
   by tests/qml/tst_autosize.qml. Pure math + bounded history: very
   high testability. -> EX-04, delegate-safe.
4. Fill/Justify length distribution (LayouterPrivate.qml). Zero direct
   fixes in our tree, but latte-dock-ng fixed a dock collapse in the
   same inherited algorithm (ng 30637c1cd) and the two-pass
   distribution is exactly the shape unit tests eat. -> EX-05,
   delegate-safe.
5. Visible-index and separator-neighbor ordering (IndexerPrivate.qml,
   client Indexer.qml twin, AppletItem/BasicItem neighbor walks).
   The 2026-07-15 loops sweep verified all these while-loops terminate
   (clean negatives, recorded in session-handoff) but nothing pins the
   RESULTS; the twins have already drifted apart structurally.
   -> EX-06, delegate-safe.
6. Storage id remapping (app/layouts/storage.cpp, C++). Density:
   fa02b887 (containments destroyed during template import; its
   liveness filter is a self-admitted band-aid with the deleter still
   unidentified), plus the whole duplicate-flow saga rode this path
   (e412889d investigation). capt extracted this exact unit
   (73f64383). -> EX-07, delegate-safe.
7. Available screen geometry (app/lattecorona.cpp, C++). Density:
   1b932ed9 (settings window overflow; our fix deliberately DIVERGES
   from upstream d30143f7 by accepting self-origin updates - the
   extraction must preserve that deviation). capt blueprint:
   screengeometrycalculator (with tests). -> EX-08, delegate-safe.
8. Positioner geometry (app/view/positioner.cpp, C++). Density: 3
   fixes (793faad2 moveToScreen remap, c5bdc239 late screen id,
   1607d022 family). capt blueprint: 4a829185. Our architecture
   note: on Wayland much placement authority moved to layer-shell
   anchors (app/wm/waylandlayershell.cpp), so the pure math matters
   mainly for X11, masks, and the canvas/edit chrome rects.
   -> EX-09, delegate-safe.
9. Visibility mask + input geometry (containment
   VisibilityManager.qml). Related family: the canvas input-region
   work (3d714d63, dbe5a03b) lives in C++ already; the QML half
   computes the dock's own mask/input rects. Errors here are
   invisible-dock / dead-input bugs. -> EX-10, delegate-safe with a
   heavy live recipe.
10. Launcher ordering complex (abilities/Launchers.qml,
    launchers/Validator.qml, TasksExtendedManager.qml). Density:
    d6d57e61 (stale synced-launcher clients crash); the Validator's
    upwardIsBetter -1-splice heuristic is in the assessed-guards
    inventory. Pure list algebra throughout. -> EX-11, delegate-safe.
11. Colorizer decision tree (colorizer/Manager.qml). The color complex
    (1f835402, 5c06b497, 79ca3360) was effects- and measurement-side,
    but every one of those investigations had to re-derive this
    QML decision tree to reason about expected behavior. -> EX-12,
    delegate-safe.
12. viewType/background predicate chain (containment main.qml,
    MultiLayered.qml). Density: 38e60eb9, f5a5f44c, d72ee0cd (the
    edit-mode background family) plus the recurring throwaway-layout
    confusion (viewType=1 rendering full-width background, mistaken
    for a regression twice in session-handoff). -> EX-13,
    delegate-safe.
13. Drop classification and insert index (DragDropArea.qml,
    MouseHandler.qml). Density: b474adad (the DropArea dead-handler
    trap was found here). -> EX-14, delegate-safe.
14. Wheel semantics (AudioStream.qml, TaskMouseArea.qml,
    EnvironmentActions.qml, RulerMouseArea.qml). Density: 299a241b
    (audio wheel matched to plasma-pa exactly, hand-verified).
    -> EX-15, delegate-safe.
15. Remaining pure-transform tail, all delegate-safe: EX-16 group
    cycling (SubWindows.qml + loaders/Tasks.qml), EX-17 preview
    title/subtext transforms (ToolTipInstance.qml), EX-18
    maxLength/offset clamp dedup (RulerMouseArea.qml vs
    AppearanceConfig.qml, two live copies of the same math), EX-19
    luminance dedup (five copies counted), EX-20 badge math, EX-21
    scroll overflow math, and the four remaining capt C++ blueprints
    EX-22..EX-25.

De-prioritized (high visibility, low extraction value): purely-drawing
QML (the states blocks, gradients, shadows), the ability
bridge/host/definition relay layers (property plumbing, no logic), the
settings pages' control-to-config plumbing (single-loader doctrine
already pinned by 32df5b47/c3d15966 fixes and their tests).

Pin-in-place verdicts (behavioral in the inventory, but extraction is
the wrong tool; each becomes a test-only task, not a backlog unit):

- shell CompactApplet.qml popup sizing/representation chain. Fix
  density is real (437d9a0c, 1aa5238c, 9ea29eaa, 5f8c10be, d12baff2)
  but every fix was about matching libplasma's live binding/parenting
  contracts, which is inherently scenegraph-coupled; the chain is
  already pinned by tests/qml/tst_compactapplet.qml (3b37750b) driving
  the real shipped file. Extracting the arithmetic would leave the
  risk (the wiring) in QML and fight the existing pin. Task: extend
  tst_compactapplet when the chain changes; nothing to extract.
- plasmoid ContextMenu.qml (909 lines). Menu assembly against live
  PlasmaExtras.Menu/TasksModel APIs; the one algorithmic piece (the
  eliding while-loop) was verified terminating in the loops sweep. The
  practical hazards here have been API-contract ones (52c2987b menu
  teardown, d67e635a/56549d73 alternatives chain), each now fixed at
  its origin. Task: qmltest contract for loadDynamicLaunchActions
  section assembly if churn resumes.
- editmode/ConfigOverlay.qml drag/reorder. Binding-entangled
  (hoveredItem hit-testing, live reparenting, input-mask re-carve
  8be2b388); recent fixes (36160c46, 8f821310) are stable and
  live-verified. Extraction would need a designed seam for the
  drag session state; flagged design-first in section F, not forced.
- TaskItem slotPublishGeometries. Geometry clamp math feeding
  libtaskmanager; depends on live item mapping (mapToGlobal) per
  frame. Task: add invariant assertions to a qmltest against the real
  TaskItem (bounds containment, hidden-view collapse).
- components/ComboBox.qml role resolution. Already fixed and
  regression-tested (a302d742 covers the three model kinds).
- LatteDockConfiguration.qml window size negotiation. Chrome-only,
  stable since 1b932ed9 fixed the C++ availability side.
- class-A stranded-binding reasserts (e412889d, eca51ae0 and the
  eca51ae0-family reassert functions in plasmoid main.qml). These are
  QML binding lifecycle countermeasures, not extractable logic; the
  open question (what destroys the bindings) is a filed watch item in
  the plan. Extraction note: EX-units that absorb the values these
  bindings feed (EX-10 mask geometry) reduce the surface, which is the
  real fix direction.

## C. Per-unit extraction specs

Written in rank order. Every line range below was verified by grep at
HEAD 5e1c2b12 (bab18b2c for the plan itself); Qt5 anchors were verified
against `git show f0ad7b23:<path>`. Where a Qt5 anchor is given as a
function name without lines, the executor reads that function at
execution time; a spec never carries an unverified line number.

Conventions used by all specs:
- "Core" means a pure C++ class/namespace: value structs in, values
  out, no QObject, no QQuickItem, no KConfig, no timers, no clock
  reads (time arrives as a parameter). Cores live next to their
  consumers: `containment/plugin/units/`, `plasmoid/plugin/units/`
  (new subdirectories, Latte-authored provenance headers so the
  plasma-desktop vendor-sync diff stays clean), `declarativeimports/
  core/units/`, or the app subtree for C++-to-C++ extractions
  (capt's placement, e.g. `app/layouts/`).
- "Shell" means the thin QML-facing wrapper (registered QObject or
  attached call sites) that feeds the core and applies its outputs.
  QML keeps ownership of timers, bindings, and scenegraph items; the
  core owns decisions and math.
- Tests go in `tests/units/<unit>test.cpp` (new ctest entries wired
  like the existing flat tests in tests/CMakeLists.txt), plus qmltest
  contract coverage where the QML shell wiring itself is the risk.
- Every unit's cutover commit deletes the QML logic body it replaces
  in the same commit (single-copy rule); rollback is one revert.

### EX-01 PreviewSwitchEngine [strong-model-only]

- Header: `plasmoid/plugin/units/previewswitchengine.h`
- Responsibility: the window-previews dialog's decision core - switch
  vs defer vs settle-adopt, hide-countdown arming/cancelling, and the
  parked-delegate LRU accounting - as one testable state machine.
- Source (verified): plasmoid/package/contents/ui/main.qml:440-476
  `materializeDelegateFor` (LRU: revive parked, build on miss, park +
  evict-oldest), 482-490 `dropCachedDelegateFor`, 518-544
  `shouldDeferSwitch` (burst debounce keyed on request cadence),
  566-631 `windowsPreviewDlg.hide`/`show`, 640-653
  `previewSwitchSettleTimer`, 656-673 `hidePreviewWinTimer`
  (composite contains-mouse test); plasmoid/package/contents/ui/task/
  TaskItem.qml:472-491 `showPreviewWindow`, 499-576
  `preparePreviewWindow` (cache-hit rootIndex refresh vs
  fresh-instance binding cluster).
- Extract-vs-pin: this is the one unit where the pin already exists
  and is still not enough. scripts/preview-contract-rules.sh
  (b4f5621c) pins ten line-level invariants by grep - it defends line
  ORDER, not semantics, and it broke new ground precisely because the
  logic is unhosted by any test harness. Fifteen fix commits in three
  days (section B, rank 1) is the strongest extraction signal in the
  tree. GATE INTERACTION, stated per the planning contract: the
  extraction deliberately migrates gate rules 1-4 (defer-before-
  prepare ordering, defer-cancels-hide, settle-adopts-directly,
  settle-interval-below-threshold) into behavioral C++ tests with an
  injected clock, and REWRITES those gate rules to instead assert the
  QML shell delegates to the engine (grep for the engine call). Rules
  that pin QML-side mechanics that stay in QML (rootIndex assigned
  after isGroup with the refresh-token bump, thumbnail icon fallback
  strictness, imperative size enforcement) remain in the gate
  untouched. No commit may leave an invariant unpinned in both
  places; each migration commit moves rule and test together.
- Interface / DI seam:
  - `struct SwitchRequest { int taskId; qint64 nowMs; bool dialogVisible; }`
  - `enum class SwitchDecision { ShowNow, Defer }` from
    `decide(SwitchRequest)`; deferred state carries the pending task;
    `settle(qint64 nowMs)` returns the task to adopt (the settle
    timer stays a QML Timer armed with the engine's interval).
  - `enum class HideDecision { StartCountdown, CancelCountdown, HideNow }`
    from `hoverChanged(HoverSnapshot)` where HoverSnapshot is the
    composite the hide timer currently reads (dialog contains-mouse,
    task contains-mouse, drag active).
  - LRU: `enum class Materialize { Revive, Build, BuildEvicting }`,
    `materialize(taskId) -> { Materialize kind; int evictTaskId; }`,
    `drop(taskId)`, capacity injected (4, from f1edd103).
  - The engine never touches the DelegateModel or the dialog; QML
    applies decisions. Timer intervals are engine constants exposed
    read-only so QML cannot drift from the tested thresholds
    (today: settle interval vs switchBurstThreshold, gate rule 4).
- capt cross-reference: none - capt has no preview-pipeline
  extraction; its tasks work is backend-side. This unit is ours alone.
- Test plan (tests/units/previewswitchenginetest.cpp): burst-boundary
  table (crossing cadence just under/over the threshold; the 120ms vs
  140ms degeneration bug from 4b533b8d's first cut as a named case);
  defer path cancels an armed hide countdown (54ed1974's regression);
  settle adopts the LAST hovered task and never re-enters the burst
  check (4b533b8d); hide-countdown composite (any contains-mouse
  cancels; drag blocks hide); LRU: revive keeps warm entries, build
  at capacity evicts the oldest, drop removes without eviction
  side-effects (f1edd103's Component.onDestruction contract);
  interleavings: defer -> exit-all -> settle must not show over
  nothing (d56a26aa family). Tests precede the extraction seam:
  written against the spec tables first, the engine implemented to
  green them.
- Qt5-fidelity: Qt5 main.qml:366-421 (hide at :366, show at :384,
  verified) is the ancestor - immediate show/hide with previewsDelay
  hover gating and no debounce/LRU (those are port-era responses to
  measured Qt6/Wayland rebuild costs: 100-400ms per adoption,
  4b533b8d body). Fidelity target is Qt5's OBSERVABLE behavior -
  previews follow the hovered task, previewsDelay honored, hide on
  exit - with the port's performance machinery preserved exactly as
  the fix commits measured it. Deviations are already documented in
  c6eeeb20..f1edd103 bodies; the spec adds none.
- Live verification (mandatory, feel-bearing): fakepointer GLIDE
  sweeps (~8px steps) across 4+ tasks at slow/medium/fast rates -
  never coordinate jumps (parabolic shift makes jumps miss enters;
  lesson recorded in latte-live-verification). Verify: single
  adoption at rest per sweep (event-loop lag probe or log counter),
  dialog never hides mid-scrub under the pointer, konsole 4-window
  group -> firefox -> konsole revives all four thumbnails
  (0913bbee's recipe), preview centered on the hovered icon
  (screenshot), zero attach(nil) on the wire across a stepped sweep
  (c6eeeb20's check, WAYLAND_DEBUG counter).
- Delegation tag: strong-model-only. The decisions being extracted
  are exactly the ones that took seven measured excavation layers to
  get right; encoding them wrong but plausibly is cheap for a weaker
  model and expensive to detect.
- Risk + rollback: highest-feel unit in the plan. Land as (1) core +
  tests, (2) QML cutover + gate migration - two commits, each
  revertable alone; the gate keeps the old rules until (2) so there
  is no unpinned window.

### EX-02 ParabolicRouter [strong-model-only]

- Header: `declarativeimports/core/units/parabolicrouter.h` (serves
  both containment and plasmoid sides through org.kde.latte.core).
- Responsibility: replace the recursive neighbor-to-neighbor
  scale-propagation signal chains with one computed per-item scale
  assignment for the whole row.
- Source (verified): containment/package/contents/ui/applet/
  ParabolicArea.qml:167-222 `sltUpdateItemScale` (slice/splice of the
  neighbour scales array, clear-request propagation up/down, bridge
  forwarding), 139-155 `calculateParabolicScales`;
  declarativeimports/abilities/items/basicitem/
  ParabolicEventsArea.qml:126-152 `calculateParabolicScales`, 154-204
  `updateScale`/propagation twins; containment/package/contents/ui/
  layouts/ParabolicEdgeSpacer.qml (55-110 per the inventory,
  accept/clear by tail/head + alignment); containment/package/
  contents/ui/abilities/privates/ParabolicEffectPrivate.qml (restore
  zoom state machine).
- Extract-vs-pin: the propagation is duplicated in two structurally
  drifted twins (containment applets vs plasmoid tasks), rides on
  index arithmetic of exactly the class the loops/degenerate-values
  sweep hunted, and crosses the applet bridge (clientsBridges) where
  sub-index handoff is hand-maintained. Pinning in place would need
  the full matrix of (edge spacers x separators x hidden x bridges)
  driven through real QML - strictly harder than testing a pure
  assignment function. Extraction also removes the per-hop signal
  latency structure, but BEHAVIOR must stay identical (see fidelity).
- Interface / DI seam: `struct RowItem { int index; bool isSeparator;
  bool isHidden; int subItemCount; }` (sub-items model bridge
  clients); `assignScales(rowItems, currentIndex, currentItemScales)
  -> QVector<float>` full-row assignment including edge-spacer
  absorption; a `clearAssignment(rowItems)` counterpart. QML shells
  (both twins) feed their row snapshot and apply the returned vector
  to zoomScale properties; the existing signals survive only as the
  application mechanism inside each shell, not as inter-item routing.
  DESIGN-FIRST note: how the bridge clients receive their slice
  (today: forwarded signals into the applet's internal indexer) needs
  a designed handoff before coding; this is half the reason for the
  strong-model tag.
- capt cross-reference: none. capt did not touch parabolic
  propagation.
- Test plan: equality harness first - an offscreen qmltest drives the
  EXISTING chain over a synthetic row (no live dock; the recipe for
  constructing ability object graphs offscreen is in
  tests/contracts, per session-handoff's tooling note) and records
  the resulting scale vectors for a case table (pointer at first/mid/
  last item, separators adjacent, hidden runs, spacer edges, bridge
  client mid-row); the C++ core must reproduce those vectors exactly.
  Then unit tests own the table and the qmltest shrinks to shell
  wiring.
- Qt5-fidelity: the chain shape is Qt5-inherited; fidelity = equal
  scale assignments for equal inputs, which the equality harness
  makes mechanical. Read f0ad7b23's ParabolicArea equivalents when
  building the case table so Qt5-era cases (no margins-area
  separators, e.g.) are represented.
- Live verification (mandatory): glide sweeps along a mixed row
  (tasks + separator + applets + edge), verifying visually smooth
  zoom with no popping neighbors; before/after screenshots at
  identical pointer positions must match within antialiasing noise;
  a fast zigzag glide (36160c46's drag-drift recipe adapted) checks
  direct-rendering engagement still triggers on rapid index jumps.
- Delegation tag: strong-model-only (maximum feel risk + the bridge
  handoff design). If the strong-model window closes before this
  lands: DEFER with this marker intact; do not delegate.
- Risk + rollback: feel regression of the signature interaction of
  the whole dock. Land core+harness first (no behavior change), then
  one cutover commit per twin (containment, plasmoid), each
  independently revertable.

### EX-03 ParabolicMathCore [delegate-safe]

- Header: `declarativeimports/core/units/parabolicmath.h`
- Responsibility: the zoom curve itself - per-item scale from pointer
  position - as pure functions.
- Source (verified): declarativeimports/abilities/definition/
  ParabolicEffect.qml:37-66 `applyParabolicEffect` (left/right scale
  stacks from mouse percentage, RTL swap), 68-76 `scaleForItem`,
  78-81 `scaleLinear`.
- Extract-vs-pin: S-size pure math, but it feeds EX-02 and is
  duplicated nowhere; extracting it first gives EX-02 a tested
  foundation and the equality harness a reference. Cheap, safe,
  foundational.
- Interface: `computeScales(double mousePosPercent, int itemIndex,
  int itemsCount, double zoomFactor) -> { QVector<double> left,
  right; }` plus `scaleForItem`/`scaleLinear` equivalents. The QML
  definition file keeps its signal emissions and calls the core for
  numbers only.
- capt cross-reference: none.
- Test plan (tests/units/parabolicmathtest.cpp): equivalence table
  against the current QML implementation (drive the QML function
  offscreen via qmltestrunner once, bake the numbers into the table
  with a comment naming the generation method); boundary cases:
  zoom=1 (all ones), single item, pointer at 0/50/100 percent, RTL
  swap symmetry (left(p) == mirror(right(1-p))).
- Qt5-fidelity: f0ad7b23 declarativeimports/abilities/definition/
  ParabolicEffect.qml:36 `applyParabolicEffect` and :67
  `linearEffect` (verified; the port renamed linearEffect to
  scaleForItem/scaleLinear - the executor confirms numeric
  equivalence of the renamed pair against the Qt5 body before
  writing the table).
- Live verification: one glide pass on the real dock after cutover;
  zoomed-row screenshot at a fixed pointer position matches
  pre-cutover capture.
- Delegation tag: delegate-safe. Pure math, equivalence-tested, no
  design decisions.
- Risk + rollback: minimal; one commit, one revert.

### EX-04 AutoSizeEngine [delegate-safe]

- Header: `containment/plugin/units/autosizeengine.h`
- Responsibility: the automatic icon-size search - shrink/grow step
  selection, oscillation detection, prediction history - as a pure
  step function.
- Source (verified): containment/package/contents/ui/abilities/
  AutoSize.qml:134-151 `producesEndlessLoop`, 154-242
  `updateIconSize`, 117-132 history ring (`clearHistory`/
  `addPrediction`), plus containment/package/contents/code/
  autosize.js (58 lines, `shrinkStep`/`growStep`; port-era file,
  absent at f0ad7b23 - verified).
- Extract-vs-pin: tests/qml/tst_autosize.qml already pins pieces
  against the shipped QML, and the maxLength<=0 early-return carries
  its deliberate-contract comment (ad9b823f). But the core is a
  feedback loop whose failure mode is a 100% CPU hang (the ad9b823f
  incident: latent upstream loop-termination defect armed by
  Wayland's late geometry), and loops that can hang the GUI thread
  belong in C++ where the full input space is table-testable.
  tst_autosize.qml remains as the shell pin.
- Interface: `struct AutoSizeInput { int layoutLength; int maxLength;
  int currentIconSize; int maxIconSize; double zoomFactor; bool
  normalState; }` -> `struct AutoSizeStep { int nextIconSize; bool
  found; }` via `step(input, History &history)`; History is a value
  type with the bounded ring (historyMinSize/historyMaxSize
  semantics preserved).
- capt cross-reference: none (capt's appletzoomsizetest is adjacent
  but backend-side).
- Test plan: the ad9b823f regression as a named case (iconSize=78
  layout from its commit body: termination for ANY icon size -
  property-style loop over sizes 16..256 asserting the search
  terminates and result is within bounds); oscillation detector
  truth table (history[1] vs prediction per producesEndlessLoop);
  asymmetric shrink/grow limits (the comments in AutoSize.qml warn
  about the oscillation the asymmetry prevents - encode as a test
  that a grow immediately after a shrink at the boundary is
  rejected); maxLength<=0 remains OUTSIDE the core (the QML contract
  comment and its onMaxLengthChanged re-run stay; core asserts
  maxLength>0 via Q_ASSERT so misuse is loud).
- Qt5-fidelity: f0ad7b23 containment/package/contents/ui/abilities/
  AutoSize.qml:151 `updateIconSize`, :131 `producesEndlessLoop`
  (verified). DEVIATION, named: ad9b823f's termination guarantee wins
  over the Qt5 body (Qt5 carries the latent infinite loop); each
  divergent branch cites ad9b823f in a comment.
- Live verification: restart the throwaway 3-dock layout with
  iconSize=78 (the ad9b823f recipe): all docks map, process idles;
  resize maxLength live via the ruler and watch the size settle
  without oscillation.
- Delegation tag: delegate-safe. The spec's tables are exhaustive
  and the hang failure mode is caught by the termination test.
- Risk + rollback: low; one core commit + one cutover commit.

### EX-05 FillLengthDistributor [delegate-safe]

- Header: `containment/plugin/units/filldistributor.h`
- Responsibility: the two-pass distribution of free length among
  fill applets (Justify two-sided and one-pool variants).
- Source (verified): containment/package/contents/ui/abilities/
  privates/LayouterPrivate.qml:74-146 `computeStep1ForLayout`,
  151-239 `computeStep2ForLayout`, 273-368
  `updateFillAppletsWithTwoSteps`, 370-415
  `updateFillAppletsWithOneStep`, 418-439 dispatcher.
- Extract-vs-pin: pure arithmetic over per-applet constraint records,
  currently executed by mutating live QML items mid-pass (the
  functions read item.metrics and assign item lengths as they go).
  Unit-testing in place requires a full ability graph; extracted, it
  is a table function. latte-dock-ng fixed a dock collapse in this
  inherited algorithm (ng 30637c1cd) - we carry the same shape.
- Interface: `struct FillItem { int minLength; int prefLength; int
  maxLength; bool isFill; }` (record -1 sentinels as std::optional
  at the boundary); `distribute(QVector<FillItem>, int
  availableSpace, Alignment alignment, SplitterInfo splitters) ->
  QVector<int>` assigned lengths. The QML layouter keeps gathering
  inputs from live items and applying results; the passes move.
- capt cross-reference: none directly; capt's
  editmodehandleactiontest and viewmodels work are elsewhere.
- Test plan: sum(assigned) <= availableSpace always; no assignment
  below min or above max; the ng 30637c1cd collapse scenario
  (fillWidth applet added to a constrained row - read ng's commit
  body for the exact trigger and encode it); all-fill,
  no-fill, single-item, zero-space, Justify halving with odd totals
  (rounding), splitter max-size participation
  (maxJustifySplitterSize semantics from the sibling
  AppletsContainer counters).
- Qt5-fidelity: f0ad7b23 containment/package/contents/ui/abilities/
  privates/LayouterPrivate.qml:74 and :151 (verified, same function
  names) - the executor diffs port-vs-Qt5 bodies first; any port-era
  drift found is a finding to record, not silently normalize.
- Live verification: Justify layout with two fill applets +
  fixed-size neighbors; add/remove an applet in edit mode and
  screenshot-verify no collapse and no overflow (the ng bug's
  symptom).
- Delegation tag: delegate-safe.
- Risk + rollback: low-medium (edit-mode add/remove paths exercise
  it); single cutover commit.

### EX-06 VisibleIndexEngine [delegate-safe]

- Header: `declarativeimports/core/units/visibleindex.h` (one core
  for both the containment and applet-side twins).
- Responsibility: visible-index mapping and separator/hidden
  neighbor predicates over an item row, including multi-item applet
  expansion.
- Source (verified): containment/package/contents/ui/abilities/
  privates/IndexerPrivate.qml:255-277 `visibleItemsBeforeCount`,
  279-289 `visibleIndex`, 291-311 `visibleIndexBelongsAtApplet`
  (plus the collector Bindings 23-253 per the inventory);
  declarativeimports/abilities/client/Indexer.qml (the drifted twin:
  firstTailItemIsSeparator/lastHeadItemIsSeparator/
  firstVisibleItemIndex/lastVisibleItemIndex/visibleIndex);
  containment/package/contents/ui/applet/AppletItem.qml:221-245
  `tailAppletIsSeparator`, 247-271 `headAppletIsSeparator`
  (while-loop neighbor walks), 486-532 `checkIndex`;
  declarativeimports/abilities/items/BasicItem.qml neighbor-walk
  twins (178-260 per the inventory).
- Extract-vs-pin: four sites hand-maintain the same ordering
  semantics; the 2026-07-15 loops sweep proved the walks terminate
  but nothing pins their RESULTS, and the twins have drifted
  structurally already. One tested core ends the drift class.
- Interface: `struct RowEntry { int index; bool isSeparator; bool
  isHidden; bool isMarginsSeparator; int subItemCount; }`;
  functions: `visibleIndexOf(entries, actualIndex)`,
  `entriesBefore(entries, actualIndex)`, `neighborIsSeparator(
  entries, actualIndex, Direction)`, `firstVisible/lastVisible`,
  `belongsAtEntry(entries, entryIndex, int visibleIndex)`. The QML
  collector Bindings keep gathering RowEntry lists (they read live
  children); all arithmetic moves.
- capt cross-reference: none for this unit.
- Test plan: separator runs at head/tail/middle; hidden interleaved
  with separators; multi-item applets (subItemCount 0/1/n) spanning
  the queried index; empty row; single separator-only row; the
  margins-area parity predicate (AppletItem `inMarginsArea`,
  inventory: parity count of preceding margin separators) as its own
  case family; cross-check containment and client twins give
  identical answers for identical inputs (the drift regression
  test).
- Qt5-fidelity: f0ad7b23 IndexerPrivate.qml:249/:273/:285 (verified
  same functions). Diff port-vs-Qt5 first, record drift findings.
- Live verification: shortcut badges (Meta+number ordering) on a
  row containing separators + a systray; badge numbers must match
  visible positions (PositionShortcuts consumes visibleIndex).
- Delegation tag: delegate-safe.
- Risk + rollback: low; per-twin cutover commits (containment,
  client) so a regression bisects to one side.

