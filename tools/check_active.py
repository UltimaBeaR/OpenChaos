"""Check for active code outside #if 0 blocks in old/ .cpp files."""
import os


def has_active_code(filepath):
    try:
        content = open(filepath, 'r', errors='replace').read()
    except Exception:
        return False, []

    result = []
    # Track nesting properly: a stack of whether each #if* level is a #if 0
    # depth_stack: list of bools, True = this level is #if 0
    depth_stack = []
    in_block_comment = False

    for line in content.split('\n'):
        stripped = line.strip()

        # Handle /* */ block comments (across lines)
        if in_block_comment:
            if '*/' in stripped:
                in_block_comment = False
            continue
        if '/*' in stripped and '*/' not in stripped[stripped.index('/*'):]:
            # Opening /* without closing */ on same line
            in_block_comment = True
            # Still process the part before /*
            stripped = stripped[:stripped.index('/*')].strip()
            if not stripped:
                continue

        if stripped.startswith('#if 0'):
            depth_stack.append(True)  # This is a #if 0 level
            continue
        if stripped.startswith('#if ') or stripped.startswith('#ifdef') or stripped.startswith('#ifndef'):
            depth_stack.append(False)  # This is a #if X (not #if 0) level
            continue
        if stripped.startswith('#else') or stripped.startswith('#elif'):
            # Flip the top-of-stack: if we were in #if 0, now we're in the else branch (active)
            if depth_stack and depth_stack[-1]:
                depth_stack[-1] = False
            continue
        if stripped == '#endif' or stripped.startswith('#endif ') or stripped.startswith('#endif//') or stripped.startswith('#endif/*') or stripped.startswith('#endif /'):
            if depth_stack:
                depth_stack.pop()
            continue

        # Line is outside any #if 0 block if none of the stack entries are True
        if any(depth_stack):
            continue

        if not stripped:
            continue
        if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
            continue
        if stripped in ('{', '}', '};', '//'):
            continue
        if (stripped.startswith('#include') or stripped.startswith('#pragma') or
                stripped.startswith('#endif') or stripped.startswith('#if') or
                stripped.startswith('#define') or stripped.startswith('#undef') or
                stripped.startswith('#else') or stripped.startswith('#elif')):
            continue
        result.append(stripped)

    return len(result) > 0, result[:5]


base = 'new_game/src/old'
found = False
for root, dirs, files in os.walk(base):
    dirs.sort()
    for f in sorted(files):
        if not f.endswith('.cpp'):
            continue
        path = os.path.join(root, f)
        has_code, sample = has_active_code(path)
        if has_code:
            found = True
            rel = path.replace('\\', '/')
            print('ACTIVE: ' + rel)
            for s in sample:
                print('  ' + s)
            print('')

if not found:
    print('No active non-declaration code found outside #if 0 blocks.')
