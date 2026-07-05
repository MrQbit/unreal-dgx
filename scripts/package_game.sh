#!/bin/bash
# Cook + stage + package a game for Linux (native aarch64) from the DGX editor.
# Produces a runnable standalone build under <Project>/Saved/StagedBuilds/Linux/.
#
#   ./package_game.sh [/path/to/Project.uproject] [platform]
# platform defaults to Linux (arm64). Also valid here: LinuxArm64. (Windows/Mac need those OSes.)
set -u
UE_ROOT="${UE_ROOT:-$HOME/UnrealEngine}"
PROJECT="${1:-$HOME/PongGame/PongGame.uproject}"
PLATFORM="${2:-Linux}"
export DOTNET_ROOT="${DOTNET_ROOT:-$HOME/.dotnet}"
export PATH="/usr/local/bin:$HOME/.dotnet:$PATH"
export UE_ARM64_MINIMAL_EDITOR=1

echo "Packaging $PROJECT for $PLATFORM ..."
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  -project="$PROJECT" \
  -platform="$PLATFORM" -clientconfig=Development \
  -build -cook -stage -pak -package \
  -unrealexe="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" \
  -nocompileeditor -utf8output
echo "Done. Look under $(dirname "$PROJECT")/Saved/StagedBuilds/$PLATFORM/"
