# GameForge spec format

A game is described in YAML. GameForge expands it (LLM), generates assets locally, imports them,
assembles Blueprints + levels, and packages for SteamOS.

```yaml
name: MyGame
genre: platformer            # platformer | fps | isometric | ...
style: "hand-painted, warm"  # art direction (feeds the image generator prompts)

assets:                      # generated locally on the GB10, then imported into UE
  - {type: image,  file: hero.png,    prompt: "cute knight sprite sheet, side view", dest: /Game/Sprites}
  - {type: mesh,   file: crate.glb,   kind: crate,                                   dest: /Game/Meshes}
  - {type: music,  file: theme.wav,   prompt: "upbeat chiptune adventure loop"}
  - {type: voice,  file: hi.wav,      text: "Welcome, hero!"}

entities:                   # each becomes a Blueprint (logic wired from behaviors)
  - {name: BP_Player, parent: /Script/Engine.Pawn,
     behaviors: [move_lr, jump],  mesh: /Game/Meshes/crate}
  - {name: BP_Coin,   parent: /Script/Engine.Actor, behaviors: [collectible]}

levels:
  - name: L1
    place:
      - {class: /Script/Engine.StaticMeshActor, at: {X: 0,  Y: 0, Z: 0}}
      - {class: /Game/Blueprints/BP_Player.BP_Player_C, at: {X: 0, Y: 0, Z: 100}}
```

Types: `image` (PNG via diffusers), `mesh` (glTF via Blender), `music`/`voice` (WAV).
`behaviors` map to Blueprint graph recipes wired by the assembler (extend in gameforge.py).
