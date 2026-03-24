#!/usr/bin/env python3
"""
Stage 4.2 — File dependency graph generator and query tool.

Parses #include directives in new_game/src/ to build a DAG of file dependencies.
Stores result in new_game_devlog/st_4_2_dep_graph.json.

Usage:
    python tools/st_4_2_dep_graph.py generate          # Parse includes, write JSON
    python tools/st_4_2_dep_graph.py deps <file>        # Direct dependencies of file
    python tools/st_4_2_dep_graph.py rdeps <file>       # Who depends on file (reverse)
    python tools/st_4_2_dep_graph.py cycles             # Find circular dependencies
    python tools/st_4_2_dep_graph.py layers             # Show files grouped by layer (folder)
    python tools/st_4_2_dep_graph.py cross-layer        # Show cross-layer dependencies
    python tools/st_4_2_dep_graph.py orphans            # Files with no deps and no rdeps
    python tools/st_4_2_dep_graph.py stats              # Summary statistics
    python tools/st_4_2_dep_graph.py update <old> <new> # Rename a file in the graph

File paths are relative to src/ (e.g., "core/types.h", "engine/graphics/pipeline/aeng.cpp").
"""

import json
import os
import re
import sys
from collections import defaultdict
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = REPO_ROOT / "new_game" / "src"
JSON_PATH = REPO_ROOT / "new_game_devlog" / "st_4_2_dep_graph.json"

# Include search roots relative to SRC_DIR
INCLUDE_ROOTS = [
    SRC_DIR,              # resolves "core/types.h", "engine/...", etc.
    SRC_DIR / "platform", # resolves <MFStdLib.h>, StdFile, etc.
]

# System headers to ignore (angle-bracket includes that aren't project headers)
SYSTEM_PREFIXES = {
    "windows", "windowsx", "d3d", "dd", "di", "ds", "mss",
    "stdio", "stdlib", "string", "stdarg", "math", "time",
    "cstdlib", "cstring", "cmath", "cassert", "cstdio", "cstdarg",
    "ctype", "assert", "io", "fcntl", "sys", "errno", "signal",
    "mmsystem", "objbase", "initguid", "tchar", "commctrl",
    "wingdi", "commdlg", "shlobj", "direct",
    "windef", "mmstream", "amstream", "mbctype", "mbstring",
    "stddef", "al/", "sdl2/", "process",
}

INCLUDE_RE = re.compile(r'^\s*#\s*include\s+[<"]([^>"]+)[>"]', re.MULTILINE)


def is_system_header(name: str) -> bool:
    """Check if an include looks like a system/external header."""
    base = name.lower().replace(".h", "").replace(".hpp", "")
    # Direct match
    if base in SYSTEM_PREFIXES:
        return True
    # Prefix match (e.g., d3dtypes.h -> d3d prefix)
    for prefix in SYSTEM_PREFIXES:
        if base.startswith(prefix):
            return True
    return False


def resolve_include(include_name: str, from_file: Path) -> str | None:
    """
    Resolve an include to a path relative to src/.
    Returns None if it's a system header or can't be found.
    """
    if is_system_header(include_name):
        return None

    # Try each include root
    for root in INCLUDE_ROOTS:
        candidate = root / include_name
        if candidate.is_file():
            try:
                return str(candidate.relative_to(SRC_DIR)).replace("\\", "/")
            except ValueError:
                pass

    # Try relative to the file's own directory
    candidate = from_file.parent / include_name
    if candidate.is_file():
        try:
            return str(candidate.relative_to(SRC_DIR)).replace("\\", "/")
        except ValueError:
            pass

    return None


def generate():
    """Parse all source files and build the dependency graph."""
    graph = {}
    unresolved = defaultdict(list)

    for ext in ("*.h", "*.cpp", "*.c"):
        for filepath in SRC_DIR.rglob(ext):
            rel = str(filepath.relative_to(SRC_DIR)).replace("\\", "/")
            try:
                content = filepath.read_text(encoding="utf-8", errors="replace")
            except Exception:
                continue

            deps = []
            for match in INCLUDE_RE.finditer(content):
                inc = match.group(1)
                resolved = resolve_include(inc, filepath)
                if resolved:
                    if resolved != rel:  # skip self-includes
                        deps.append(resolved)
                elif not is_system_header(inc):
                    unresolved[rel].append(inc)

            # Deduplicate while preserving order
            seen = set()
            unique_deps = []
            for d in deps:
                if d not in seen:
                    seen.add(d)
                    unique_deps.append(d)

            graph[rel] = sorted(unique_deps)

    # Sort by key for stable output
    graph = dict(sorted(graph.items()))

    data = {
        "_meta": {
            "description": "Stage 4.2 file dependency graph (src/ relative paths)",
            "generated_by": "tools/st_4_2_dep_graph.py",
            "file_count": len(graph),
            "edge_count": sum(len(v) for v in graph.values()),
        },
        "graph": graph,
    }

    if unresolved:
        data["unresolved"] = dict(sorted(unresolved.items()))

    JSON_PATH.write_text(json.dumps(data, indent=2, ensure_ascii=False), encoding="utf-8")
    meta = data["_meta"]
    print(f"Generated {JSON_PATH.name}: {meta['file_count']} files, {meta['edge_count']} edges")
    if unresolved:
        total_unresolved = sum(len(v) for v in unresolved.values())
        print(f"  {total_unresolved} unresolved includes in {len(unresolved)} files")
        for f, incs in sorted(unresolved.items())[:10]:
            for inc in incs:
                print(f"    {f} -> {inc}")
        if len(unresolved) > 10:
            print(f"    ... and {len(unresolved) - 10} more files")


def load_graph() -> dict:
    if not JSON_PATH.exists():
        print(f"Error: {JSON_PATH} not found. Run 'generate' first.", file=sys.stderr)
        sys.exit(1)
    data = json.loads(JSON_PATH.read_text(encoding="utf-8"))
    return data["graph"]


def build_reverse(graph: dict) -> dict[str, list[str]]:
    rev = defaultdict(list)
    for f, deps in graph.items():
        for d in deps:
            rev[d].append(f)
    return {k: sorted(v) for k, v in sorted(rev.items())}


def normalize_path(p: str) -> str:
    """Accept partial paths and try to match to a file in the graph."""
    return p.replace("\\", "/")


def find_file(graph: dict, query: str) -> str | None:
    """Find a file in the graph by exact or partial match."""
    query = normalize_path(query)
    if query in graph:
        return query
    # Try suffix match
    matches = [f for f in graph if f.endswith(query) or f.endswith("/" + query)]
    if len(matches) == 1:
        return matches[0]
    if len(matches) > 1:
        print(f"Ambiguous match for '{query}':", file=sys.stderr)
        for m in matches:
            print(f"  {m}", file=sys.stderr)
        return None
    # Not in graph keys — might be a header only referenced as dependency
    all_files = set(graph.keys())
    for deps in graph.values():
        all_files.update(deps)
    matches = [f for f in all_files if f.endswith(query) or f.endswith("/" + query)]
    if len(matches) == 1:
        return matches[0]
    if len(matches) > 1:
        print(f"Ambiguous match for '{query}':", file=sys.stderr)
        for m in matches:
            print(f"  {m}", file=sys.stderr)
    else:
        print(f"No match for '{query}'", file=sys.stderr)
    return None


def cmd_deps(args):
    graph = load_graph()
    target = find_file(graph, args[0])
    if not target:
        return
    deps = graph.get(target, [])
    print(f"{target} depends on ({len(deps)}):")
    for d in deps:
        print(f"  {d}")


def cmd_rdeps(args):
    graph = load_graph()
    target = find_file(graph, args[0])
    if not target:
        return
    rev = build_reverse(graph)
    rdeps = rev.get(target, [])
    print(f"{target} is depended on by ({len(rdeps)}):")
    for r in rdeps:
        print(f"  {r}")


def cmd_cycles(_args=None):
    graph = load_graph()
    # DFS-based cycle detection
    WHITE, GRAY, BLACK = 0, 1, 2
    color = {f: WHITE for f in graph}
    cycles = []

    def dfs(node, path):
        color[node] = GRAY
        for dep in graph.get(node, []):
            if dep not in color:
                continue
            if color[dep] == GRAY:
                cycle_start = path.index(dep)
                cycles.append(path[cycle_start:] + [dep])
            elif color[dep] == WHITE:
                dfs(dep, path + [dep])
        color[node] = BLACK

    for f in graph:
        if color[f] == WHITE:
            dfs(f, [f])

    if cycles:
        print(f"Found {len(cycles)} cycle(s):")
        for i, cycle in enumerate(cycles, 1):
            print(f"\n  Cycle {i}:")
            for f in cycle:
                print(f"    {f}")
    else:
        print("No cycles found.")


def get_layer(filepath: str) -> str:
    """Extract top-level layer from path."""
    parts = filepath.split("/")
    return parts[0] if len(parts) > 1 else "(root)"


def cmd_layers(_args=None):
    graph = load_graph()
    layers = defaultdict(list)
    for f in graph:
        layers[get_layer(f)].append(f)
    for layer in sorted(layers):
        files = layers[layer]
        print(f"\n{layer}/ ({len(files)} files)")
        for f in sorted(files):
            deps = graph.get(f, [])
            ext_deps = [d for d in deps if get_layer(d) != layer]
            if ext_deps:
                print(f"  {f}  ->  {', '.join(get_layer(d) for d in ext_deps)}")
            else:
                print(f"  {f}")


def cmd_cross_layer(_args=None):
    graph = load_graph()
    cross = defaultdict(lambda: defaultdict(list))
    for f, deps in graph.items():
        src_layer = get_layer(f)
        for d in deps:
            dst_layer = get_layer(d)
            if src_layer != dst_layer:
                cross[src_layer][dst_layer].append((f, d))

    print("Cross-layer dependencies (source -> target: count):\n")
    for src in sorted(cross):
        for dst in sorted(cross[src]):
            edges = cross[src][dst]
            print(f"  {src}/ -> {dst}/  ({len(edges)} edges)")
            # Show first few
            for f, d in edges[:3]:
                print(f"    {f} -> {d}")
            if len(edges) > 3:
                print(f"    ... +{len(edges) - 3} more")


def cmd_orphans(_args=None):
    graph = load_graph()
    rev = build_reverse(graph)
    all_files = set(graph.keys())

    orphans = []
    for f in sorted(all_files):
        deps = graph.get(f, [])
        rdeps = rev.get(f, [])
        if not deps and not rdeps:
            orphans.append(f)

    if orphans:
        print(f"Orphan files (no deps, no rdeps): {len(orphans)}")
        for f in orphans:
            print(f"  {f}")
    else:
        print("No orphan files.")


def cmd_stats(_args=None):
    graph = load_graph()
    rev = build_reverse(graph)
    all_files = set(graph.keys())
    for deps in graph.values():
        all_files.update(deps)

    h_files = [f for f in graph if f.endswith(".h")]
    cpp_files = [f for f in graph if f.endswith(".cpp") or f.endswith(".c")]

    edge_count = sum(len(v) for v in graph.values())

    # Most depended-on
    top_rdeps = sorted(rev.items(), key=lambda x: len(x[1]), reverse=True)[:15]

    # Most dependencies
    top_deps = sorted(graph.items(), key=lambda x: len(x[1]), reverse=True)[:15]

    # Layer stats
    layers = defaultdict(int)
    for f in graph:
        layers[get_layer(f)] += 1

    print(f"Files in graph: {len(graph)}")
    print(f"  .h:   {len(h_files)}")
    print(f"  .cpp: {len(cpp_files)}")
    print(f"Total edges: {edge_count}")
    print(f"Avg deps per file: {edge_count / len(graph):.1f}")

    print(f"\nFiles by layer:")
    for layer, count in sorted(layers.items(), key=lambda x: -x[1]):
        print(f"  {layer}/: {count}")

    print(f"\nMost depended-on (top 15):")
    for f, rdeps in top_rdeps:
        print(f"  {len(rdeps):3d} <- {f}")

    print(f"\nMost dependencies (top 15):")
    for f, deps in top_deps:
        print(f"  {len(deps):3d} -> {f}")


def cmd_update(args):
    """Rename a file in the graph (for tracking moves)."""
    if len(args) < 2:
        print("Usage: update <old_path> <new_path>", file=sys.stderr)
        sys.exit(1)

    old_path = normalize_path(args[0])
    new_path = normalize_path(args[1])

    data = json.loads(JSON_PATH.read_text(encoding="utf-8"))
    graph = data["graph"]

    # Rename key
    if old_path in graph:
        graph[new_path] = graph.pop(old_path)
    else:
        print(f"Warning: {old_path} not found as a key in graph", file=sys.stderr)

    # Rename in all dependency lists
    count = 0
    for f in graph:
        if old_path in graph[f]:
            graph[f] = [new_path if d == old_path else d for d in graph[f]]
            count += 1

    # Re-sort
    data["graph"] = dict(sorted(graph.items()))
    JSON_PATH.write_text(json.dumps(data, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"Renamed {old_path} -> {new_path} (updated {count} dependency references)")


COMMANDS = {
    "generate": lambda _: generate(),
    "deps": cmd_deps,
    "rdeps": cmd_rdeps,
    "cycles": cmd_cycles,
    "layers": cmd_layers,
    "cross-layer": cmd_cross_layer,
    "orphans": cmd_orphans,
    "stats": cmd_stats,
    "update": cmd_update,
}


def main():
    if len(sys.argv) < 2 or sys.argv[1] not in COMMANDS:
        print(__doc__)
        print("Commands:", ", ".join(COMMANDS.keys()))
        sys.exit(1)

    cmd = sys.argv[1]
    args = sys.argv[2:]
    COMMANDS[cmd](args)


if __name__ == "__main__":
    main()
