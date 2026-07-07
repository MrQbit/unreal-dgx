# The 3D FPS vertical

The strongest fit for this stack. Unreal is *the* FPS engine, the CC0 asset ecosystem covers FPS well,
and almost nobody is doing AI-assisted FPS generation. The model is **library + generate + wire**:

- **Library (CC0)** provides the foundation — characters, weapons, environment kits, materials, audio.
- **Generation** provides the *variation* — restyled textures, custom music/voice, procedural layout.
- **The agent** does the valuable part — composing the level and **wiring the gameplay**.

## The CC0 asset spine

Defined in [`assets/fps_spine.yaml`](assets/fps_spine.yaml); fetched by `fetch_spine.py` into
`~/gameforge/library/<role>/`. Everything is **CC0** (public-domain-equivalent) — clean for personal
*and* redistributable use, which also keeps a hosted service out of licensing trouble.

| Role | CC0 source | Fetch |
|---|---|---|
| Characters (animated) | Quaternius, Kenney | manual |
| Weapons | Quaternius, Kenney | manual |
| Environment kits | Kenney (prototype), Quaternius (modular) | manual |
| Materials (PBR) | **ambientCG** | ✅ auto |
| Sky / lighting (HDRI) | **Poly Haven** | ✅ auto |
| Props (models) | **Poly Haven** | ✅ auto |
| Audio (SFX) | Kenney | manual |

```bash
pipeline/fetch_spine.py           # auto-downloads ambientCG + Poly Haven; prints the manual CC0 grabs
```
The materials, HDRIs, and props auto-download today (verified). Kenney/Quaternius are CC0 but have no
stable script URL, so the fetcher prints their pages — download, unzip into `library/<role>/<name>/`.

**FBX note:** the arm64 build's FBX import is stubbed, so prefer **glTF** packs. FBX-only packs go
`FBX → Blender → glTF` (Blender is already in the pipeline) — or use the x86_64 side where FBX works.

## What generation adds (not the library)

| Generated | Tool |
|---|---|
| Restyled / unique surface textures | SDXL-Turbo (`gen/image_gen.py`) |
| Level BGM, stingers | MusicGen (`gen/audio_gen.py music`) |
| Enemy / announcer voice lines | Piper (`gen/audio_gen.py voice`) |
| Procedural level layout of the modular kit | Blender / GeometryScript |
| Gameplay logic | the FPS behavior recipes (below) |

## FPS behavior recipes

Spec `behaviors: [...]` compile into Blueprint graphs (`gameforge.py`). Entity parent should be
`/Script/Engine.Character` and the player auto-possessed (Player 0, via the GameMode).

| behavior | graph |
|---|---|
| `fps_controller` | CameraComponent + mouse look (Turn/LookUp → yaw/pitch) + **camera-relative** WASD move (`DGXGameplayLibrary.GetCameraForward/Right` → AddMovementInput) |
| `jump` | Jump input → `ACharacter::Jump` |
| `shoot` | Fire input → `DGXGameplayLibrary.FireHitscan` (line-trace + ApplyDamage, in runtime C++) |
| `enemy` | Tick → patrol move (chase-the-player is a refinement) |

The hard logic (hitscan trace + damage, yaw-only camera-relative vectors) lives in a **runtime** C++
module (`DGXGameplay`, ships in the packaged game); the recipes just wire input → helper, so they're
robust rather than fragile hand-built graphs. `enemy` chase-the-player is the remaining refinement.

## Build one

With the editor service up (`scripts/start_forge_service.sh`):
1. `pipeline/fetch_spine.py` — get the CC0 spine (once).
2. Drop the manual character/weapon/kit packs into `~/gameforge/library/`.
3. Shape a spec with Claude (see [`specs/fps.yaml`](specs/fps.yaml)) → **"build it"**. Claude generates
   the variation assets, imports spine + generated assets, wires the FPS recipes, lays out the level,
   and packages for SteamOS.

## Licenses

All spine sources are **CC0**: ambientCG, Poly Haven, Kenney, Quaternius. Generated assets are yours.
Keep the spine CC0-only if you ever host this as a service — it sidesteps per-asset EULA questions.
