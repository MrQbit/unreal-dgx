#!/bin/bash
# Cook + stage + pak + package a game for Linux from the DGX editor. Produces a runnable standalone
# build under <Project>/Saved/StagedBuilds/Linux/.  Run this in a real terminal / tmux on the DGX —
# the first cook compiles all shaders and takes a while (this coding harness reaps long processes).
#
#   ./package_game.sh [/path/to/Project.uproject] [arch]
#     arch = arm64 (default, native)  |  x64 (SteamOS / Steam Deck, cross-compiled)
set -u
UE_ROOT="${UE_ROOT:-$HOME/UnrealEngine}"
PROJECT="${1:-$HOME/PongGame/PongGame.uproject}"
ARCH="${2:-arm64}"
export DOTNET_ROOT="${DOTNET_ROOT:-$HOME/.dotnet}"
export PATH="/usr/local/bin:$HOME/.dotnet:$PATH"
export UE_ARM64_MINIMAL_EDITOR=1

ARCH_ARGS=""
if [ "$ARCH" = "x64" ]; then
	# SteamOS / Steam Deck: cross-compile the x86_64 game binary (cook is shared Vulkan content).
	export UE_X64_SYSROOT="${UE_X64_SYSROOT:-$HOME/x86_64-sysroot}"
	ARCH_ARGS="-specifiedarchitecture=x64"
	echo "Packaging $PROJECT for Linux x86_64 (SteamOS) — UE_X64_SYSROOT=$UE_X64_SYSROOT"
else
	echo "Packaging $PROJECT for Linux arm64 (native)"
fi

"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
	-project="$PROJECT" \
	-platform=Linux -clientconfig=Development \
	$ARCH_ARGS \
	-build -cook -stage -pak -package \
	-unrealexe="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor-Cmd" \
	-nocompileeditor -utf8output
echo "Done. Staged build under $(dirname "$PROJECT")/Saved/StagedBuilds/Linux/"
