#!/bin/bash
# Start the GameForge editor SERVICE: one long-lived headless UnrealEditor with the Remote Control
# HTTP API on :30010, so Claude can drive the whole build pipeline over RC *from chat* — no per-spec
# scripts. Start this ONCE on the DGX (systemd unit below, or tmux), leave it running, and then the
# workflow is just: refine a spec with Claude -> "build it" -> Claude generates assets + assembles +
# packages against this live service.
#
#   ./start_forge_service.sh [Project.uproject]      # default: ~/DGXGame/DGXGame.uproject
#
# Health check:  curl -s http://127.0.0.1:30010/remote/info   (200 once ready; first boot compiles
#                shaders and can take a few minutes before the port answers).
set -u
UE_ROOT="${UE_ROOT:-$HOME/UnrealEngine}"
PROJECT="${1:-$HOME/DGXGame/DGXGame.uproject}"
EDITOR="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor"
export DOTNET_ROOT="${DOTNET_ROOT:-$HOME/.dotnet}"

[ -x "$EDITOR" ]  || { echo "editor not found: $EDITOR (set UE_ROOT)"; exit 1; }
[ -f "$PROJECT" ] || { echo "project not found: $PROJECT"; exit 1; }

echo "GameForge editor service"
echo "  project : $PROJECT"
echo "  RC API  : http://127.0.0.1:30010   (WebSocket 30020)"
echo "  runs in the foreground (managed by systemd/tmux); Ctrl-C or 'systemctl stop' to end."
echo

# Runs in the FOREGROUND so systemd/tmux owns the process. 'tail -f /dev/null' keeps stdin open so the
# editor does not exit on EOF. -RenderOffScreen = full Vulkan RHI, no window (stable; -NullRHI is not).
# -RCWebControlEnable force-starts the Remote Control HTTP server without the editor UI.
exec bash -c "tail -f /dev/null | '$EDITOR' '$PROJECT' \
  -RenderOffScreen -unattended -nosplash -nopause -stdout -RCWebControlEnable -RCWebInterfaceEnable"
