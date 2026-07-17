#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Latte Dock contributors
# SPDX-FileCopyrightText: 2026 Bree Spektor
# SPDX-License-Identifier: GPL-2.0-or-later
# e2e-mode: nested-only
#
# Smoke: the vehicle dock reaches lifecycleState running, its views settle
# with sane geometry on the vehicle output, a SIGTERM exits cleanly, and a
# relaunch comes back. This is the by-hand verification of the nested
# vehicle's first proof (session-handoff 2026-07-16), made re-runnable.
set -uo pipefail
source "${E2E_REPO:?run through scripts/run-e2e.sh}/tests/e2e/lib.sh"

e2e_wait_running 30 || e2e_fail "dock not running"
e2e_wait_settled 30 || e2e_fail "views did not settle"

#! every settled view must carry a real geometry; a 0x0 view is the
#! stranded-startup signature viewsData exists to expose
e2e_json viewsData | python3 -c '
import json, sys
views = json.load(sys.stdin)
if not views:
    sys.exit("no views loaded")
for v in views:
    x, y, w, h = v["absoluteGeometry"]
    if w <= 0 or h <= 0:
        sys.exit("view %s has degenerate geometry %s" % (v["containmentId"], v["absoluteGeometry"]))
print("%d views settled" % len(views))
'

e2e_dock_stop || e2e_fail "no clean SIGTERM exit"
echo "clean SIGTERM exit"

e2e_dock_start || e2e_fail "dock did not come back after restart"
echo "dock relaunched and settled"
