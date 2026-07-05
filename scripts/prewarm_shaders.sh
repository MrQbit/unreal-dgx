#!/bin/bash
# One-time shader/DDC pre-warm. Compiles every shader the project needs and caches the results
# in the Derived Data Cache, so subsequent editor launches start FAST (shaders load from cache
# instead of recompiling). Run this once on the DGX after a build or a shader-setting change;
# let it finish (it can take a while the first time — the heavy TSR/Lumen shaders dominate).
#
#   ./prewarm_shaders.sh [/path/to/Project.uproject]
set -u
UE_ROOT="${UE_ROOT:-$HOME/UnrealEngine}"
PROJECT="${1:-$HOME/DGXGame/DGXGame.uproject}"
export DOTNET_ROOT="${DOTNET_ROOT:-$HOME/.dotnet}"

echo "Pre-warming DDC/shaders for $PROJECT ..."
echo "(first run is slow; progress prints as shader batches finish)"
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PROJECT" \
  -run=DerivedDataCache -fill \
  -unattended -nopause -nosplash -NullRHI -stdout
echo "Done. Editor launches should now be fast (shaders served from DDC)."
