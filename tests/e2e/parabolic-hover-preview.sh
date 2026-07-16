#!/usr/bin/env bash
# E2E: gliding the pointer along the dock engages the parabolic pipeline
# and hovering a task maps a preview dialog (the EX-01/02/03 paths end to
# end). Screen-agnostic: derives the widest bottom dock from dumpwins and
# glides along it with small steps (jump-clicks land beside
# parabolic-shifted icons; glides are the only honest pointer input).
set -uo pipefail
repo="${E2E_REPO:?run through scripts/run-e2e.sh}"

dock="$("$repo/scripts/tools/dumpwins.sh" 2>/dev/null | awk -F'|' '
    $2 ~ /latte-dock/ && $6 == "layer=3" {
        split($4, g, " "); split(g[1], pos, ","); split(g[2], size, "x");
        if (size[1] > 1000 && size[2] < 500 && size[1] > best) { best = size[1];
            printf ""; x=pos[1]; y=pos[2]; w=size[1]; h=size[2]; }
    }
    END { if (best) printf "%d %d %d %d\n", x, y, w, h }')"

[[ -n "$dock" ]] || { echo "FAIL: no horizontal dock found in dumpwins"; exit 1; }
read -r dx dy dw dh <<< "$dock"

#! hover line: just above the dock window's bottom edge
hovery=$(( dy + dh - 8 ))
startx=$(( dx + dw / 3 ))
endx=$(( dx + dw * 2 / 3 ))

"$E2E_FAKEPOINTER" move "$startx" $(( hovery - 160 )); sleep 0.3
"$E2E_FAKEPOINTER" move "$startx" "$hovery"; sleep 0.4
x=$startx
while (( x < endx )); do "$E2E_FAKEPOINTER" move "$x" "$hovery"; x=$(( x + 16 )); done
sleep 1.6   #! previewsDelay (throwaway default 650ms) + build time

previews="$("$repo/scripts/tools/dumpwins.sh" 2>/dev/null | grep -cE 'latte-dock\|\|[0-9.,]+ [0-9]+x[0-9]+\|[^|]+\|layer=6' || true)"

#! leave the dock so zoom restores and the preview hides
"$E2E_FAKEPOINTER" move "$startx" $(( hovery - 400 )); sleep 1.2

if (( previews > 0 )); then
    echo "parabolic glide engaged; preview dialog mapped (layer=6)"
    exit 0
fi
echo "FAIL: no preview dialog mapped after gliding the tasks region"
exit 1
