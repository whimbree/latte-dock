#!/usr/bin/env bash
# Source-scan rule test for the Qt6 effect contracts this port earned the
# hard way (latte-plasma6-defect-families, family 7).
#
# Rule: no autoPaddingEnabled anywhere in shipped QML except the literal
# `autoPaddingEnabled: false`. autoPadding recomputes the effect's padding
# and re-dirties it continuously, so every window carrying such an effect
# re-rendered empty frames forever - measured 18.2% idle CPU and ~19,500
# failing statx/s from per-frame theme lookups before e3376405 made
# ShadowedItem's padding static. Effects must carry a STATIC per-side
# paddingRect instead (per-side semantics: 6c7001ce).
#
# This is a plain grep over the source tree, not a staged install: the rule
# must hold for every shipped QML file whether or not a build exists.
set -euo pipefail

repo="$(cd "$(dirname "$0")/.." && pwd)"

shipped=(
    "$repo/containment"
    "$repo/plasmoid"
    "$repo/shell"
    "$repo/declarativeimports"
    "$repo/indicators"
)

# Property assignments only (identifier followed by a colon); prose mentions
# of the property name in comments are fine.
violations="$(grep -rn --include='*.qml' -E 'autoPaddingEnabled[[:space:]]*:' "${shipped[@]}" \
    | grep -vE 'autoPaddingEnabled[[:space:]]*:[[:space:]]*false([[:space:];/]|$)' || true)"

if [[ -n "$violations" ]]; then
    echo "FAIL: autoPaddingEnabled must only ever be assigned the literal 'false' in shipped QML:" >&2
    echo "$violations" >&2
    exit 1
fi

echo "qml-effect-rules: OK (autoPaddingEnabled only ever disabled in shipped QML)"
