#!/usr/bin/env python3
"""
Unreal Engine MCP server — bridges MCP tool calls to the Unreal Editor's built-in
Remote Control HTTP API (http://127.0.0.1:30010). No custom UE plugin and no UE Python
plugin required — only the stock "RemoteControl" plugin (already enabled in the project).

Start the headless editor first (scripts/run_headless_editor.sh), then run this server
from an MCP client (Claude Code). See mcp/README.md.

Deps:  pip install "mcp[cli]" httpx     (or: uv add "mcp[cli]" httpx)
Run:   python unreal_rc_server.py        (stdio transport)

Env:
  UE_RC_URL   base URL of the Remote Control HTTP server (default http://127.0.0.1:30010)
"""
import os
import json
import httpx
from mcp.server.fastmcp import FastMCP

RC_URL = os.environ.get("UE_RC_URL", "http://127.0.0.1:30010").rstrip("/")
_client = httpx.Client(base_url=RC_URL, timeout=30.0)

mcp = FastMCP("unreal-remote-control")


def _req(method: str, path: str, body: dict | None = None) -> dict:
    """Send a Remote Control request and return parsed JSON (or a status/error dict)."""
    try:
        r = _client.request(method, path, json=body)
    except httpx.ConnectError:
        return {"error": f"cannot reach Unreal Remote Control at {RC_URL} — is the headless editor running?"}
    out: dict = {"status": r.status_code}
    text = r.text.strip()
    if text:
        try:
            out["result"] = r.json()
        except json.JSONDecodeError:
            out["result"] = text
    if r.status_code >= 400:
        out["error"] = f"HTTP {r.status_code}"
    return out


# Common editor scripting objects. Static UBlueprintFunctionLibrary functions are callable
# directly on the class CDO, which is the reliable route for editor operations over RC.
EDITOR_ACTOR_SUBSYSTEM = "/Script/UnrealEd.Default__EditorActorSubsystem"
EDITOR_LEVEL_LIBRARY   = "/Script/UnrealEd.Default__EditorLevelLibrary"    # deprecated but present in 5.4
EDITOR_ASSET_LIBRARY   = "/Script/EditorScriptingUtilities.Default__EditorAssetLibrary"
LEVEL_EDITOR_SUBSYSTEM = "/Script/UnrealEd.Default__LevelEditorSubsystem"


@mcp.tool()
def unreal_info() -> dict:
    """List all available Remote Control HTTP routes (GET /remote/info). Good connectivity check."""
    return _req("GET", "/remote/info")


@mcp.tool()
def unreal_call_function(object_path: str, function_name: str,
                         parameters: dict | None = None, transaction: bool = True) -> dict:
    """Call a UFunction on an object (PUT /remote/object/call).

    object_path   e.g. "/Game/Maps/Main.Main:PersistentLevel.Cube_5" or a CDO like
                  "/Script/UnrealEd.Default__EditorLevelLibrary".
    function_name the C++ function name (NOT the Blueprint display name).
    parameters    dict of named args, e.g. {"NewLocation": {"X":100,"Y":0,"Z":30}}.
    transaction   record an undo transaction (default True).
    """
    body = {"objectPath": object_path, "functionName": function_name,
            "parameters": parameters or {}, "generateTransaction": transaction}
    return _req("PUT", "/remote/object/call", body)


@mcp.tool()
def unreal_get_property(object_path: str, property_name: str | None = None) -> dict:
    """Read a property (or all properties if property_name is omitted) — PUT /remote/object/property."""
    body = {"objectPath": object_path, "access": "READ_ACCESS"}
    if property_name:
        body["propertyName"] = property_name
    return _req("PUT", "/remote/object/property", body)


@mcp.tool()
def unreal_set_property(object_path: str, property_name: str, value, transaction: bool = True) -> dict:
    """Write a property (PUT /remote/object/property).

    value is the new value; it is wrapped as {property_name: value} in the request.
    """
    body = {"objectPath": object_path, "propertyName": property_name,
            "access": "WRITE_TRANSACTION_ACCESS" if transaction else "WRITE_ACCESS",
            "propertyValue": {property_name: value}}
    return _req("PUT", "/remote/object/property", body)


@mcp.tool()
def unreal_describe(object_path: str) -> dict:
    """List an object's properties and callable functions (PUT /remote/object/describe)."""
    return _req("PUT", "/remote/object/describe", {"objectPath": object_path})


@mcp.tool()
def unreal_search_assets(query: str = "", class_names: list[str] | None = None,
                         package_paths: list[str] | None = None, recursive: bool = True) -> dict:
    """Query the Asset Registry (PUT /remote/search/assets).

    class_names   e.g. ["Blueprint", "StaticMesh"]; package_paths e.g. ["/Game"].
    """
    filt: dict = {"RecursivePaths": recursive}
    if class_names:
        filt["ClassNames"] = class_names
    if package_paths:
        filt["PackagePaths"] = package_paths
    return _req("PUT", "/remote/search/assets", {"Query": query, "Filter": filt})


@mcp.tool()
def unreal_spawn_actor(actor_class: str, location: dict | None = None,
                       rotation: dict | None = None) -> dict:
    """Spawn an actor into the current level.

    actor_class  full class path, e.g. "/Script/Engine.PointLight",
                 "/Script/Engine.StaticMeshActor", or "/Game/BP/BP_Player.BP_Player_C".
    location/rotation  dicts like {"X":0,"Y":0,"Z":100} / {"Pitch":0,"Yaw":0,"Roll":0}.

    Uses EditorActorSubsystem.SpawnActorFromClass (falls back to EditorLevelLibrary).
    """
    params = {"ActorClass": actor_class,
              "Location": location or {"X": 0, "Y": 0, "Z": 0},
              "Rotation": rotation or {"Pitch": 0, "Yaw": 0, "Roll": 0}}
    res = unreal_call_function(EDITOR_ACTOR_SUBSYSTEM, "SpawnActorFromClass", params)
    if res.get("error"):
        res = unreal_call_function(EDITOR_LEVEL_LIBRARY, "SpawnActorFromClass", params)
    return res


@mcp.tool()
def unreal_list_actors() -> dict:
    """List all actors in the current level (EditorActorSubsystem.GetAllLevelActors)."""
    return unreal_call_function(EDITOR_ACTOR_SUBSYSTEM, "GetAllLevelActors", {}, transaction=False)


@mcp.tool()
def unreal_save_asset(asset_path: str) -> dict:
    """Save an asset by package path, e.g. "/Game/Maps/Main" (EditorAssetLibrary.SaveAsset)."""
    return unreal_call_function(EDITOR_ASSET_LIBRARY, "SaveAsset",
                                {"AssetToSave": asset_path, "bOnlyIfIsDirty": False})


@mcp.tool()
def unreal_save_current_level() -> dict:
    """Save the currently loaded level (LevelEditorSubsystem.SaveCurrentLevel)."""
    return unreal_call_function(LEVEL_EDITOR_SUBSYSTEM, "SaveCurrentLevel", {})


@mcp.tool()
def unreal_raw(method: str, path: str, body: dict | None = None) -> dict:
    """Escape hatch: send an arbitrary Remote Control request.
    method "GET"/"PUT"; path like "/remote/object/call"; body is the JSON payload."""
    return _req(method.upper(), path, body)


# ============================================================================
# Blueprint AUTHORING — via the DGXMCPTools C++ editor plugin (built for arm64).
# These go beyond stock Remote Control: they create/edit/compile Blueprint GRAPHS.
# All call the plugin's function library CDO.
# ============================================================================
DGX_BP_TOOLS = "/Script/DGXMCPTools.Default__DGXBlueprintTools"


def _bp(func: str, params: dict) -> dict:
    """Call a DGXBlueprintTools function; return its ReturnValue (or the full result on error)."""
    res = unreal_call_function(DGX_BP_TOOLS, func, params)
    if res.get("error"):
        return res
    r = res.get("result", {})
    return {"ReturnValue": r.get("ReturnValue")} if isinstance(r, dict) and "ReturnValue" in r else res


@mcp.tool()
def bp_create(package_path: str, name: str, parent_class: str = "/Script/Engine.Actor") -> dict:
    """Create a new Blueprint asset. package_path e.g. "/Game/Blueprints", parent_class e.g.
    "/Script/Engine.Actor" or "/Script/Engine.Pawn". Returns the new Blueprint's object path."""
    return _bp("CreateBlueprint", {"PackagePath": package_path, "AssetName": name, "ParentClassPath": parent_class})


@mcp.tool()
def bp_add_component(blueprint_path: str, component_class: str, name: str) -> dict:
    """Add a component to a Blueprint, e.g. component_class "/Script/Engine.StaticMeshComponent"."""
    return _bp("AddComponent", {"BlueprintPath": blueprint_path, "ComponentClassPath": component_class, "ComponentName": name})


@mcp.tool()
def bp_set_component_property(blueprint_path: str, component_name: str, property_name: str, value: str) -> dict:
    """Set a default property on a component. value is an import string (e.g. "(X=1,Y=2,Z=3)" for a vector)."""
    return _bp("SetComponentProperty", {"BlueprintPath": blueprint_path, "ComponentName": component_name,
                                        "PropertyName": property_name, "Value": value})


@mcp.tool()
def bp_add_variable(blueprint_path: str, name: str, var_type: str = "float") -> dict:
    """Add a member variable. var_type: bool,int,int64,float,double,string,name,text,vector,rotator,transform,
    or an object class path (e.g. "/Script/Engine.Actor")."""
    return _bp("AddVariable", {"BlueprintPath": blueprint_path, "VarName": name, "VarType": var_type})


@mcp.tool()
def bp_set_variable_default(blueprint_path: str, name: str, value: str) -> dict:
    """Set a member variable's default value (import string)."""
    return _bp("SetVariableDefault", {"BlueprintPath": blueprint_path, "VarName": name, "Value": value})


@mcp.tool()
def bp_add_function(blueprint_path: str, function_name: str) -> dict:
    """Add a new (empty) function graph to a Blueprint."""
    return _bp("AddFunctionGraph", {"BlueprintPath": blueprint_path, "FunctionName": function_name})


@mcp.tool()
def bp_add_event_node(blueprint_path: str, event_name: str = "ReceiveBeginPlay", x: float = 0, y: float = 0) -> dict:
    """Add an overridable event node to the event graph (e.g. "ReceiveBeginPlay", "ReceiveTick").
    Returns the node GUID (use with bp_connect)."""
    return _bp("AddEventNode", {"BlueprintPath": blueprint_path, "EventName": event_name, "NodePosX": x, "NodePosY": y})


@mcp.tool()
def bp_add_call_node(blueprint_path: str, function_name: str, target_class: str = "",
                     graph: str = "", x: float = 300, y: float = 0) -> dict:
    """Add a 'call function' node. target_class is the class owning the function
    ("/Script/Engine.KismetSystemLibrary" for statics; "" = self). graph "" = event graph.
    Returns the node GUID."""
    return _bp("AddCallFunctionNode", {"BlueprintPath": blueprint_path, "GraphName": graph,
                                       "TargetClassPath": target_class, "FunctionName": function_name,
                                       "NodePosX": x, "NodePosY": y})


@mcp.tool()
def bp_add_variable_node(blueprint_path: str, var_name: str, setter: bool = False,
                         graph: str = "", x: float = 0, y: float = 0) -> dict:
    """Add a variable get (setter=False) or set (setter=True) node. Returns the node GUID."""
    return _bp("AddVariableNode", {"BlueprintPath": blueprint_path, "GraphName": graph,
                                   "VarName": var_name, "bIsSetter": setter, "NodePosX": x, "NodePosY": y})


@mcp.tool()
def bp_connect(blueprint_path: str, node_a_guid: str, pin_a: str,
               node_b_guid: str, pin_b: str, graph: str = "") -> dict:
    """Connect two node pins. Exec pins: out="then", in="execute". Data pins use the parameter name."""
    return _bp("ConnectNodes", {"BlueprintPath": blueprint_path, "GraphName": graph,
                                "NodeAGuid": node_a_guid, "PinAName": pin_a,
                                "NodeBGuid": node_b_guid, "PinBName": pin_b})


@mcp.tool()
def bp_describe_graph(blueprint_path: str, graph: str = "") -> dict:
    """List nodes (GUIDs, titles, pins) in a graph — inspect what's been built. graph "" = event graph."""
    return _bp("DescribeGraph", {"BlueprintPath": blueprint_path, "GraphName": graph})


@mcp.tool()
def bp_compile(blueprint_path: str) -> dict:
    """Compile a Blueprint. Returns ReturnValue=true if it compiled cleanly."""
    return _bp("CompileBlueprint", {"BlueprintPath": blueprint_path})


@mcp.tool()
def bp_save(asset_path: str) -> dict:
    """Save a Blueprint/asset to disk, e.g. asset_path "/Game/Blueprints/BP_Foo"."""
    return _bp("SaveAsset", {"AssetPath": asset_path})


@mcp.tool()
def bp_spawn(class_path: str, location: dict | None = None, rotation: dict | None = None) -> dict:
    """Spawn an actor from a Blueprint or native class into the editor level (via the plugin).
    class_path e.g. "/Game/Blueprints/BP_Foo.BP_Foo_C" or "/Script/Engine.PointLight"."""
    return _bp("SpawnActor", {"ClassPath": class_path,
                              "Location": location or {"X": 0, "Y": 0, "Z": 0},
                              "Rotation": rotation or {"Pitch": 0, "Yaw": 0, "Roll": 0}})


@mcp.tool()
def asset_import(source_file: str, dest_path: str = "/Game/Imported") -> dict:
    """Import a generated asset file into the project (the generation pipeline entry point).
    Handles .glb/.gltf meshes (Interchange), .png/.jpg/.tga textures, .wav sounds, etc.
    source_file is an absolute path on disk; dest_path is a content path. Returns the asset path."""
    return _bp("ImportAsset", {"SourceFile": source_file, "DestPath": dest_path})


@mcp.tool()
def asset_set_actor_mesh(actor_path: str, static_mesh_path: str) -> dict:
    """Assign an imported StaticMesh to a StaticMeshActor already in the level."""
    return _bp("SetActorStaticMesh", {"ActorPath": actor_path, "StaticMeshPath": static_mesh_path})


if __name__ == "__main__":
    mcp.run()
