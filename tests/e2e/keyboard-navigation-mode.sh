#!/usr/bin/env bash
# E2E: keyboard focus mode lifecycle over D-Bus in the nested vehicle -
# enter/exit readback (viewsData keyboardNavigation), unknown-id refusal,
# and the bulletproof focus-loss exit: with the mode on, a client window
# mapping in the compositor takes the keyboard focus and the mode must
# fall back to off ON ITS OWN (a dock stuck focusable breaks every
# fullscreen application - that is the defect class this pins).
#
# Uses the driver's managed vehicle dock like every sibling recipe. The
# first version launched its OWN dock instead (written before the driver
# existed); under the driver's shared bus that second launch died on the
# KDBusService unique name AND its forwarded activation popped the
# Settings window on the driver dock - which then held keyboard focus,
# so enterKeyboardNavigation's requestActivate never landed and the
# focus-loss leg failed while ALSO poisoning whichever recipe ran next
# (caught 2026-07-17 promoting the suite; the window dump in the ledger
# is the evidence). Recipes must never launch a second dock.
#
# The focus-taker is a minimal QML window, not konsole: konsole's cold
# start inside the nested session exceeded every reasonable wait and the
# focus-loss leg timed out on it (caught 2026-07-17 while landing the
# mode; the qml window maps in about a second).
set -u

repo="${E2E_REPO:?run through scripts/run-e2e.sh}"
source "$repo/tests/e2e/lib.sh"
scratch="$(mktemp -d /tmp/kbnav-e2e.XXXXXX)"

cat > "$scratch/focus-taker.qml" <<'EOF'
import QtQuick
Window { visible: true; width: 300; height: 200; title: "kbnav-focus-taker" }
EOF

call() { busctl --user call org.kde.lattedock /Latte org.kde.LatteDock "$@"; }
viewsjson() { call viewsData | sed 's/^s "//; s/"$//; s/\\"/"/g'; }

TAKER=""
cleanup() {
    [ -n "$TAKER" ] && kill "$TAKER" 2>/dev/null
    rm -rf "$scratch" 2>/dev/null
}
trap cleanup EXIT INT TERM

fail() { echo "FAIL: $1"; exit 1; }
pass() { echo "ok: $1"; }

e2e_wait_settled 45 || fail "vehicle dock never settled"
pass "driver dock running and settled"

cid=$(viewsjson | jq -r '.[0].containmentId // empty' 2>/dev/null)
{ [ -n "$cid" ] && [ "$cid" != "null" ]; } || fail "no containment id in viewsData"
pass "containment id $cid"

kbnav() { viewsjson | jq -r ".[] | select(.containmentId == $cid) | .keyboardNavigation"; }

[ "$(kbnav)" = "false" ] || fail "baseline keyboardNavigation is not false"
pass "baseline false (mode off is the default)"

#! unknown id is refused loudly, dock stays alive, real view untouched
call setViewKeyboardNavigation ub 999999 true >/dev/null 2>&1
sleep 1
[ "$(call lifecycleState | awk '{print $2}')" = '"running"' ] || fail "dock died on unknown-id refusal"
[ "$(kbnav)" = "false" ] || fail "unknown-id call changed the real view's state"
pass "unknown containment id refused, dock alive"

call setViewKeyboardNavigation ub "$cid" true >/dev/null
got=""; for _ in $(seq 1 10); do got=$(kbnav); [ "$got" = "true" ] && break; sleep 0.5; done
[ "$got" = "true" ] || fail "enter did not read back keyboardNavigation true"
pass "enter over D-Bus: keyboardNavigation true"

call setViewKeyboardNavigation ub "$cid" false >/dev/null
got=""; for _ in $(seq 1 10); do got=$(kbnav); [ "$got" = "false" ] && break; sleep 0.5; done
[ "$got" = "false" ] || fail "exit did not read back keyboardNavigation false"
pass "exit over D-Bus: keyboardNavigation false"

#! the focus-loss exit
call setViewKeyboardNavigation ub "$cid" true >/dev/null
for _ in $(seq 1 10); do [ "$(kbnav)" = "true" ] && break; sleep 0.5; done
[ "$(kbnav)" = "true" ] || fail "re-enter before the focus-loss leg failed"

#! Wait for the compositor to actually grant the layer-shell dock its
#! OnDemand keyboard focus before mapping the taker. This state is
#! Qt-level (QWindow::active on the layer surface) and NOT observable
#! over D-Bus or KWin scripting - KWin's workspace.activeWindow never
#! reports layer surfaces. If the taker maps before the grant lands the
#! dock was never active, so there is no active->inactive transition for
#! the exit watcher to catch and the leg races false-negative (proven
#! 2026-07-17: the leg passed deterministically once ~1.5s of probe
#! overhead sat here, failed without it). The settle is for an
#! inherently-unobservable compositor grant, not a value clamp; the
#! denial half of this (grant refused, not merely slow) is the filed
#! keyboard-item follow-up.
sleep 3

qml "$scratch/focus-taker.qml" > /dev/null 2>&1 &
TAKER=$!
got=""
for _ in $(seq 1 60); do
    got=$(kbnav); [ "$got" = "false" ] && break; sleep 0.5
done
kill "$TAKER" 2>/dev/null; TAKER=""
[ "$got" = "false" ] || fail "focus loss did not exit keyboard navigation (dock stuck focusable)"
pass "focus loss exits the mode on its own"

#! exit is idempotent
call setViewKeyboardNavigation ub "$cid" false >/dev/null
sleep 1
[ "$(call lifecycleState | awk '{print $2}')" = '"running"' ] || fail "dock died on idempotent exit"
pass "idempotent exit, dock alive"

echo "PASS: keyboard-navigation-mode"
exit 0
