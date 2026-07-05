# Unreal Engine 5.4 — Native aarch64 (ARM64) Port for NVIDIA DGX Spark (GB10)

**Status: 🚧 IN PROGRESS**

A native **aarch64 Linux** build of the Unreal Engine 5.4 **Editor**, running on the
**NVIDIA DGX Spark (GB10 Grace-Blackwell)** — no emulation. Epic ships the Linux editor for
x86_64 only; this repository documents and captures the engine patches + third-party library
work required to compile, link, and boot the full editor natively on ARM64.

## Target hardware

| | |
|---|---|
| Machine | NVIDIA DGX Spark ("spark") |
| SoC | NVIDIA GB10 (Grace CPU: Cortex‑X925 + Cortex‑A725, 20 cores; Blackwell GPU) |
| Arch | `aarch64` |
| Memory | 121 GB unified |
| OS | Ubuntu 24.04.4 LTS, kernel 6.17.0-nvidia |
| GPU driver | NVIDIA 580.159.03 (open kernel module, aarch64), CUDA 13 |
| Vulkan | 1.4 (GB10 exposes 16 graphics queues) |

## What works today

- ✅ **UnrealBuildTool, GitDependencies, receipts** — all build & run natively on aarch64
- ✅ **The entire engine C++ compiles** natively with system `clang-18` + libc++ (0 compile errors)
- ✅ **DXC + ShaderConductor built for arm64** (the LLVM‑scale HLSL→SPIR‑V dependency)
- ✅ **~25 third‑party libraries ported** (rebuilt from source, arch‑fixed, or stubbed) — see
  [`docs/THIRD_PARTY_LIBS.md`](docs/THIRD_PARTY_LIBS.md)
- ✅ **The editor links**: `Engine/Binaries/Linux/UnrealEditor` — ELF aarch64, 415+ modules, ~7.6 GB
- ✅ **The editor boots natively**: loads the engine, mounts plugins, compiles Vulkan shaders,
  starts the Asset Registry (reports `platform="LinuxArm64", cpu="ARM|Cortex-X925"`)
- ✅ **Vulkan confirmed on the Blackwell GPU** — the GB10 enumerates as a Vulkan 1.4 device and
  UE detects the NVIDIA aarch64 driver + loads the Vulkan shader formats (the GPU is **not** a blocker)

## What's still in progress

- 🚧 **Full module completeness** — a "minimal" build dropped some essential engine modules
  (target‑platform modules) causing a `RunningPlatform` runtime assert. Being resolved by switching
  to a full‑module build with the un‑portable plugins (USD, OpenCV, …) disabled.
- ⬜ **Editor GUI + Vulkan rendering** verified end‑to‑end (raw Vulkan is confirmed; UE device
  creation happens after the current assert)
- ⬜ **Unreal MCP + Claude Code** integration (drive the editor to build a game)

See [`docs/STATUS.md`](docs/STATUS.md) for the live blocker list.

## Repository layout

| Path | Contents |
|---|---|
| `patches/unreal-engine-5.4-aarch64.patch` | All engine source patches (43 files) vs UE 5.4 |
| `patches/BASE_COMMIT.txt` | The exact UE 5.4 commit the patch applies to |
| `docs/PORTING.md` | The porting method + every category of fix, in order |
| `docs/THIRD_PARTY_LIBS.md` | Per‑library: what was wrong on arm64 and how it was resolved |
| `docs/BUILD.md` | How to reproduce the build from a fresh UE 5.4 checkout |
| `docs/STATUS.md` | Milestones done / current blocker / remaining work |

> **Note on the engine source:** Epic's UE source is licensed and cannot be redistributed, so this
> repo contains **patches**, not the engine tree. Apply them to your own UE 5.4 checkout
> (see `docs/BUILD.md`). You need a GitHub account linked to Epic Games to access UE source.

## Quick start

```bash
# 1. Get UE 5.4 source (requires EpicGames GitHub access) and check out the base commit
git clone -b 5.4 https://github.com/EpicGames/UnrealEngine.git
cd UnrealEngine && git checkout 847de5e25

# 2. Apply the aarch64 patches
git apply /path/to/unreal-dgx/patches/unreal-engine-5.4-aarch64.patch

# 3. Follow docs/BUILD.md (toolchain, dependencies, third-party lib builds, editor build)
```
