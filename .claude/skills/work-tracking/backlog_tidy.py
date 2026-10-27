"""Bring both backlogs in line with the task system rules (skill `work-tracking`, section "Backlog").

There are two backlogs: main (`backlog.txt`) and background (`backlog-background.txt`) — the rules are common, the
script tidies both.

What it does with each:
- a task line is `number area icons description`, one space between the parts; icons are the task type circles in
  the order 🔵 analysis, 🟢 feature, 🔴 bug, with no spaces between them;
- a line without an area (number followed directly by icons or text) gets the area from the task registry, a line
  without icons gets the icons from the type in the registry;
- icons in the wrong order are reordered;
- tasks of ONE area in a row — no blank lines between them, between DIFFERENT areas — exactly 4 blank lines
  (however many there were: fewer — added, more — trimmed); no blank lines at the start of the file;
- the order of lines is NEVER changed: the order is the priority, the human sets it.

What it checks and reports (the files are still tidied, except in --check mode):
- ERROR: a line doesn't start with a number; the area is not in `areas.md`; the area or icons in the backlog differ
  from the registry; a number repeats (including across both backlogs); type circles occur in the line text; a task
  in the active registry has no type;
- WARNING: a task in the backlog is closed or is not in the active registry;
- INFO: active tasks that are in neither backlog (except plans).

Plans (type `plan`) are not put into backlogs — their list is in `plans.md`: the script checks that every open plan is
there, that `plans.md` contains only open plans, and that there is exactly one "Current" line pointing to an open
plan (ERROR — a plan in a backlog, an open plan missing from `plans.md`, a non-open plan in `plans.md`, "Current"
missing, duplicated or not open).

Run: `python backlog_tidy.py [--tracker FOLDER] [--check]`. The tracker folder defaults to `tracker/` at the
repository root (searched upward from the current folder). `--check` — only check: writes nothing, exit code 1 if a
file would have to change or there are errors.
"""
import argparse
import os
import re
import sys

# How many blank lines between tasks of different areas.
GAP = 4
# Tracker backlogs: main and background.
BACKLOGS = ('backlog.txt', 'backlog-background.txt')
# Task types and their icons — in the order the icons are written.
TYPES = (('analysis', '🔵'), ('feature', '🟢'), ('bug', '🔴'))
# Type "plan" — not work but the order of work: it's not in the backlogs, the list of open ones is `plans.md`.
PLAN = 'plan'
ICONS = ''.join(icon for _, icon in TYPES)


def find_tracker(start):
    d = os.path.abspath(start)
    while True:
        cand = os.path.join(d, 'tracker')
        if os.path.isfile(os.path.join(cand, 'backlog.txt')):
            return cand
        parent = os.path.dirname(d)
        if parent == d:
            return None
        d = parent


def read_text(path):
    raw = open(path, 'rb').read().decode('utf-8')
    return raw, ('\r\n' in raw), raw.replace('\r\n', '\n')


def known_areas(tracker):
    _, _, s = read_text(os.path.join(tracker, 'areas.md'))
    return set(re.findall(r'^- `([^`\s]+)`', s, re.M))


def icons_of(types):
    return ''.join(icon for name, icon in TYPES if name in types)


def read_index(path, plans=None):
    """Task number → (area, type icons) from a tracker area registry (`| number | area | … | tags | …`); plan numbers
    (type `plan`) go into `plans`."""
    if not os.path.isfile(path):
        return {}
    _, _, s = read_text(path)
    out = {}
    for line in s.split('\n'):
        m = re.match(r'\| \[?(\d+)\]?(?:\([^)]*\))? \| ([^|\s]+) \| [^|]*\| ([^|]*)\|', line)
        if m:
            tags = {t.strip() for t in m.group(3).split(',')}
            num = int(m.group(1))
            if PLAN in tags:
                if plans is not None:
                    plans.add(num)
                continue
            out[num] = (m.group(2), icons_of(tags))
    return out


def check_plans(tracker, plans, errors):
    """`plans.md` — every open plan is there, and only open plans (numbers — from `#N` links)."""
    path = os.path.join(tracker, 'plans.md')
    if not os.path.isfile(path):
        if plans:
            errors.append('plans.md is missing, but there are open plans: ' + ', '.join(map(str, sorted(plans))))
        return
    _, _, s = read_text(path)
    listed = {int(n) for n in re.findall(r'^\s*-\s*\[#(\d+)\]', s, re.M)}
    for n in sorted(plans - listed):
        errors.append(f'plans.md: open plan {n} is not in the list')
    for n in sorted(listed - plans):
        errors.append(f'plans.md: {n} is not an open plan (closed, not a plan, or not in the registry)')
    current = re.findall(r'\*\*Current:\*\*\s*\[#(\d+)\]', s)
    if plans and len(current) != 1:
        errors.append('plans.md: the "**Current:** [#N]…" line must occur exactly once')
    for n in current:
        if int(n) not in plans:
            errors.append(f'plans.md: "Current" is {n}, which is not an open plan')


def tidy(path, areas, active, closed, plans, seen, errors, warnings, fixed):
    """Tidy one backlog. Returns (before, after, CRLF flag)."""
    name = os.path.basename(path)
    if not os.path.isfile(path):
        return '', '', False
    raw, crlf, s = read_text(path)
    entries = []  # (area or None, line)
    for no, line in enumerate(s.split('\n'), 1):
        if not line.strip():
            continue
        where = f'{name}:{no}'
        m = re.match(r'\s*(\d+)\s+(.*)$', line)
        if not m:
            errors.append(f'{where}: does not start with a task number: {line.strip()[:80]}')
            entries.append((None, line.rstrip()))
            continue
        num, rest = int(m.group(1)), m.group(2).strip()
        if num in plans:
            errors.append(f'{where}: {num} is a plan; plans are not put into backlogs (their list is plans.md)')
        if num in seen:
            errors.append(f'{where}: task {num} occurs in the backlogs a second time (already in {seen[num]})')
        seen[num] = name
        reg_area, reg_icons = active.get(num, (None, ''))

        # area
        token, _, tail = rest.partition(' ')
        if token in areas:
            area, rest = token, tail.strip()
        elif reg_area:
            area = reg_area
            fixed.append(f'{where}: task {num} — area {area} set from the registry')
        else:
            errors.append(f'{where}: task {num}: "{token}" is not an area from areas.md, and the task is not in the active registry')
            area = None

        # type icons
        token, _, tail = rest.partition(' ')
        if token and all(ch in ICONS for ch in token):
            icons, rest = icons_of({n for n, i in TYPES if i in token}), tail.strip()
            if icons != token:
                fixed.append(f'{where}: task {num} — icons reordered to {icons}')
        elif reg_icons:
            icons = reg_icons
            fixed.append(f'{where}: task {num} — icons {icons} set from the registry')
        else:
            icons = ''
            errors.append(f'{where}: task {num} — no type icons, and the type is not found in the registry')
        if any(ch in rest for ch in ICONS):
            errors.append(f'{where}: task {num} — type circles in the line text (they go only right after the area)')

        if num in active:
            if area and area != reg_area:
                errors.append(f'{where}: task {num} — area {area} in the backlog, but {reg_area} in the registry')
            if icons and reg_icons and icons != reg_icons:
                errors.append(f'{where}: task {num} — icons {icons} in the backlog, but {reg_icons} per the registry')
        elif num in closed:
            warnings.append(f'{where}: task {num} is closed but still in the backlog')
        elif num not in plans:
            warnings.append(f'{where}: task {num} is not in the active registry')
        parts = [str(num)] + [x for x in (area, icons, rest) if x]
        entries.append((area, ' '.join(parts)))

    out = []
    prev = object()
    for area, line in entries:
        if out and area != prev:
            out.extend([''] * GAP)
        out.append(line)
        prev = area
    result = '\n'.join(out) + '\n' if out else ''
    if crlf:
        result = result.replace('\n', '\r\n')
    return raw, result, crlf


def main():
    sys.stdout.reconfigure(encoding='utf-8')
    ap = argparse.ArgumentParser(description='Bring both backlogs in line with the task system rules.')
    ap.add_argument('--tracker', help='tracker folder (default: tracker/ at the repository root)')
    ap.add_argument('--check', action='store_true', help='only check, write nothing')
    args = ap.parse_args()

    tracker = args.tracker or find_tracker(os.getcwd())
    if not tracker:
        print('ERROR: tracker/backlog.txt not found upward from the current folder — pass --tracker')
        return 2
    areas = known_areas(tracker)
    plans = set()
    active = read_index(os.path.join(tracker, 'active', 'tasks', '_index.md'), plans)
    closed = read_index(os.path.join(tracker, 'closed', 'tasks', '_index.md'), set())

    errors, warnings, fixed = [], [], []
    check_plans(tracker, plans, errors)
    for num, (_, icons) in sorted(active.items()):
        if not icons:
            errors.append(f'active registry: task {num} has no type (analysis / feature / bug in the tags)')

    seen = {}
    results = []
    for name in BACKLOGS:
        path = os.path.join(tracker, name)
        raw, result, _ = tidy(path, areas, active, closed, plans, seen, errors, warnings, fixed)
        results.append((path, raw, result))

    for f in fixed:
        print('FIXED:', f)
    for w in warnings:
        print('WARNING:', w)
    for e in errors:
        print('ERROR:', e)
    absent = sorted(n for n in active if n not in seen)
    if absent:
        print('INFO: active tasks in neither backlog:', ', '.join(map(str, absent)))

    changed = [path for path, raw, result in results if raw != result]
    if args.check:
        print('backlogs are tidy' if not changed and not errors else 'backlogs need tidying')
        return 1 if changed or errors else 0
    for path, raw, result in results:
        if raw != result:
            open(path, 'wb').write(result.encode('utf-8'))
    print('tidied: ' + ', '.join(os.path.basename(p) for p in changed) if changed else 'backlogs already tidy')
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
