#!/usr/bin/env bash
# E2E: dbus duplicateView produces a collision-free containment whose
# appletOrder references exactly its own new applet ids (the EX-07
# StorageIdRemapper path end to end), then removes the duplicate and
# waits out the libplasma undo window before finishing.
set -uo pipefail
repo="${E2E_REPO:?run through scripts/run-e2e.sh}"
layout="$repo/build/_runconfig/latte/My Layout.layout.latte"

[[ -f "$layout" ]] || { echo "FAIL: throwaway layout not found"; exit 1; }

before_ids="$(grep -E '^\[Containments\]\[[0-9]+\]$' "$layout" | grep -oE '[0-9]+' | sort -n | uniq)"
src_id="$(echo "$before_ids" | head -1)"

busctl --user call org.kde.lattedock /Latte org.kde.LatteDock duplicateView u "$src_id" >/dev/null
sleep 6

after_ids="$(grep -E '^\[Containments\]\[[0-9]+\]$' "$layout" | grep -oE '[0-9]+' | sort -n | uniq)"
new_id="$(comm -13 <(echo "$before_ids") <(echo "$after_ids") | head -1)"

[[ -n "$new_id" ]] || { echo "FAIL: no new containment appeared after duplicateView"; exit 1; }

#! collision-free by construction of comm; check the applet references
order="$(awk -v id="$new_id" '$0=="[Containments]["id"][General]"{f=1;next} /^\[/{f=0} f&&/^appletOrder=/{sub(/^appletOrder=/,""); print}' "$layout")"
applets="$(grep -oE "^\[Containments\]\[$new_id\]\[Applets\]\[[0-9]+\]$" "$layout" | grep -oE '[0-9]+\]$' | tr -d ']' | sort -n)"

ok=1
if [[ -n "$order" ]]; then
    for token in ${order//;/ }; do
        echo "$applets" | grep -qx "$token" || { echo "FAIL: appletOrder token $token has no applet group"; ok=0; }
    done
fi

#! cleanup: remove the duplicate and wait out the undo window
busctl --user call org.kde.lattedock /Latte org.kde.LatteDock removeView u "$new_id" >/dev/null
for i in $(seq 1 24); do
    grep -q "^\[Containments\]\[$new_id\]$" "$layout" || break
    sleep 5
done
grep -q "^\[Containments\]\[$new_id\]$" "$layout" && { echo "FAIL: duplicate $new_id still in layout after undo window"; exit 1; }

[[ "$ok" == 1 ]] && echo "duplicate $src_id -> $new_id: ids collision-free, appletOrder consistent, cleaned up"
[[ "$ok" == 1 ]]
