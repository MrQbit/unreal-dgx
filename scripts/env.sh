#!/bin/bash
# Source this before building/running the aarch64 UnrealEditor.
export DOTNET_ROOT="$HOME/.dotnet"
export PATH="/usr/local/bin:$HOME/.dotnet:/usr/bin:/bin:/usr/sbin:/sbin"
export UE_ARM64_MINIMAL_EDITOR=1
# Build:  ./Engine/Build/BatchFiles/Linux/Build.sh UnrealEditor Linux Development -ForceUseSystemCompiler
# Run:    ./Engine/Binaries/Linux/UnrealEditor-Cmd -run=CompileAllBlueprints -unattended -nopause -nosplash
