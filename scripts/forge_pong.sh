#!/bin/bash
# Turnkey: build a game from a spec end-to-end on the DGX. Launches the headless editor + Remote
# Control, runs GameForge (generate assets -> import -> assemble Blueprints+level), packages x64/SteamOS.
#   ./forge_pong.sh [Project.uproject] [spec.yaml]
set -u
PIPE=/home/mrqbit/Downloads/unreal-dgx/pipeline
PROJECT="${1:-$HOME/DGXGame/DGXGame.uproject}"
SPEC="${2:-$PIPE/specs/pong.yaml}"
UE_ROOT="${UE_ROOT:-$HOME/UnrealEngine}"
export DOTNET_ROOT="${DOTNET_ROOT:-$HOME/.dotnet}" FORGE_HOME="${FORGE_HOME:-$HOME/gameforge}"
export HF_HOME="$FORGE_HOME/models/hf" UE_RC_URL="http://127.0.0.1:30010"
LOG=/tmp/forge_editor.log

echo "== 1. launch headless editor + Remote Control =="
setsid bash -c "tail -f /dev/null | '$UE_ROOT/Engine/Binaries/Linux/UnrealEditor' '$PROJECT' \
  -RenderOffScreen -unattended -nosplash -nopause -stdout -RCWebControlEnable -abslog='$LOG'" \
  > /tmp/forge_editor.out 2>&1 &
echo "   editor launching (first run compiles shaders — minutes). log: $LOG"

echo "== 2. wait for Remote Control on :30010 =="
for i in $(seq 1 180); do
  curl -s -o /dev/null http://127.0.0.1:30010/remote/info && { echo "   RC up"; break; }
  sleep 10
done

echo "== 3. generate assets + import =="; python3 "$PIPE/gameforge.py" "$SPEC" --stage assets
echo "== 4. assemble Blueprints + level =="; python3 "$PIPE/gameforge.py" "$SPEC" --stage assemble
echo "== 5. package for SteamOS (x64) =="; python3 "$PIPE/gameforge.py" "$SPEC" --stage package --project "$PROJECT"
echo "Done. Staged build under $(dirname "$PROJECT")/Saved/StagedBuilds/Linux/"
