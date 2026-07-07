#!/usr/bin/env python3
"""
GameForge — orchestrate a game spec into a built SteamOS game, all locally on the DGX Spark.

  gameforge.py <spec.yaml> [--project ~/PongGame/PongGame.uproject] [--stage all|assets|assemble|package]

Pipeline: (1) expand spec -> asset manifest  (2) generate assets locally (Blender/diffusers/audio)
(3) import into UE  (4) assemble Blueprints+levels via the Remote Control MCP  (5) package for x64/SteamOS.

Prereqs: headless UE editor with Remote Control running (scripts/run_headless_editor.sh) + pipeline/setup.sh.
This is the spine; generation/assembly detail is spec-driven and extended per game. No external APIs.
"""
import argparse, json, os, subprocess, sys, urllib.request
try:
    import yaml
except ImportError:
    yaml = None

HERE = os.path.dirname(os.path.abspath(__file__))
RC = os.environ.get("UE_RC_URL", "http://127.0.0.1:30010").rstrip("/")
FORGE = os.environ.get("FORGE_HOME", os.path.expanduser("~/gameforge"))
VENV_PY = os.path.join(FORGE, ".venv", "bin", "python")
BP_TOOLS = "/Script/DGXMCPTools.Default__DGXBlueprintTools"


# ---- Remote Control (UE) ----
def rc_call(func, params):
    body = json.dumps({"objectPath": BP_TOOLS, "functionName": func,
                       "parameters": params, "generateTransaction": True}).encode()
    req = urllib.request.Request(RC + "/remote/object/call", data=body,
                                 headers={"Content-Type": "application/json"}, method="PUT")
    with urllib.request.urlopen(req, timeout=120) as r:
        out = json.loads(r.read() or b"{}")
    return out.get("ReturnValue", out)

def import_asset(src, dest):        return rc_call("ImportAsset", {"SourceFile": src, "DestPath": dest})
def bp_create(pkg, name, parent):   return rc_call("CreateBlueprint", {"PackagePath": pkg, "AssetName": name, "ParentClassPath": parent})
def bp_compile(path):               return rc_call("CompileBlueprint", {"BlueprintPath": path})
def bp_save(path):                  return rc_call("SaveAsset", {"AssetPath": path})
def spawn(cls, loc):                return rc_call("SpawnActor", {"ClassPath": cls, "Location": loc, "Rotation": {"Pitch":0,"Yaw":0,"Roll":0}})

# node/graph primitives (graph "" = event graph)
def bp_component(p, cls, name):      return rc_call("AddComponent", {"BlueprintPath": p, "ComponentClassPath": cls, "ComponentName": name})
def bp_event(p, name, x, y):         return rc_call("AddEventNode", {"BlueprintPath": p, "EventName": name, "NodePosX": x, "NodePosY": y})
def bp_input_axis(p, axis, x, y):    return rc_call("AddInputAxisEvent", {"BlueprintPath": p, "AxisName": axis, "NodePosX": x, "NodePosY": y})
def bp_call(p, cls, fn, x, y):       return rc_call("AddCallFunctionNode", {"BlueprintPath": p, "GraphName": "", "TargetClassPath": cls, "FunctionName": fn, "NodePosX": x, "NodePosY": y})
def bp_connect(p, a, pa, b, pb):     return rc_call("ConnectNodes", {"BlueprintPath": p, "GraphName": "", "NodeAGuid": a, "PinAName": pa, "NodeBGuid": b, "PinBName": pb})
def bp_pin(p, node, pin, val):       return rc_call("SetPinDefault", {"BlueprintPath": p, "GraphName": "", "NodeGuid": node, "PinName": pin, "Value": val})

ACTOR = "/Script/Engine.Actor"
KSL   = "/Script/Engine.KismetSystemLibrary"

# ---- behavior recipes: behavior name -> function(bp_path) that builds a compiling gameplay graph ----
def _recipe_hello(p):
    ev  = bp_event(p, "ReceiveBeginPlay", -400, 0)
    ps  = bp_call(p, KSL, "PrintString", 0, 0)
    bp_pin(p, ps, "InString", "GameForge entity ready")
    bp_connect(p, ev, "then", ps, "execute")

def _recipe_spin(p):
    ev  = bp_event(p, "ReceiveTick", -400, 0)
    rot = bp_call(p, "", "K2_AddActorLocalRotation", 0, 0)   # "" = self/Actor
    bp_pin(p, rot, "DeltaRotation", "(Pitch=0.0,Yaw=2.0,Roll=0.0)")
    bp_connect(p, ev, "then", rot, "execute")

def _recipe_auto_move(p):
    ev  = bp_event(p, "ReceiveTick", -400, 0)
    mv  = bp_call(p, "", "K2_AddActorWorldOffset", 0, 0)
    bp_pin(p, mv, "DeltaLocation", "(X=0.0,Y=3.0,Z=0.0)")
    bp_connect(p, ev, "then", mv, "execute")

def _recipe_input_move(p, axis, direction):
    # add a movement component so AddMovementInput takes effect, then wire the axis value into it
    bp_component(p, "/Script/Engine.FloatingPawnMovement", "Movement")
    ax  = bp_input_axis(p, axis, -400, 0)
    mv  = bp_call(p, "/Script/Engine.Pawn", "AddMovementInput", 0, 0)
    bp_pin(p, mv, "WorldDirection", direction)
    bp_connect(p, ax, "then", mv, "execute")
    bp_connect(p, ax, "Axis Value", mv, "ScaleValue")   # float -> float, clean

PAWN = "/Script/Engine.Pawn"
CHAR = "/Script/Engine.Character"

# ---- FPS recipes (entity parent should be /Script/Engine.Character; auto-possess Player 0) ----
def _recipe_fps_controller(p):
    """First-person controller: camera + mouse look (Turn/LookUp) + WASD move (MoveForward/MoveRight)."""
    bp_component(p, "/Script/Engine.CameraComponent", "FPCamera")
    # look: axis "Axis Value" (float) -> controller yaw/pitch (float) — clean wire
    for axis, fn in (("Turn", "AddControllerYawInput"), ("LookUp", "AddControllerPitchInput")):
        ax = bp_input_axis(p, axis, -420, 0)
        n  = bp_call(p, PAWN, fn, -60, 0)
        bp_connect(p, ax, "then", n, "execute")
        bp_connect(p, ax, "Axis Value", n, "Val")
    # move: axis value -> AddMovementInput.ScaleValue (world-axis; camera-relative is a refinement)
    for axis, direction in (("MoveForward", "(X=1.0,Y=0.0,Z=0.0)"), ("MoveRight", "(X=0.0,Y=1.0,Z=0.0)")):
        ax = bp_input_axis(p, axis, -420, 0)
        mv = bp_call(p, PAWN, "AddMovementInput", -60, 0)
        bp_pin(p, mv, "WorldDirection", direction)
        bp_connect(p, ax, "then", mv, "execute")
        bp_connect(p, ax, "Axis Value", mv, "ScaleValue")

def _recipe_jump(p):
    ax = bp_input_axis(p, "Jump", -400, 200)   # (an action mapping named "Jump" also works)
    jump = bp_call(p, CHAR, "Jump", 0, 200)
    bp_connect(p, ax, "then", jump, "execute")

def _recipe_shoot(p):
    # Fire input -> hitscan. A full LineTraceByChannel graph is many fragile nodes; ship a clear hook
    # (PrintString) that the agent replaces with a trace+damage graph per game. TODO: line trace.
    ax = bp_input_axis(p, "Fire", -400, 400)
    ps = bp_call(p, KSL, "PrintString", 0, 400)
    bp_pin(p, ps, "InString", "Fire")
    bp_connect(p, ax, "then", ps, "execute")

def _recipe_enemy(p):
    # simple patrol: Tick -> move forward. Chase-the-player is a refinement (GetPlayerPawn -> direction).
    _recipe_auto_move(p)

RECIPES = {
    "hello":     _recipe_hello,
    "spin":      _recipe_spin,
    "collectible": _recipe_spin,          # spinning pickup
    "auto_move": _recipe_auto_move,
    "bounce":    _recipe_auto_move,       # placeholder motion; refine per game
    "move_lr":   lambda p: _recipe_input_move(p, "MoveRight", "(X=1.0,Y=0.0,Z=0.0)"),
    "move_ud":   lambda p: _recipe_input_move(p, "MoveUp",    "(X=0.0,Y=0.0,Z=1.0)"),
    # FPS
    "fps_controller": _recipe_fps_controller,
    "jump":      _recipe_jump,
    "shoot":     _recipe_shoot,
    "enemy":     _recipe_enemy,
}


# ---- local generators (all on the GB10) ----
def gen_mesh(kind, out):
    subprocess.run(["blender", "--background", "--python", os.path.join(HERE, "blender", "gen_prop.py"),
                    "--", kind, out], check=True, stdout=subprocess.DEVNULL)
    return out

def gen_image(prompt, out, **kw):
    subprocess.run([VENV_PY, os.path.join(HERE, "gen", "image_gen.py"), prompt, out], check=True)
    return out

def gen_audio(mode, text, out):
    subprocess.run([VENV_PY, os.path.join(HERE, "gen", "audio_gen.py"), mode, text, out], check=True)
    return out

def llm_expand(spec_text):
    """Use the local LLM to expand a terse spec into a structured asset+design manifest (best-effort)."""
    prompt = ("Expand this game spec into JSON with keys: entities[], assets[], levels[]. "
              "Keep it concise and buildable.\n\n" + spec_text)
    try:
        p = subprocess.run(["ollama", "run", os.environ.get("FORGE_LLM", "nemotron-3-super:120b"), prompt],
                           capture_output=True, text=True, timeout=600)
        return p.stdout
    except Exception as e:
        return f"(LLM expansion skipped: {e})"


# ---- stages ----
def stage_assets(spec):
    os.makedirs(os.path.join(FORGE, "assets"), exist_ok=True)
    for a in spec.get("assets", []):
        out = os.path.join(FORGE, "assets", a["file"])
        t = a["type"]
        if t == "mesh":     gen_mesh(a.get("kind", "crate"), out)
        elif t == "image":  gen_image(a["prompt"], out)
        elif t == "music":  gen_audio("music", a["prompt"], out)
        elif t == "voice":  gen_audio("voice", a["text"], out)
        dest = a.get("dest", "/Game/Imported")
        print("  import", out, "->", import_asset(out, dest))

def stage_assemble(spec):
    for e in spec.get("entities", []):
        path = bp_create("/Game/Blueprints", e["name"], e.get("parent", ACTOR))
        if not path:
            print("  entity", e["name"], "-> FAILED to create"); continue
        # give it a visible body if a mesh was specified (mesh asset set via SetComponentProperty)
        if e.get("mesh"):
            bp_component(path, "/Script/Engine.StaticMeshComponent", "Body")
            rc_call("SetComponentProperty", {"BlueprintPath": path, "ComponentName": "Body",
                    "PropertyName": "StaticMesh", "Value": e["mesh"]})
        # wire each behavior into a real gameplay graph
        for b in e.get("behaviors", []):
            fn = RECIPES.get(b)
            if fn: fn(path); print("    behavior", b, "wired")
            else:  print("    behavior", b, "(no recipe yet)")
        ok = bp_compile(path); bp_save("/Game/Blueprints/" + e["name"])
        print("  entity", e["name"], "-> compiled:", ok)
    for lv in spec.get("levels", []):
        for p in lv.get("place", []):
            print("  place", p["class"], "->", spawn(p["class"], p.get("at", {"X":0,"Y":0,"Z":0})))

def stage_package(project, arch="x64"):
    subprocess.run([os.path.join(HERE, "..", "scripts", "package_game.sh"), project, arch], check=False)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("spec"); ap.add_argument("--project", default=os.path.expanduser("~/PongGame/PongGame.uproject"))
    ap.add_argument("--stage", default="all", choices=["all", "expand", "assets", "assemble", "package"])
    a = ap.parse_args()
    raw = open(a.spec).read()
    spec = yaml.safe_load(raw) if yaml else json.loads(raw)

    if a.stage in ("all", "expand"):  print(llm_expand(raw)[:2000])
    if a.stage in ("all", "assets"):    print("== generate + import assets =="); stage_assets(spec)
    if a.stage in ("all", "assemble"):  print("== assemble game =="); stage_assemble(spec)
    if a.stage in ("all", "package"):   print("== package (SteamOS x64) =="); stage_package(a.project)

if __name__ == "__main__":
    main()
