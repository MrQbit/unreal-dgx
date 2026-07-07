# Unreal on the DGX Spark + GameForge — chat-driven game building on ARM64

Two things live here:

1. **A native `aarch64` port of the Unreal Engine 5.4 editor** for the **NVIDIA DGX Spark (GB10
   Grace‑Blackwell)** — Epic ships the Linux editor for x86_64 only; this repo carries the engine
   patches + third‑party work to compile, link, boot, and *package games* natively on ARM64.
2. **GameForge** — a self‑contained pipeline that turns a **game spec into a built, SteamOS‑runnable
   game**, with all asset generation running **locally on the Blackwell GPU** (no external APIs). You
   start one editor service, then just chat with Claude: *refine a spec → "build it."*

> **Who this is for:** anyone with a DGX Spark who wants to build games — hobbyists included. Own the
> whole pipeline on your own hardware; no cloud bill, no per‑seat SaaS.

## What works today (verified on the DGX)

**Engine / build**
- ✅ Entire engine C++ compiles natively (system `clang-18` + libc++, 0 errors); editor links & boots
- ✅ **Vulkan on the Blackwell GPU** (GB10 = Vulkan 1.4 device); editor runs headless with Remote Control
- ✅ **Packages games for two targets from the one box**: native **arm64**, and **x86_64 / SteamOS**
  (cross‑compiled — a real `x86-64` game binary built on the ARM64 Spark; see `docs/STEAMOS_X86_64.md`)
- ✅ ~25 third‑party libs ported; glTF import works (Interchange) so generated 3D assets get in

**GameForge pipeline (all local on the GB10)**
- ✅ **Image gen** — SDXL‑Turbo on CUDA (torch 2.11+cu128 sees the GB10); produced real textures
- ✅ **Audio gen** — MusicGen (music) + Piper (voice); produced real WAVs
- ✅ **3D gen** — Blender headless → glTF (procedural props)
- ✅ **Local LLM** — ollama (spec expansion / dialogue)
- ✅ **Asset import + Blueprint authoring** — a UE editor plugin (`DGXMCPTools`) drives imports and
  builds/wires Blueprint gameplay graphs, over Remote Control (MCP)
- ✅ **Behavior recipes** — spec `behaviors: [...]` compile into real Blueprint graphs

**Console‑pending** (the editor can't be launched from inside a coding harness, only on the DGX console):
the first live end‑to‑end run + the long SteamOS cook. Every *stage* is individually proven.

## Quick start

### A. Build the engine (once)
```bash
# UE 5.4 source requires an EpicGames-linked GitHub account (Epic source can't be redistributed).
git clone -b 5.4 https://github.com/EpicGames/UnrealEngine.git
cd UnrealEngine && git checkout "$(cat /path/to/unreal-dgx/patches/BASE_COMMIT.txt)"
git apply /path/to/unreal-dgx/patches/unreal-engine-5.4-aarch64.patch
# then follow docs/BUILD.md (toolchain, third-party libs, editor build)
```

### B. Set up GameForge (once)
```bash
pipeline/setup.sh        # Blender + local AI generators (torch/diffusers/MusicGen/Piper) + models
```

### C. Build games — the service model
```bash
scripts/start_forge_service.sh          # start ONE headless editor + Remote Control (:30010)
scripts/forge_status.sh                 # is it up?  (first boot compiles shaders)
```
Then just talk to Claude: shape a spec together, say **"build it"** — Claude generates the assets
locally, imports + assembles them into the live editor over Remote Control, and packages for SteamOS.
No per‑spec scripts. (Testing shortcut: `scripts/forge_pong.sh <proj> <spec>` runs one spec end‑to‑end.)

See **[`pipeline/ARCHITECTURE.md`](pipeline/ARCHITECTURE.md)** for the full design and workflow.

## Repository layout

| Path | Contents |
|---|---|
| `patches/` | The engine source patch (52 files) vs UE 5.4 + the base commit |
| `plugin/DGXMCPTools/` | UE editor plugin: asset import + Blueprint‑graph authoring + the Forge commandlet |
| `pipeline/` | **GameForge** — `ARCHITECTURE.md`, `setup.sh`, `gameforge.py` (orchestrator), `gen/` (image/audio), `blender/`, `specs/`, `assets/` (the CC0 asset spine) |
| `mcp/` | Remote Control ↔ MCP server (drive the editor from an MCP client) |
| `scripts/` | `start_forge_service.sh` (+ systemd unit), `package_game.sh`, cross‑compile env, etc. |
| `docs/` | `PORTING.md`, `BUILD.md`, `STEAMOS_X86_64.md`, `PACKAGING.md`, `THIRD_PARTY_LIBS.md`, `STATUS.md` |
| `game/PongGame/` | A code‑driven sample game (builds for arm64 **and** x86_64/SteamOS) |

## Focus: the 3D FPS vertical

The strongest fit for this stack is a **3D FPS** (UE's home turf) built on a **CC0 asset spine** — see
[`pipeline/FPS.md`](pipeline/FPS.md). Generation is reserved for *variation* (textures, music, layout);
the curated CC0 library provides the foundation (characters, weapons, environments); and the agent
does the valuable part — wiring the gameplay.

## Licensing notes

- **Engine:** Epic's UE source is licensed — this repo ships **patches**, not the engine tree.
- **Generated assets:** produced locally, yours to use.
- **Library assets:** the FPS spine uses **CC0** sources (public‑domain‑equivalent) so personal *and*
  redistributable use is clean — see `pipeline/FPS.md` for the per‑source licenses.
