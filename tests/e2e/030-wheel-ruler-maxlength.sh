#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Latte Dock contributors
# SPDX-FileCopyrightText: 2026 Bree Spektor
# SPDX-License-Identifier: GPL-2.0-or-later
# e2e-mode: nested-only
#
# EX-15 live check 4 (docs/agent-logs/EX-15.md): in edit mode, one wheel
# detent over the max-length ruler moves maxLength by exactly 6 points per
# direction (the RulerMouseArea cutover through LatteCore.WheelStepper,
# VerticalOnly pick, threshold 96; the +-6 step feeds EX-18's shared clamp).
# Asserted on the layout file, which the containment flushes within ~1s of
# the write (measured in the vehicle), so every detent is verified
# individually and a double-landing cannot masquerade as a single step.
#
# The ruler lives in the CANVAS window (mapped only in edit mode, full
# screen width, thin), at its outermost ~13px rows; located from the window
# dump instead of hardcoding the overlay's font-dependent thickness. Each
# detent is a single-invocation fakepointer scroll (motion -> 100ms ->
# axis, the shape that wins the vehicle's enter race - measured 6/6
# deliveries in calibration) with a retry loop for the occasional loss.
set -uo pipefail
source "${E2E_REPO:?run through scripts/run-e2e.sh}/tests/e2e/lib.sh"

view="$(e2e_tasks_view)" || e2e_fail "no tasks view"

#! an ABSENT key IS a value here: writing the default (100) back makes
#! KConfig delete the entry, so the readback must normalize absent -> 100
#! or the up-detent's landing reads as "no change" (cost a full failing
#! afternoon arc before the write path was instrumented and found healthy)
cfg() {
    local v
    v="$(kreadconfig6 --file "$E2E_LAYOUT" --group Containments --group "$view" --group General --key maxLength)"
    v="${v%.*}"
    echo "${v:-100}"
}

orig_raw="$(kreadconfig6 --file "$E2E_LAYOUT" --group Containments --group "$view" --group General --key maxLength)"
orig="$orig_raw"
restore_config() {
    e2e_dock_stop >/dev/null 2>&1 || true
    if [[ -n "$orig" ]]; then
        kwriteconfig6 --file "$E2E_LAYOUT" --group Containments --group "$view" --group General --key maxLength "$orig"
    else
        kwriteconfig6 --file "$E2E_LAYOUT" --group Containments --group "$view" --group General --key maxLength --delete
    fi
}
trap restore_config EXIT

start="$(cfg)"
(( start - 6 >= 50 )) || e2e_fail "maxLength $start leaves no headroom for exact-step assertions"

strip="$(e2e_view_field "$view" '"%d %d %d %d" % tuple(v["absoluteGeometry"])')"
read -r sx sy sw sh <<< "$strip"
screen="$(e2e_view_field "$view" '"%d %d %d %d" % tuple(v["screenGeometry"])')"
read -r scx scy scw sch <<< "$screen"

windows_before="$(e2e_dumpwins | grep '|latte-dock|' | sort)"
#! park the pointer mid-screen before the canvas maps (part of the one
#! delivery rhythm that proved reliable - 6/6 alternating detents in
#! calibration; deviations from it lost up-detents 0/5)
"$E2E_FAKEPOINTER" move 800 400
sleep 0.5
e2e_call setViewEditMode ub "$view" true >/dev/null
sleep 3

#! the canvas: the latte window edit mode just mapped, screen-wide and thin
canvas="$(comm -13 <(echo "$windows_before") <(e2e_dumpwins | grep '|latte-dock|' | sort) | awk -F'|' -v scw="$scw" '
    { split($4, g, " "); split(g[1], pos, ","); split(g[2], size, "x");
      if (size[1] == scw && size[2] < 300) { printf "%d %d %d %d\n", pos[1], pos[2], size[1], size[2]; exit } }')"
[[ -n "$canvas" ]] || e2e_fail "no canvas window mapped for edit mode"
read -r cx cy cw ch <<< "$canvas"

rx=$(( sx + sw / 2 ))
#! bottom dock: the ruler occupies the canvas' outermost rows (its top)
ry=$(( cy + 7 ))

# wheel_step <detent> <expected-value>: deliver one detent on the ruler
# (single-invocation scroll, then off the ruler so no tooltip dwells) and
# wait for the layout file to land EXACTLY on the expected value.
wheel_step() {
    local detent="$1" expect="$2" attempt i v
    for attempt in 1 2 3 4 5; do
        "$E2E_FAKEPOINTER" scroll "$rx" "$ry" "$detent" 100
        "$E2E_FAKEPOINTER" move "$rx" 650
        sleep 1.2
        for i in $(seq 1 6); do
            v="$(cfg)"
            if [[ "$v" != "$last" ]]; then
                [[ "$v" == "$expect" ]] || e2e_fail "detent $detent moved maxLength $last -> $v (expected $expect: exactly 6 per detent)"
                last="$v"
                return 0
            fi
            sleep 1
        done
        echo "  (ruler detent $detent not delivered on attempt $attempt, retrying)"
    done
    e2e_fail "ruler detent $detent never delivered after 5 attempts"
}

last="$start"
wheel_step -1 $(( start - 6 ))
echo "down-detent: maxLength $start -> $last (exactly -6)"
wheel_step 1 "$start"
echo "up-detent: maxLength back to $last (exactly +6)"

e2e_call setViewEditMode ub "$view" false >/dev/null
sleep 1.5

#! the clean stop must persist the same value (no shutdown rewrite)
e2e_dock_stop || e2e_fail "no clean stop to confirm persistence"
final="$(cfg)"
[[ "$final" == "$start" ]] || e2e_fail "maxLength changed across shutdown: $last -> $final"

echo "ruler wheel steps maxLength by 6 per detent, both directions, persisted"
