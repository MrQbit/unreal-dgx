#!/usr/bin/env python3
"""
Fetch the GameForge FPS CC0 asset spine into ~/gameforge/library/<role>/.

  fetch_spine.py [assets/fps_spine.yaml]

Auto-downloads sources with a stable public API (ambientCG textures, Poly Haven HDRIs/models/textures).
For manual sources (Kenney, Quaternius — CC0 but no stable script URL) it prints the page + where to
drop the unzip. All assets are CC0 (public-domain-equivalent): clean for personal AND redistribution.
"""
import json, os, sys, urllib.request, zipfile, io

HERE = os.path.dirname(os.path.abspath(__file__))
try:
    import yaml
except ImportError:
    yaml = None

def _get(url, timeout=120):
    req = urllib.request.Request(url, headers={"User-Agent": "GameForge/1.0"})
    return urllib.request.urlopen(req, timeout=timeout).read()

def fetch_ambientcg(item, dest):
    # https://ambientcg.com/get?file=<Id>_<Res>-<Fmt>.zip  -> zip of PBR maps
    fn = f"{item['id']}_{item['res']}-{item['fmt']}.zip"
    url = f"https://ambientcg.com/get?file={fn}"
    print(f"    ambientCG {fn}")
    z = zipfile.ZipFile(io.BytesIO(_get(url)))
    z.extractall(dest)
    return True

def fetch_polyhaven(item, dest):
    # api.polyhaven.com/files/<id> -> JSON of download URLs per type/res/format
    meta = json.loads(_get(f"https://api.polyhaven.com/files/{item['id']}"))
    typ = item["type"]
    urls = []
    if typ == "hdris":
        res = item.get("res", "2k")
        urls = [meta["hdri"][res]["hdr"]["url"]]
    elif typ == "models":
        res = item.get("res", "1k")
        node = meta["gltf"][res]["gltf"]
        urls = [node["url"]] + [v["url"] for v in node.get("include", {}).values()]
    else:  # textures: grab common PBR maps
        res = item.get("res", "1k")
        for m in ("Diffuse", "nor_gl", "Rough", "AO", "Displacement"):
            if m in meta and res in meta[m]:
                fmt = next(iter(meta[m][res]))
                urls.append(meta[m][res][fmt]["url"])
    for u in urls:
        name = u.split("/")[-1].split("?")[0]
        print(f"    PolyHaven {name}")
        open(os.path.join(dest, name), "wb").write(_get(u))
    return bool(urls)

def main():
    spec_path = sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "assets", "fps_spine.yaml")
    if not yaml:
        sys.exit("pip install pyyaml (or run inside the gameforge venv)")
    spec = yaml.safe_load(open(spec_path))
    root = os.path.expanduser(spec.get("library_root", "~/gameforge/library"))
    manual = []
    for role, items in spec.get("roles", {}).items():
        for item in items:
            dest = os.path.join(root, role, item["name"])
            os.makedirs(dest, exist_ok=True)
            if item.get("fetch") == "api":
                print(f"  [{role}] {item['name']}")
                try:
                    (fetch_ambientcg if item["source"] == "ambientCG" else fetch_polyhaven)(item, dest)
                except Exception as e:
                    print(f"    ! failed ({e}) — grab manually: {item.get('url','(see source)')}")
            else:
                manual.append((role, item))
    if manual:
        print("\n== Manual CC0 grabs (no stable script URL) — download, unzip into library/<role>/<name>/ ==")
        for role, item in manual:
            print(f"  [{role}] {item['name']:32s} {item.get('url','')}")
    print(f"\nLibrary root: {root}")

if __name__ == "__main__":
    main()
