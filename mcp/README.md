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

## Blueprint-graph authoring (bp_* tools)

The **DGXMCPTools** C++ editor plugin (`../plugin/`, built for arm64) adds full Blueprint-**graph**
authoring on top of Remote Control — `bp_create`, `bp_add_component`, `bp_add_variable`,
`bp_add_event_node`, `bp_add_call_node`, `bp_add_variable_node`, `bp_connect`, `bp_compile`,
`bp_save`, `bp_spawn`, `bp_describe_graph`, etc. Build + enable the plugin (see `../plugin/README.md`)
and these tools appear alongside the Remote Control ones. Example — a BeginPlay→PrintString graph:

```
bp = bp_create("/Game/Blueprints", "BP_Hello", "/Script/Engine.Actor")["ReturnValue"]
a  = bp_add_event_node(bp, "ReceiveBeginPlay")["ReturnValue"]
b  = bp_add_call_node(bp, "PrintString", "/Script/Engine.KismetSystemLibrary", x=400)["ReturnValue"]
bp_connect(bp, a, "then", b, "execute"); bp_compile(bp); bp_save("/Game/Blueprints/BP_Hello")
```

## Known limits / notes
- Object paths use the format `/Game/Path/Pkg.Obj:PersistentLevel.ActorName`; CDOs use
  `/Script/<Module>.Default__<Class>`. `functionName` is the **C++** name.
- The editor stays up as long as its process lives; run it under tmux/systemd so it survives your SSH
  session. It renders offscreen — open the same project in the DGX's GUI editor to *see* the result.
