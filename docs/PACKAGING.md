# Packaging / publishing from the aarch64 editor — what's possible

Short version: **yes, you can build shippable game packages — natively for Linux (the DGX's own
platform), and for a couple of other targets with extra SDKs. Windows and Apple can't be packaged
from this Linux machine alone** — that's a normal Unreal/toolchain limitation, not something specific
to the arm64 port. You package a platform *on* that platform (or with its cross-toolchain), and
Apple targets require macOS for the Metal shader compiler.

## What the editor can cook/build (verified on this build)

Two things gate a target: a **TargetPlatform module** (to cook assets) and a **shader-format
compiler** (to cook shaders). On this build:

- TargetPlatform modules built: **LinuxTargetPlatform, LinuxArm64TargetPlatform, AndroidTargetPlatform**
- Shader formats built: **VulkanShaderFormat, ShaderFormatOpenGL, ShaderFormatVectorVM**
- No **ShaderFormatD3D** (Windows) and no **MetalShaderFormat** (Apple) — those don't build on Linux.

## Matrix

| Target | Cook (assets+shaders) | Build game binary | Verdict |
|---|---|---|---|
| **Linux arm64** (aarch64) | ✅ Vulkan/GL, native | ✅ native (this machine) | **YES — fully works.** Runs on the DGX and other ARM64 Linux. |
| **Linux x86_64** | ✅ Vulkan/GL | ⚠️ needs the x86_64 Linux cross-toolchain (clang + sysroot) | **Yes, with toolchain setup.** |
| **Android** | ✅ Vulkan/GLES | ⚠️ needs the Android NDK (arm64 Linux host) | **Yes, with the NDK installed.** |
| **Windows** | ❌ no D3D shader format on Linux | ❌ needs MSVC/clang-cl + Windows SDK | **No** from here — package on Windows. |
| **macOS / iOS / tvOS** | ❌ Metal compiler is macOS-only | ❌ Mach-O + codesign need a Mac | **No** from here — package on a Mac. |

So the DGX is a self-contained dev+publish box **for Linux**, and can reach Android/Linux-x64 with the
usual extra SDKs. Cross-building Windows/Apple from Linux isn't supported by Unreal regardless of the
arm64 work; use those platforms (or a build farm) for their packages.

## How to package (Linux, native)

The reliable path — cook + stage + package the Game target for Linux arm64:

```bash
# 1. Build the Game runtime target (once)
export DOTNET_ROOT=$HOME/.dotnet PATH=/usr/local/bin:$HOME/.dotnet:$PATH UE_ARM64_MINIMAL_EDITOR=1
cd $HOME/UnrealEngine
./Engine/Build/BatchFiles/Linux/Build.sh PongGame Linux Development \
   -Project=$HOME/PongGame/PongGame.uproject -ForceUseSystemCompiler

# 2. Cook + stage + package (BuildCookRun)
./Engine/Build/BatchFiles/RunUAT.sh BuildCookRun \
   -project=$HOME/PongGame/PongGame.uproject \
   -platform=Linux -clientconfig=Development \
   -cook -stage -pak -package -build \
   -unrealexe=$HOME/UnrealEngine/Engine/Binaries/Linux/UnrealEditor-Cmd
# -> staged, runnable build under PongGame/Saved/StagedBuilds/Linux/
```

`scripts/package_game.sh` wraps this. First cook compiles all shaders (slow once; cached after).

> Note on the target platform name: Unreal's "Linux" platform on this build produces arm64 binaries
> (the host arch). For an explicit arm64 client, `-platform=LinuxArm64` is also available.
