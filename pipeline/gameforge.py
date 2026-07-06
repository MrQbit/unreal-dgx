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
        path = bp_create("/Game/Blueprints", e["name"], e.get("parent", "/Script/Engine.Actor"))
        # components / variables / graph wiring are added here per the entity's spec via rc_call(...)
        bp_compile(path); bp_save("/Game/Blueprints/" + e["name"])
        print("  entity", e["name"], "->", path)
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
