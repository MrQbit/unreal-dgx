# Headless Blender: generate a simple parametric prop and export glTF for UE import.
# Usage: blender --background --python gen_prop.py -- <kind> <out.glb>
#   kind: crate | pillar | coin | platform  (extend as needed)
import bpy, sys, math
argv = sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else ["crate", "/tmp/prop.glb"]
kind, out = (argv + ["crate", "/tmp/prop.glb"])[:2]

# clean scene
bpy.ops.object.select_all(action='SELECT'); bpy.ops.object.delete()

def add_mat(obj, rgba):
    m = bpy.data.materials.new("M"); m.use_nodes = True
    bsdf = m.node_tree.nodes.get("Principled BSDF")
    if bsdf: bsdf.inputs["Base Color"].default_value = rgba
    obj.data.materials.append(m)

if kind == "crate":
    bpy.ops.mesh.primitive_cube_add(size=1.0)
    o = bpy.context.object; bpy.ops.object.modifier_add(type='BEVEL')
    add_mat(o, (0.55, 0.35, 0.15, 1))
elif kind == "pillar":
    bpy.ops.mesh.primitive_cylinder_add(radius=0.4, depth=3.0)
    add_mat(bpy.context.object, (0.8, 0.8, 0.82, 1))
elif kind == "coin":
    bpy.ops.mesh.primitive_cylinder_add(radius=0.5, depth=0.08); 
    o=bpy.context.object; o.rotation_euler[0]=math.radians(90)
    add_mat(o, (1.0, 0.85, 0.1, 1))
else:  # platform
    bpy.ops.mesh.primitive_cube_add(size=1.0); o=bpy.context.object; o.scale=(4,1,0.3)
    add_mat(o, (0.3, 0.5, 0.35, 1))

bpy.ops.object.shade_smooth()
bpy.ops.export_scene.gltf(filepath=out, export_format='GLB', use_selection=False)
print("GLTF_EXPORT_OK", out)
