#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Latte Dock contributors
# SPDX-FileCopyrightText: 2026 Bree Spektor
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Run a command under a throwaway nested kwin_wayland session so it gets a
# Vulkan-capable wayland QPA (QVulkanInstance needs platform glue the
# offscreen QPA lacks; the probe's window still never maps).
#
# The upstream harness used kwin's --exit-with-session to launch the command;
# on the pinned kwin 6.6.5 that never starts the session app (verified with a
# trivial script: kwin comes up, the wayland socket appears, the app never
# runs). So this wrapper backgrounds kwin on a private socket in a private
# XDG_RUNTIME_DIR, waits for the socket, runs the command directly (it
# inherits the caller's env, no generated session script needed), then tears
# kwin down. Exit code is the command's own.
#
# The kwin bring-up/teardown mechanics live in scripts/lib-nested-kwin.sh,
# shared with the e2e recipe driver; this file keeps only what is
# sceneprobe-specific (the Vulkan device dispatch and the probe's env).
#
# Device dispatch, SCENEPROBE_DEVICE (this is the ONE dispatch point;
# golden filenames carry the device name so the sets stay independent):
#   lavapipe (default) - Mesa software Vulkan from the flake pin,
#       LP_NUM_THREADS=0 for bit-reproducibility. The only tier CI gates
#       on: pure CPU, VM-safe, and the only device the gate script uses.
#   dgpu - the host's real Vulkan driver, opt-in. A DOCUMENTED optional
#       extra as of 2026-07-16 (direction change recorded in
#       docs/agent-logs/2026-07-16-sceneprobe-followup-scenes.md; the
#       adoption plan's "undocumented local extra" line is superseded on
#       this point): the harness must also WORK with a GPU but never
#       REQUIRE one - CI/microvm stay lavapipe-only. VK_ICD_FILENAMES is
#       left unset so the loader enumerates the host's ICDs
#       (/run/opengl-driver on NixOS); on multi-GPU boxes export
#       MESA_VK_DEVICE_SELECT yourself (the upstream harness hardcoded
#       its own card's 1002:7550 here - a hardware pin that does not
#       belong in the repo). dgpu goldens are blessed separately if ever;
#       a missing dgpu golden set is reported loudly by the probe while
#       the shader/validation/blank gates still verdict.
#
# The validation layer comes from the flake pin in BOTH modes (devShell
# exports LATTE_VK_LAYER_PATH); the lavapipe ICD from the pin too, never
# the host's /run/opengl-driver - lavapipe goldens must be blessed against
# the exact Mesa CI runs.
set -u

DEV="${SCENEPROBE_DEVICE:-lavapipe}"
case "$DEV" in
  lavapipe)
    ICD="${LATTE_VULKAN_LAVAPIPE_ICD:-}"
    [ -n "$ICD" ] && [ -f "$ICD" ] || { echo "lavapipe ICD not found (LATTE_VULKAN_LAVAPIPE_ICD unset or missing; run inside the flake devShell)" >&2; exit 2; }
    ;;
  dgpu)
    # no ICD forced: the loader enumerates the host's drivers; refuse
    # loudly if that enumeration could only find lavapipe-from-the-pin
    # semantics (nothing to check here portably - the probe prints the
    # selected device name, read it)
    ICD=""
    ;;
  *)
    echo "unsupported SCENEPROBE_DEVICE '$DEV': lavapipe (default, CI tier) or dgpu (opt-in host GPU)" >&2
    exit 2
    ;;
esac
LAYERS="${LATTE_VK_LAYER_PATH:-}"
[ -n "$LAYERS" ] && [ -d "$LAYERS" ] || { echo "validation layer manifests not found (LATTE_VK_LAYER_PATH unset or missing; run inside the flake devShell)" >&2; exit 2; }

source "$(cd "$(dirname "$0")/../../scripts" && pwd)/lib-nested-kwin.sh"

nested_kwin_prepare
trap nested_kwin_cleanup EXIT INT TERM
nested_kwin_start 256 256 sceneprobe-wl || exit 2

# LP_NUM_THREADS=0 disables llvmpipe's threaded rasterizer, which is what
# makes lavapipe output bit-reproducible (the {0,0} golden tier depends on
# it). Both it and the forced ICD are lavapipe-only: the dgpu mode leaves
# the loader's enumeration alone.
DEV_ENV=()
[ "$DEV" = "lavapipe" ] && DEV_ENV=(LP_NUM_THREADS=0 VK_ICD_FILENAMES="$ICD")
env -u DISPLAY -u XAUTHORITY \
    QT_QPA_PLATFORM=wayland WAYLAND_DISPLAY="$NESTED_SOCK" XDG_RUNTIME_DIR="$NESTED_RT" \
    QSG_RHI_BACKEND=vulkan "${DEV_ENV[@]}" VK_LAYER_PATH="$LAYERS" \
    timeout 90 "$@"
ec=$?

exit "$ec"
