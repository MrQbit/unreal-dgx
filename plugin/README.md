# DGXMCPTools — Blueprint-authoring editor plugin (aarch64)

A native UE 5.4 C++ **editor plugin** that closes the one gap stock Remote Control can't: it authors
**Blueprint graphs**. It exposes `BlueprintCallable` `UFUNCTION`s (create Blueprint assets, add
components / variables / functions, add & wire graph nodes, compile, spawn) that the Remote Control
HTTP API calls directly — so the MCP can build real gameplay logic, not just place actors.

Built and verified natively on the GB10 (aarch64): compiles clean, all 14 UFUNCTIONs reflected and
callable at `/Script/DGXMCPTools.Default__DGXBlueprintTools`.

## What it exposes

| UFUNCTION | Purpose |
|---|---|
| `CreateBlueprint(pkg, name, parentClass)` | New Blueprint asset → returns its path |
| `AddComponent` / `SetComponentProperty` | Add a component to the SCS; set its defaults |
| `AddVariable` / `SetVariableDefault` | Member variables (bool/int/float/string/vector/object/…) |
| `AddFunctionGraph` | New function graph |
| `AddEventNode` | Overridable event (BeginPlay, Tick, …) → node GUID |
| `AddCallFunctionNode` | Call any UFunction → node GUID |
| `AddVariableNode` | Variable get/set node → node GUID |
| `ConnectNodes` | Wire two pins (exec or data) by node GUID + pin name |
| `CompileBlueprint` / `SaveAsset` | Compile; persist to disk |
| `SpawnActor` / `DescribeGraph` | Spawn into the level; introspect a graph |

The Python MCP server (`mcp/unreal_rc_server.py`) wraps these as `bp_*` tools.

## Deploy + build (on the DGX)

```bash
# 1. Copy the plugin into the engine tree
cp -r /path/to/unreal-dgx/plugin  $HOME/UnrealEngine/Engine/Plugins/Editor/DGXMCPTools
#    (i.e. Engine/Plugins/Editor/DGXMCPTools/{DGXMCPTools.uplugin, Source/...})

# 2. Rebuild the editor (the plugin is EnabledByDefault, so it builds + loads automatically)
cd $HOME/UnrealEngine
export DOTNET_ROOT=$HOME/.dotnet PATH=/usr/local/bin:$HOME/.dotnet:$PATH UE_ARM64_MINIMAL_EDITOR=1
./Engine/Build/BatchFiles/Linux/Build.sh UnrealEditor Linux Development -ForceUseSystemCompiler
```

Then launch the headless editor (`scripts/run_headless_editor.sh`) and the `bp_*` MCP tools work.

## Example: a Blueprint that prints on BeginPlay (what Claude would drive)

```
bp_create("/Game/Blueprints", "BP_Hello", "/Script/Engine.Actor")   -> "/Game/Blueprints/BP_Hello.BP_Hello"
begin = bp_add_event_node(bp, "ReceiveBeginPlay")                   -> "<guidA>"
call  = bp_add_call_node(bp, "PrintString", "/Script/Engine.KismetSystemLibrary", x=400)  -> "<guidB>"
bp_connect(bp, begin, "then", call, "execute")
bp_compile(bp); bp_save("/Game/Blueprints/BP_Hello")
bp_spawn("/Game/Blueprints/BP_Hello.BP_Hello_C", {"Z":100})
```

## Notes

- Editor-only plugin (`Type: Editor`), so it lives in the editor process the MCP drives — not in a
  packaged game. Gameplay it authors (Blueprints, C++) ships normally.
- Source is version-controlled here; it's deployed into `Engine/Plugins/Editor/` to build with the
  engine editor (same pattern as the engine patches).
