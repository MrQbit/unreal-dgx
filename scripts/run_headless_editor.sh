#!/bin/bash
# Launch the native aarch64 Unreal Editor headless (offscreen) with the Remote Control
# HTTP API server, so an MCP client can drive it. Run this in a normal terminal / tmux /
# systemd unit on the DGX (NOT piped through another tool that reaps child processes).
#
#   ./run_headless_editor.sh [/path/to/Project.uproject]
#
# The Remote Control HTTP API comes up on http://127.0.0.1:30010 (WebSocket on 30020).
# First launch compiles many shaders and can take a while before the port opens — watch the log.
set -u

UE_ROOT="${UE_ROOT:-$HOME/UnrealEngine}"
PROJECT="${1:-$HOME/DGXGame/DGXGame.uproject}"
EDITOR="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor"
LOG="${UE_RC_LOG:-/tmp/ue_headless.log}"

export DOTNET_ROOT="${DOTNET_ROOT:-$HOME/.dotnet}"
export DISPLAY="${DISPLAY:-:0}"

[ -x "$EDITOR" ] || { echo "editor not found at $EDITOR (set UE_ROOT)"; exit 1; }
[ -f "$PROJECT" ] || { echo "project not found at $PROJECT"; exit 1; }

echo "Launching headless editor:"
echo "  editor : $EDITOR"
echo "  project: $PROJECT"
echo "  log    : $LOG"
echo "  RC API : http://127.0.0.1:30010  (WebSocket 30020)"
echo

# -RenderOffScreen: full Vulkan RHI but no window (required; -NullRHI destabilises Slate).
# -RCWebControlEnable: force-start the Remote Control server outside the normal editor UI.
# stdin is left attached to the terminal so the editor does not exit on EOF.
exec "$EDITOR" "$PROJECT" \
  -RenderOffScreen -unattended -nosplash -nopause -stdout -RCWebControlEnable \
  -abslog="$LOG"
