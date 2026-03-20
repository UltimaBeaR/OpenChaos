#!/usr/bin/env python3
"""
Entity mapping tool for OpenChaos Этап 4+.
Tracks which original entities (functions, types, variables, files) map to which new files.

RECORD TYPES (--kind):
  file       — File-level move: a file in old/ that came from a different path in original_game/.
               Used for Этап 2 records (editor headers copied to main headers, etc.).
               'file' field = path in old/, 'orig_file' = path in original_game/.
  function   — Function moved from old/ to new/.
  struct     — Struct moved from old/ to new/.
  class      — Class moved from old/ to new/.
  variable   — Global variable moved from old/ to new/.
  type       — Typedef or type alias moved from old/ to new/.
  macro      — #define macro moved from old/ to new/.

STAGES:
  2 — File moves from Этап 2 (editor headers → old/fallen/Headers/).
  4 — Entity moves from Этап 4 (old/ → new/). Default.

ORIG_FILE RULES:
  Always a path relative to original_game/ root — see stage4_rules.md for details.

WORKFLOW for Этап 4 migration:
  Before adding any entity, ALWAYS run find first to check:
    python tools/entity_map.py find --name ENTITY_NAME

  Case 1: No results → the entity hasn't been recorded yet.
    Use the file's path in original_game/ as --orig-file (e.g. fallen/Source/Thing.cpp).

  Case 2: find returns a 'file' record (stage 2) for the source file:
    This means the file in old/ came from a different path in original_game/.
    Use THAT record's orig_file as your --orig-file for entity records.
    Example: old/fallen/Headers/anim.h came from fallen/Editor/Headers/Anim.h (stage 2 record).
    So entities migrated from anim.h use --orig-file "fallen/Editor/Headers/Anim.h".

  Case 3: find returns an entity record (stage 4) — entity already migrated, skip.

Usage:
  python tools/entity_map.py add    --name NAME --file FILE --orig-name ORIG --orig-file ORIG_FILE --kind KIND [--conflict] [--stage N]
  python tools/entity_map.py find   --name NAME        # поиск по name и orig_name (substring)
  python tools/entity_map.py list   [--kind KIND] [--stage N]
  python tools/entity_map.py stats
  python tools/entity_map.py rename --name OLD --new-name NEW
"""

import argparse
import json
import sys
from datetime import date
from pathlib import Path

MAPPING_FILE = Path(__file__).parent.parent / "new_game_planning" / "entity_mapping.json"


def load() -> dict:
    if not MAPPING_FILE.exists():
        return {"entries": []}
    with open(MAPPING_FILE, encoding="utf-8") as f:
        return json.load(f)


def save(data: dict) -> None:
    with open(MAPPING_FILE, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")


def cmd_add(args) -> None:
    data = load()
    entries = data["entries"]

    # Check for duplicates
    for e in entries:
        if e["name"] == args.name and e["file"] == args.file:
            print(f"ERROR: entry already exists: {args.name} in {args.file}", file=sys.stderr)
            sys.exit(1)

    entry = {
        "name": args.name,
        "file": args.file,
        "orig_name": args.orig_name,
        "orig_file": args.orig_file,
        "kind": args.kind,
        "conflict": args.conflict,
        "stage": args.stage,
        "date": str(date.today()),
    }
    entries.append(entry)
    save(data)
    print(f"Added: {args.name} ({args.kind}, stage {args.stage}) -> {args.file}")


def fmt_entry(e: dict) -> str:
    conflict_flag = " [CONFLICT]" if e.get("conflict") else ""
    stage_flag = f" [stage {e.get('stage', 4)}]" if e.get("stage", 4) != 4 else ""
    return f"{e['name']}{conflict_flag}{stage_flag}  ({e['kind']})"


def cmd_find(args) -> None:
    data = load()
    needle = args.name.lower()
    results = [
        e for e in data["entries"]
        if needle in e["name"].lower() or needle in e["orig_name"].lower()
    ]
    if not results:
        print(f"No entries found for: {args.name}")
        return
    for e in results:
        print(fmt_entry(e))
        print(f"  file:      {e['file']}")
        print(f"  orig_name: {e['orig_name']}")
        print(f"  orig_file: {e['orig_file']}")
        print(f"  date:      {e['date']}")
        print()


def cmd_list(args) -> None:
    data = load()
    entries = data["entries"]
    if args.kind:
        entries = [e for e in entries if e["kind"] == args.kind]
    if args.stage is not None:
        entries = [e for e in entries if e.get("stage", 4) == args.stage]
    if not entries:
        print("No entries.")
        return
    for e in entries:
        print(f"{fmt_entry(e)}  {e['file']}  <-  {e['orig_file']}")


def cmd_stats(args) -> None:
    data = load()
    entries = data["entries"]
    total = len(entries)
    by_kind: dict[str, int] = {}
    by_stage: dict[int, int] = {}
    for e in entries:
        k = e["kind"]
        by_kind[k] = by_kind.get(k, 0) + 1
        s = e.get("stage", 4)
        by_stage[s] = by_stage.get(s, 0) + 1
    conflicts = sum(1 for e in entries if e.get("conflict"))

    print(f"Total entries: {total}")
    print("By kind:")
    for kind, count in sorted(by_kind.items()):
        print(f"  {kind}: {count}")
    print("By stage:")
    for stage, count in sorted(by_stage.items()):
        print(f"  stage {stage}: {count}")
    if conflicts:
        print(f"Conflicts: {conflicts}")


def cmd_rename(args) -> None:
    data = load()
    found = False
    for e in data["entries"]:
        if e["name"] == args.name:
            e["name"] = args.new_name
            found = True
    if not found:
        print(f"ERROR: no entry with name: {args.name}", file=sys.stderr)
        sys.exit(1)
    save(data)
    print(f"Renamed: {args.name} -> {args.new_name}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Entity mapping tool for OpenChaos")
    sub = parser.add_subparsers(dest="command", required=True)

    # add
    p_add = sub.add_parser("add", help="Add a new entity mapping entry")
    p_add.add_argument("--name",      required=True, help="Current name (in new/ for stage 4, in old/ for stage 2)")
    p_add.add_argument("--file",      required=True, help="Current file path (new/ for stage 4, old/ for stage 2)")
    p_add.add_argument("--orig-name", required=True, dest="orig_name", help="Original name in original_game/")
    p_add.add_argument("--orig-file", required=True, dest="orig_file", help="Original file path (fallen/...)")
    p_add.add_argument("--kind",      required=True,
                       choices=["function", "class", "struct", "variable", "type", "macro", "file"],
                       help="Entity kind. Use 'file' for file-level moves.")
    p_add.add_argument("--conflict",  action="store_true", default=False,
                       help="Set if name was changed due to conflict")
    p_add.add_argument("--stage",     type=int, default=4,
                       help="Development stage this record belongs to (default: 4)")

    # find
    p_find = sub.add_parser("find", help="Find entries by name (substring match on name and orig_name)")
    p_find.add_argument("--name", required=True)

    # list
    p_list = sub.add_parser("list", help="List all entries")
    p_list.add_argument("--kind",  help="Filter by kind")
    p_list.add_argument("--stage", type=int, help="Filter by stage")

    # stats
    sub.add_parser("stats", help="Show statistics")

    # rename
    p_rename = sub.add_parser("rename", help="Rename an entity (updates 'name' field)")
    p_rename.add_argument("--name",     required=True, dest="name")
    p_rename.add_argument("--new-name", required=True, dest="new_name")

    args = parser.parse_args()
    {
        "add":    cmd_add,
        "find":   cmd_find,
        "list":   cmd_list,
        "stats":  cmd_stats,
        "rename": cmd_rename,
    }[args.command](args)


if __name__ == "__main__":
    main()
