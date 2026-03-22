"""
Replace TRUE/FALSE/INFINITY with UC_TRUE/UC_FALSE/UC_INFINITY.

Step 2: old/ files — replace in all files from audit list.
Step 3: new/ files — replace per-entity based on uc_orig comments.

Usage: python tools/replace_defines.py
"""

import re
import os

BASE = r"c:\WORK\OpenChaos\new_game\src"

# Word-boundary replacement patterns
REPLACEMENTS = [
    (re.compile(r'\bINFINITY\b'), 'UC_INFINITY'),
    (re.compile(r'\bTRUE\b'), 'UC_TRUE'),
    (re.compile(r'\bFALSE\b'), 'UC_FALSE'),
]

# Audit results: files where each macro resolves through our #define.
# Paths are relative to new_game/src/ (without src/ prefix in audit, but we add old/ prefix).
# Union of all three lists (TRUE, FALSE, INFINITY).
# Format: original path -> set of macros that need replacing
AUDIT_TRUE_FILES = {
    "fallen/DDEngine/Source/polyrenderstate.cpp",
    "fallen/DDEngine/Source/aeng.cpp",
    "fallen/Source/collide.cpp",
    "fallen/Source/pcom.cpp",
    "fallen/Source/interfac.cpp",
    "fallen/DDEngine/Source/panel.cpp",
    "fallen/Source/frontend.cpp",
    "fallen/Source/eway.cpp",
    "fallen/DDEngine/Source/figure.cpp",
    "fallen/DDEngine/Source/engineMap.cpp",
    "fallen/Source/Person.cpp",
    "fallen/DDLibrary/Source/DDManager.cpp",
    "fallen/outro/os.cpp",
    "fallen/DDEngine/Source/poly.cpp",
    "fallen/Source/Controls.cpp",
    "fallen/Source/Attract.cpp",
    "fallen/DDEngine/Source/shape.cpp",
    "fallen/Source/Game.cpp",
    "fallen/DDEngine/Source/drawxtra.cpp",
    "fallen/DDEngine/Source/sky.cpp",
    "fallen/DDEngine/Source/flamengine.cpp",
    "fallen/Source/elev.cpp",
    "fallen/Source/Vehicle.cpp",
    "fallen/DDEngine/Source/texture.cpp",
    "fallen/DDEngine/Source/renderstate.cpp",
    "fallen/DDEngine/Source/fastprim.cpp",
    "fallen/DDLibrary/Source/GHost.cpp",
    "fallen/DDLibrary/Source/DIManager.cpp",
    "fallen/DDLibrary/Source/D3DTexture.cpp",
    "fallen/DDEngine/Source/truetype.cpp",
    "fallen/DDEngine/Source/superfacet.cpp",
    "fallen/DDEngine/Source/polypage.cpp",
    "fallen/DDEngine/Source/farfacet.cpp",
    "fallen/DDEngine/Source/Text.cpp",
    "fallen/Source/dirt.cpp",
    "fallen/DDLibrary/Source/WindProcs.cpp",
    "fallen/DDEngine/Source/font2d.cpp",
    "fallen/Source/pause.cpp",
    "fallen/Source/overlay.cpp",
    "fallen/DDLibrary/Source/GKeyboard.cpp",
    "fallen/DDLibrary/Source/GDisplay.cpp",
    "fallen/DDEngine/Source/qeng.cpp",
    "fallen/DDEngine/Source/oval.cpp",
    "fallen/Source/Main.cpp",
    "fallen/DDLibrary/Source/net.cpp",
    "fallen/DDLibrary/Source/GWorkScreen.cpp",
    "fallen/DDLibrary/Source/GMouse.cpp",
    "fallen/DDLibrary/Source/FileClump.cpp",
    "fallen/DDLibrary/Source/Drive.cpp",
    "fallen/DDLibrary/Source/BinkClient.cpp",
    "fallen/DDLibrary/Source/AsyncFile2.cpp",
    "fallen/DDEngine/Source/wibble.cpp",
    "fallen/DDEngine/Source/vertexbuffer.cpp",
    "fallen/DDEngine/Source/planmap.cpp",
    "fallen/DDEngine/Source/console.cpp",
    "fallen/DDEngine/Source/Gamut.cpp",
    "fallen/DDEngine/Source/Font.cpp",
    "fallen/DDEngine/Source/Bucket.cpp",
    "fallen/Source/widget.cpp",
    "fallen/DDEngine/Source/facet.cpp",
    "fallen/Source/road.cpp",
    "fallen/Source/mav.cpp",
    "fallen/Source/outline.cpp",
    "fallen/Source/fc.cpp",
    "fallen/Source/Special.cpp",
    "fallen/Source/ware.cpp",
    "fallen/Source/save.cpp",
    "fallen/Source/ns.cpp",
    "fallen/outro/imp.cpp",
    "fallen/Source/night.cpp",
    "fallen/Source/hm.cpp",
    "fallen/DDEngine/Source/menufont.cpp",
    "fallen/DDEngine/Source/Crinkle.cpp",
    "fallen/Source/ob.cpp",
    "fallen/Source/Combat.cpp",
    "fallen/DDEngine/Source/Quaternion.cpp",
    "fallen/Source/wmove.cpp",
    "fallen/Source/cnet.cpp",
    "fallen/Source/barrel.cpp",
    "fallen/Source/Prim.cpp",
    "fallen/DDEngine/Source/sprite.cpp",
    "fallen/DDEngine/Source/comp.cpp",
    "fallen/outro/credits.cpp",
    "fallen/Source/wand.cpp",
    "fallen/Source/pap.cpp",
    "fallen/Source/Thing.cpp",
    "fallen/Source/Darci.cpp",
    "fallen/DDEngine/Source/mesh.cpp",
    "fallen/DDEngine/Source/cone.cpp",
    "fallen/outro/outroTga.cpp",
    "fallen/outro/outroFont.cpp",
    "fallen/Source/stair.cpp",
    "fallen/Source/spark.cpp",
    "fallen/Source/qmap.cpp",
    "fallen/Source/puddle.cpp",
    "fallen/Source/plat.cpp",
    "fallen/Source/music.cpp",
    "fallen/Source/memory.cpp",
    "fallen/Source/io.cpp",
    "fallen/Source/guns.cpp",
    "fallen/Source/grenade.cpp",
    "fallen/Source/bat.cpp",
    "fallen/DDLibrary/Source/Tga.cpp",
    "fallen/outro/wire.cpp",
    "fallen/outro/mf.cpp",
    "fallen/outro/back.cpp",
    "fallen/Source/supermap.cpp",
    "fallen/Source/snipe.cpp",
    "fallen/Source/sm.cpp",
    "fallen/Source/qedit.cpp",
    "fallen/Source/pyro.cpp",
    "fallen/Source/psystem.cpp",
    "fallen/Source/maths.cpp",
    "fallen/Source/gamemenu.cpp",
    "fallen/Source/door.cpp",
    "fallen/DDLibrary/Source/MFX.cpp",
    "fallen/DDEngine/Source/smap.cpp",
    "fallen/DDEngine/Source/ic.cpp",
    "MFStdLib/Source/StdLib/StdMem.cpp",
    "MFStdLib/Source/StdLib/StdFile.cpp",
}

# Files only in FALSE list (not in TRUE)
AUDIT_FALSE_EXTRA = {
    "fallen/Source/playcuts.cpp",
    "fallen/Source/hook.cpp",
    "fallen/Source/dike.cpp",
    "fallen/Source/Sound.cpp",
    "fallen/Source/pow.cpp",
    "fallen/Source/animal.cpp",
    "fallen/Source/tracks.cpp",
    "fallen/Source/drip.cpp",
    "fallen/DDEngine/Source/build.cpp",
    "fallen/outro/outroMain.cpp",
    "fallen/outro/cam.cpp",
    "fallen/Source/walkable.cpp",
    "fallen/Source/trip.cpp",
    "fallen/Source/soundenv.cpp",
    "fallen/Source/shadow.cpp",
    "fallen/Source/ribbon.cpp",
    "fallen/Source/morph.cpp",
    "fallen/Source/mist.cpp",
    "fallen/Source/interact.cpp",
    "fallen/Source/inside2.cpp",
    "fallen/Source/heap.cpp",
    "fallen/Source/glitter.cpp",
    "fallen/Source/fog.cpp",
    "fallen/Source/fire.cpp",
    "fallen/Source/env2.cpp",
    "fallen/Source/drawtype.cpp",
    "fallen/Source/chopper.cpp",
    "fallen/Source/canid.cpp",
    "fallen/Source/build2.cpp",
    "fallen/Source/balloon.cpp",
    "fallen/Source/animtmap.cpp",
    "fallen/Source/Thug.cpp",
    "fallen/Source/Switch.cpp",
    "fallen/Source/State.cpp",
    "fallen/Source/Roper.cpp",
    "fallen/Source/Player.cpp",
    "fallen/Source/Pjectile.cpp",
    "fallen/Source/Map.cpp",
    "fallen/Source/Hierarchy.cpp",
    "fallen/Source/FMatrix.cpp",
    "fallen/Source/Enemy.cpp",
    "fallen/Source/Effect.cpp",
    "fallen/Source/Cop.cpp",
    "fallen/Source/Building.cpp",
    "fallen/Source/Anim.cpp",
}

AUDIT_INFINITY_FILES = {
    "fallen/Source/Prim.cpp",
    "fallen/Source/ob.cpp",
    "fallen/DDEngine/Source/NGamut.cpp",
    "fallen/DDEngine/Source/aeng.cpp",
    "fallen/Source/pcom.cpp",
    "fallen/outro/imp.cpp",
    "fallen/Source/hm.cpp",
    "fallen/Source/collide.cpp",
    "fallen/DDEngine/Source/smap.cpp",
    "fallen/DDEngine/Source/Crinkle.cpp",
    "fallen/Source/dirt.cpp",
    "fallen/Source/Combat.cpp",
    "fallen/Source/ware.cpp",
    "fallen/Source/barrel.cpp",
    "fallen/DDEngine/Source/mesh.cpp",
    "fallen/DDEngine/Source/figure.cpp",
    "fallen/DDEngine/Source/farfacet.cpp",
    "fallen/DDEngine/Source/aa.cpp",
    "fallen/Source/wand.cpp",
    "fallen/Source/pap.cpp",
    "fallen/Source/eway.cpp",
    "fallen/DDEngine/Source/ic.cpp",
    "fallen/DDEngine/Source/facet.cpp",
    "fallen/Source/stair.cpp",
    "fallen/Source/plat.cpp",
    "fallen/Source/Person.cpp",
    "fallen/DDEngine/Source/engineMap.cpp",
    "fallen/DDEngine/Source/comp.cpp",
    "fallen/Source/supermap.cpp",
    "fallen/Source/road.cpp",
    "fallen/Source/mav.cpp",
    "fallen/Source/fc.cpp",
    "fallen/Source/elev.cpp",
    "fallen/Source/door.cpp",
    "fallen/Source/bat.cpp",
    "fallen/Source/Controls.cpp",
    "fallen/DDEngine/Source/panel.cpp",
    "fallen/DDEngine/Source/font2d.cpp",
    "fallen/DDEngine/Source/Message.cpp",
}

# All files that need any replacement
ALL_AUDIT_FILES = AUDIT_TRUE_FILES | AUDIT_FALSE_EXTRA | AUDIT_INFINITY_FILES

# Build per-file macro sets for precise replacement
def _header_to_source_variants(path):
    """Map a header path to possible .cpp source paths in the audit.

    E.g. 'fallen/DDEngine/Headers/poly.h' -> ['fallen/DDEngine/Source/poly.cpp']
         'fallen/Headers/Game.h' -> ['fallen/Source/Game.cpp']
         'MFStdLib/Headers/MFStdLib.h' -> ['MFStdLib/Source/StdLib/StdMem.cpp', ...]
    """
    import os
    variants = []
    if path.endswith('.h'):
        base = os.path.splitext(os.path.basename(path))[0]
        # Try replacing Headers/ with Source/
        if '/Headers/' in path:
            src_dir = path.replace('/Headers/', '/Source/')
            src_dir = os.path.dirname(src_dir)
            variants.append(src_dir + '/' + base + '.cpp')
    return variants


def get_macros_for_file(orig_path):
    """Return which macros to replace for a given original file path."""
    macros = set()

    # Direct lookup
    paths_to_check = [orig_path]

    # If this is a header, also check corresponding source files
    if orig_path.endswith('.h'):
        paths_to_check.extend(_header_to_source_variants(orig_path))

    # Build case-insensitive lookup sets
    def _ci_match(path, audit_set):
        path_lower = path.lower()
        return any(a.lower() == path_lower for a in audit_set)

    for p in paths_to_check:
        if _ci_match(p, AUDIT_TRUE_FILES):
            macros.add('TRUE')
            macros.add('FALSE')  # TRUE list is subset of FALSE list
        if _ci_match(p, AUDIT_FALSE_EXTRA):
            macros.add('FALSE')
        if _ci_match(p, AUDIT_INFINITY_FILES):
            macros.add('INFINITY')

    return macros


def replace_in_line(line, macros):
    """Replace macros in a single line based on allowed set."""
    for pattern, replacement in REPLACEMENTS:
        name = replacement.replace('UC_', '')
        if name in macros:
            line = pattern.sub(replacement, line)
    return line


def process_file_old(filepath, orig_path):
    """Process an old/ file: replace all occurrences based on audit."""
    macros = get_macros_for_file(orig_path)
    if not macros:
        return 0

    with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
        lines = f.readlines()

    count = 0
    new_lines = []
    for line in lines:
        new_line = replace_in_line(line, macros)
        if new_line != line:
            count += 1
        new_lines.append(new_line)

    if count > 0:
        with open(filepath, 'w', encoding='utf-8', newline='') as f:
            f.writelines(new_lines)

    return count


def process_file_new(filepath):
    """Process a new/ file: replace per-entity based on uc_orig comments."""
    with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
        lines = f.readlines()

    # Parse uc_orig comments to find entity boundaries and their source files
    # Each entity spans from its uc_orig comment to the next uc_orig or EOF
    uc_orig_re = re.compile(r'//\s*uc_orig:\s*\S+\s*\(([^)]+)\)')

    # Build a map: line_index -> set of macros to replace
    # Default: no replacement until we see a uc_orig
    current_macros = set()
    line_macros = []

    for i, line in enumerate(lines):
        m = uc_orig_re.search(line)
        if m:
            orig_file = m.group(1).strip()
            current_macros = get_macros_for_file(orig_file)
        line_macros.append(current_macros.copy())

    count = 0
    new_lines = []
    for i, line in enumerate(lines):
        macros = line_macros[i]
        new_line = replace_in_line(line, macros)
        if new_line != line:
            count += 1
        new_lines.append(new_line)

    if count > 0:
        with open(filepath, 'w', encoding='utf-8', newline='') as f:
            f.writelines(new_lines)

    return count


def main():
    total_old = 0
    total_new = 0
    old_files_changed = 0
    new_files_changed = 0

    # Step 2: old/ files
    print("=== Step 2: old/ files ===")
    old_base = os.path.join(BASE, "old")
    for orig_path in sorted(ALL_AUDIT_FILES):
        # Map audit path to old/ path
        filepath = os.path.join(old_base, orig_path)
        if not os.path.exists(filepath):
            print(f"  SKIP (not found): {filepath}")
            continue
        count = process_file_old(filepath, orig_path)
        if count > 0:
            old_files_changed += 1
            total_old += count
            print(f"  {count:4d} replacements: old/{orig_path}")

    print(f"\nold/ total: {total_old} replacements in {old_files_changed} files\n")

    # Step 3: new/ files
    print("=== Step 3: new/ files (per-entity) ===")
    new_base = os.path.join(BASE, "new")
    for root, dirs, files in os.walk(new_base):
        for fname in sorted(files):
            if not (fname.endswith('.cpp') or fname.endswith('.h')):
                continue
            filepath = os.path.join(root, fname)
            # Skip the definition headers themselves
            relpath = os.path.relpath(filepath, new_base).replace('\\', '/')
            if relpath in ('core/types.h', 'core/macros.h'):
                continue
            count = process_file_new(filepath)
            if count > 0:
                new_files_changed += 1
                total_new += count
                print(f"  {count:4d} replacements: new/{relpath}")

    print(f"\nnew/ total: {total_new} replacements in {new_files_changed} files\n")
    print(f"GRAND TOTAL: {total_old + total_new} replacements in {old_files_changed + new_files_changed} files")


if __name__ == '__main__':
    main()
