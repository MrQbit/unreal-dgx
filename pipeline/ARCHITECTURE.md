# GameForge — a self-contained "game spec → SteamOS game" pipeline on the DGX Spark

**Goal:** anyone with a DGX Spark (GB10) + Claude can hand in a game spec and get a complete, built,
SteamOS-runnable game — **all generation happens locally on the Spark** (no external art/audio/model
APIs). The GB10 is an AI supercomputer (Blackwell GPU, 121 GB unified memory), so it both *generates
the assets* and *runs the engine*.

```
 game spec (yaml)
        │
        ▼
 ┌─────────────────────────────────────────────────────────────────────────┐
 │ 1. SPEC EXPANSION      local LLM (ollama: nemotron-3-super:120b) + Claude  │
 │    spec → detailed design + asset manifest (entities, meshes, textures,   │
 │    sounds, levels, gameplay logic)                                        │
 ├─────────────────────────────────────────────────────────────────────────┤
 │ 2. ASSET GENERATION    (local, on the GB10)                               │
 │    • 3D meshes  → Blender (bpy, procedural) [+ optional AI image→3D] → glTF│
 │    • textures / sprites / UI → diffusers (SDXL/FLUX on Blackwell) → PNG    │
 │    • music / SFX → AudioCraft/MusicGen ; voice → Piper TTS → WAV           │
 │    • VFX / procedural audio → Niagara / MetaSound (in-engine)              │
 ├─────────────────────────────────────────────────────────────────────────┤
 │ 3. IMPORT              MCP asset_import → UE (glTF via Interchange, PNG,   │
 │    WAV) — the aarch64 editor + DGXMCPTools plugin                          │
 ├─────────────────────────────────────────────────────────────────────────┤
 │ 4. ASSEMBLY            MCP bp_* tools → Blueprints (entities, GameMode),   │
 │    wire logic graphs, build levels (spawn+configure actors, assign meshes,│
 │    lighting), materials                                                   │
 ├─────────────────────────────────────────────────────────────────────────┤
 │ 5. PACKAGE            x86_64 cross-build + cook → SteamOS / Steam Deck .pak│
 └─────────────────────────────────────────────────────────────────────────┘
```

## Workflow — how you actually use it

**The service model (primary).** Start one long-lived headless editor + Remote Control on the DGX and
leave it running; everything after is conversation.

1. On the DGX, once:  `scripts/start_forge_service.sh`  (or `systemctl enable --now forge-editor`).
   Editor comes up with the RC API on `:30010`. Check: `scripts/forge_status.sh`.
2. In chat with Claude: refine a spec together → say **"build it"**. Claude generates the assets
   locally (Blender/SDXL/MusicGen), imports + assembles them into the live editor over Remote Control,
   and packages for SteamOS. No per-spec script — the service is the only thing you start.

```
  you  ⇄  Claude (chat: shape spec, "build it")
                     │  generate assets locally + drive over Remote Control
                     ▼
        [ persistent editor service on the DGX  :30010 ]  ──►  SteamOS package
```

**Testing shortcut (not the workflow).** `scripts/forge_pong.sh <proj> <spec>` launches its own editor
and runs one spec end-to-end, then exits — a self-contained smoke test only.

The two DGX-console dependencies today: start the service once, and run the long SteamOS cook. Both
are one-time/occasional; the build-a-game loop itself is just chat.

## Components (what runs where)

| Stage | Tool | Status on the DGX |
|---|---|---|
| Orchestration | `gameforge.py` (this dir) drives all stages | scaffolded here |
| LLM (design/dialogue) | **ollama** `nemotron-3-super:120b` | ✅ installed (86 GB) |
| 3D meshes | **Blender 4.0.2** headless (`bpy`) → glTF | ✅ installed (arm64) + validated |
| Textures/sprites | **diffusers** SDXL-Turbo/FLUX on CUDA | ✅ **verified** — torch 2.11+cu128 sees GB10 (cap 12.1); SDXL-Turbo generated a 512² PNG |
| Music/SFX | **MusicGen** (via transformers) | ✅ **verified** — generated a 5.9 s 32 kHz WAV on the GB10 |
| Voice | **Piper TTS** (light, arm64) | ✅ **verified** — synthesized a 3.3 s speech WAV |
| Import → UE | **DGXMCPTools** `ImportAsset` (glTF/PNG/WAV via Interchange/AssetTools) | ✅ built + reflected |
| Assembly | **DGXMCPTools** `bp_*` (Blueprint graphs, levels, spawn/config) over Remote Control MCP | ✅ built |
| Package (SteamOS) | UBT x64 cross-build + cook (`-Architecture=x64`) | ✅ binary proven; cook = long |

## The critical enabler already solved

- **glTF import works on arm64** (Interchange `InterchangeGltfTranslator` — mesh + material + anim),
  so generated 3D assets enter UE even though FBX is stubbed. `ImportAsset` also covers textures/audio.
- **Everything is local** — the LLM, image, audio, 3D generation, engine, and packaging all run on the
  one Spark. No cloud art/audio APIs, matching the "just runs for anyone with a Spark" intent.

## Flow of one entity (e.g. "player" in a platformer)

1. LLM expands: player = 2D sprite sheet, run/jump anims, jump SFX, movement config.
2. Generate: `diffusers` → `player_sheet.png`; `audiocraft`/`piper` → `jump.wav`.
3. Import: `asset_import("player_sheet.png", "/Game/Sprites")`, `asset_import("jump.wav", "/Game/Audio")`.
4. Assemble: `bp_create` BP_Player (Paper2D or Pawn), `bp_add_component` sprite/flipbook,
   `bp_add_event_node`/`bp_add_call_node`/`bp_connect` for input+jump, `bp_compile`, `bp_save`.
5. Level: `bp_spawn` platforms + player start; `asset_set_actor_mesh` for 3D props.
6. Package: `scripts/package_game.sh <proj> x64`.

## Directory

```
pipeline/
  ARCHITECTURE.md        this file
  setup.sh               one-shot installer for the AI generators (run once on the DGX)
  gameforge.py           orchestrator: spec -> generate -> import -> assemble -> package
  gen/
    image_gen.py         diffusers (SDXL/FLUX) local text->image  (textures/sprites/UI)
    audio_gen.py         music/SFX (AudioCraft) + voice (Piper)   -> WAV
  blender/
    gen_prop.py          procedural 3D props -> glTF  (validated)
  specs/
    schema.md            spec format
    pong.yaml            worked example
```

## Reality / scope notes

- This is a **large, staged system**. The engine + import + assembly + x64 packaging are done and
  proven; the **local AI generators** (image/audio) are installed by `setup.sh` (big first-run model
  downloads) and are the main remaining setup on a fresh Spark.
- Generation quality scales with the models chosen; the pipeline is model-agnostic (swap SDXL↔FLUX,
  MusicGen↔StableAudio) — `setup.sh` picks sensible defaults that fit the GB10.
- Best-fit first games: **2D platformer** and **greybox 3D FPS** (mechanics fully code-driven; art
  from Blender-procedural + local image/audio gen). Art-heavy adventures need more generation passes.
