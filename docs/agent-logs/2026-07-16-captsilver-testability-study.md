# CaptSilver testability study - session log (2026-07-16)

Read-only research pass over ~/Projects/latte-dock-qt6 (CaptSilver fork),
comparing its testing infrastructure against this tree, with the sceneprobe
visual-regression harness as the deep-dive target. Full analysis returned to
the coordinator; this is the method log.

## Order of work

1. Listed both trees' tests/ directories. Fork working tree: ~48 C++ test
   files flat under tests/, plus tests/qml (17 leaf qmltest files + 66
   coverage-driver tests under tests/qml/pkg), tests/sceneprobe,
   tests/coverage (C++/QML coverage harness with its own pytest suite),
   tests/e2e (nested-kwin live harness), tests/manual (script gates).
   Ours: tests/ (7 behavioral + 6 script gates), tests/units (27 sanitized
   pure-core tests), tests/contracts (7 C++ + 4 qmltest contract pins),
   tests/qml (19 qmltest files). Confirmed our ctest count: 6 add_test +
   14 ecm_add_test + 27 latte_add_unit_test = 47.

2. Sceneprobe, file by file: main.cpp (QQuickRenderControl render-to-texture
   on the Vulkan RHI, QRhi readback, StepAnimationDriver fixed clock,
   probeExpect/probeTolerance scene properties, bless mode, artifact
   writing), imagecompare.{h,cpp} (comparator: per-channel max delta +
   exceed-budget fraction, invariants floor, region/pixel expectations,
   amplified diff), run.sh (gate orchestration, self-test-first, QML
   staging via DESTDIR install), run_in_kwin.sh (nested kwin_wayland
   --virtual session wrapper, ICD selection), imgdiff_main.cpp (CLI reuse
   of the comparator, also used by their e2e), suppressions files, all 19
   scene QML files, both selftest scenes, the comparator's own unit test.

3. Read the sceneprobe commit trail (git log --format with bodies) for
   rationale: the per-scene tolerance commit (2b105b43) documents the
   lavapipe MultiEffect run-to-run variance that forced tolerance
   overrides for mask/blur/passthrough; a49a50e0/76f39d2d document the
   deterministic-clock re-bless.

4. Read their e2e harness (run.sh + shot.py: real dock in nested kwin,
   D-Bus add/remove drive, KWin ScreenShot2 capture, latte-imgdiff pixel
   assertion), their coverage harness README + run.sh + qml_coverage.sh
   (Cov-tick QML instrumentation, honest-coverage rules in
   tests/qml/pkg/README.md), and the manual/ script gates.

5. Enumerated every C++ test's header comment for the inventory
   classification, plus the fork's tests/CMakeLists.txt in full - notable
   mechanism there: the "prebuilt objects glob-link" trick (link the app's
   own .cpp.o files minus main.cpp) to test Corona-coupled classes without
   recompiling or restructuring.

## Dead ends and surprises

- Grepping the fork's working tree for ICoronaHost/IViewFactory/IScreenInfo
  found nothing. Cause: the local checkout sits 54 commits behind
  origin/main (working tree at 9003f33a; origin/main at 81384003, which
  matches the fork-sync hash in CLAUDE.md). The DI seams (app/iscreeninfo.h,
  app/layout/iviewfactory.h, CoronaEngine, tests/fakescreeninfo.h) and ~18
  additional tests exist only in the fetched origin/main objects, read via
  git show. ICoronaHost does not exist anywhere under that name; the
  Corona-kernel seam is called CoronaEngine.
- Sceneprobe itself is identical between the working tree and origin/main;
  all sceneprobe detail came from the working tree as instructed.
- No CI config exists in the fork at all (no .github/workflows, no
  .gitlab-ci.yml). Their "CI story" is local shell gates plus a sample
  pre-push hook (tests/coverage/pre-push.sample) that runs the coverage
  ratchet inside a Fedora distrobox.
- The fork's qmltest files never call grabImage - consistent with the
  offscreen-platform blank-grab problem this tree hit; their pixel
  assertions all go through sceneprobe (render control readback) or the
  e2e ScreenShot2 path instead.
- Mid-task constraint from the repo owner: adoption target must run under
  a plain VM (KVM, no GPU passthrough). Verified: sceneprobe's default
  device is lavapipe (software Vulkan, LP_NUM_THREADS=0, ICD pinned via
  VK_ICD_FILENAMES); the dgpu mode is an optional second golden set behind
  an explicit --device dgpu flag with its own hardware-specific
  MESA_VK_DEVICE_SELECT pin; nothing in the default gate touches real
  hardware. Cross-machine golden determinism is the open risk (Mesa
  version, fontconfig); noted in the analysis with mitigations.

No builds, tests, or GPU work were run; everything above is from reading
code, configs and git history.
