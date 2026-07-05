# Driving the aarch64 Unreal Editor with Claude Code via MCP

This wires **Claude Code → MCP → Unreal Remote Control HTTP API → the headless aarch64 editor**,
so Claude can spawn actors, set properties, call functions, query/save assets, and build a game —
all on the DGX, no GUI needed. It uses only the stock **RemoteControl** plugin (already enabled in
`DGXGame.uproject`) — no custom C++ plugin and no UE Python plugin.

```
Claude Code ──stdio──▶ unreal_rc_server.py ──HTTP :30010──▶ UnrealEditor (-RenderOffScreen)
```

## 1. Prerequisites (once)

```bash
# Python MCP deps (uv recommended; pip works too)
uv pip install "mcp[cli]" httpx      # or: pip install "mcp[cli]" httpx
```

## 2. Start the headless editor (leave it running)

Run this in a **normal terminal / tmux / screen** on the DGX (a persistent session — not piped
through another program that reaps children):

```bash
tmux new -s ue
UE_ROOT=$HOME/UnrealEngine ./scripts/run_headless_editor.sh $HOME/DGXGame/DGXGame.uproject
# wait until the log shows:  LogHttpListener: Created new HttpListener on 127.0.0.1:30010
# (first launch compiles many shaders and can be slow — see the shader-speed note in docs/STATUS.md)
```

Verify the API is up from another shell:

```bash
curl -s http://127.0.0.1:30010/remote/info | head
```

## 3. Register the MCP server with Claude Code

Either add it via the CLI:

```bash
claude mcp add unreal -- python /path/to/unreal-dgx/mcp/unreal_rc_server.py
```

…or drop a `.mcp.json` in your working directory (see `mcp/claude_mcp.json` here):

```json
{
  "mcpServers": {
    "unreal": {
      "command": "python",
      "args": ["/home/mrqbit/Downloads/unreal-dgx/mcp/unreal_rc_server.py"],
      "env": { "UE_RC_URL": "http://127.0.0.1:30010" }
    }
  }
}
```

Then in Claude Code: `/mcp` should list **unreal** with its tools.

## 4. Tools exposed

| Tool | What it does |
|---|---|
| `unreal_info` | List all Remote Control routes (connectivity check) |
| `unreal_spawn_actor(actor_class, location, rotation)` | Spawn an actor into the current level |
| `unreal_list_actors()` | List all actors in the level |
| `unreal_call_function(object_path, function_name, parameters)` | Call any UFunction |
| `unreal_get_property` / `unreal_set_property` | Read / write a property |
| `unreal_describe(object_path)` | List an object's properties + callable functions |
| `unreal_search_assets(query, class_names, package_paths)` | Asset Registry query |
| `unreal_save_asset` / `unreal_save_current_level` | Persist changes |
| `unreal_raw(method, path, body)` | Send an arbitrary Remote Control request |

### Example (what Claude can do)

- "Spawn a PointLight at Z=300" → `unreal_spawn_actor("/Script/Engine.PointLight", {"Z":300})`
- "Add a cube and move it" → spawn `/Script/Engine.StaticMeshActor`, then `unreal_call_function`
  the actor's `SetActorLocation`.
- "List everything in the level" → `unreal_list_actors()`.

## Known limits / notes

- **Blueprint node graphs** can't be authored over Remote Control — it calls exposed UFunctions and
  sets properties, and can *create* Blueprint assets and set their defaults, but not wire event
  graphs. Games built from spawned/configured actors + C++ gameplay classes + property tweaks work
  fully. For graph authoring you'd add a custom C++ editor plugin (e.g. chongdashu/unreal-mcp) built
  for arm64 — a normal engine-source build.
- Object paths use the format `/Game/Path/Pkg.Obj:PersistentLevel.ActorName`; CDOs use
  `/Script/<Module>.Default__<Class>`. `functionName` is the **C++** name.
- The editor stays up as long as its process lives; run it under tmux/systemd so it survives your SSH
  session. It renders offscreen — open the same project in the DGX's GUI editor to *see* the result.
