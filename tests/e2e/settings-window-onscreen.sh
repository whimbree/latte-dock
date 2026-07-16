#!/usr/bin/env bash
# E2E: the view settings window maps FULLY on-screen on a cold session
# (the 1b932ed9 regression: upstream's self-origin exclusion made the
# chrome map 99px above the screen top on cold starts). Consumes the
# EX-08 ScreenGeometryCalculator path end to end.
set -uo pipefail
repo="${E2E_REPO:?run through scripts/run-e2e.sh}"

busctl --user call org.kde.kglobalaccel /component/lattedock \
    org.kde.kglobalaccel.Component invokeShortcut s "show view settings" >/dev/null
sleep 3
#! first invoke can race kglobalaccel registration
if ! "$repo/scripts/tools/dumpwins.sh" 2>/dev/null | grep -qE "latte-dock\|\|[0-9.,]+ [0-9]+x1[0-9]{3}\|"; then
    busctl --user call org.kde.kglobalaccel /component/lattedock \
        org.kde.kglobalaccel.Component invokeShortcut s "show view settings" >/dev/null
    sleep 2.5
fi

wins="$("$repo/scripts/tools/dumpwins.sh" 2>/dev/null)"

#! the primary config window is the tall latte window (height > 400,
#! not a dock strip): parse "x,y WxH" and the screen bounds, assert
#! the window rect sits inside its screen rect
result="$(echo "$wins" | awk -F'|' '
    $2 ~ /latte-dock/ {
        split($4, g, " "); split(g[1], pos, ","); split(g[2], size, "x");
        x = pos[1]; y = pos[2]; w = size[1]; h = size[2];
        if (h > 400 && w > 300 && w < 2000) { cx=x; cy=y; cw=w; ch=h; found=1 }
    }
    $2 ~ /plasmashell/ && $6 == "layer=0" {
        split($4, g, " "); split(g[1], pos, ","); split(g[2], size, "x");
        sx = pos[1]; sy = pos[2]; sw = size[1]; sh = size[2];
    }
    END {
        if (!found) { print "NOCONFIG"; exit }
        if (sw == 0) { print "NOSCREEN"; exit }
        if (cx >= sx && cy >= sy && cx+cw <= sx+sw && cy+ch <= sy+sh) print "ONSCREEN";
        else printf "OFFSCREEN config=%s,%s %sx%s screen=%s,%s %sx%s\n", cx, cy, cw, ch, sx, sy, sw, sh;
    }')"

#! close the settings again (focus loss)
"$E2E_FAKEPOINTER" click 2200 800 >/dev/null 2>&1 || true
sleep 1

case "$result" in
    ONSCREEN) echo "settings window fully on-screen"; exit 0;;
    NOCONFIG) echo "FAIL: no settings window mapped after two invokes"; exit 1;;
    NOSCREEN) echo "FAIL: could not determine screen geometry from dumpwins"; exit 1;;
    *)        echo "FAIL: $result"; exit 1;;
esac
