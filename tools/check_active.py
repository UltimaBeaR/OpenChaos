"""Check for active code outside #if 0 blocks in old/ .cpp files."""
import os

def has_active_code(filepath):
    try:
        content = open(filepath, 'r', errors='replace').read()
    except Exception:
        return False, []

    depth_if0 = 0
    result = []
    for line in content.split('\n'):
        stripped = line.strip()
        if stripped.startswith('#if 0'):
            depth_if0 += 1
            continue
        if depth_if0 > 0:
            if stripped.startswith('#endif'):
                depth_if0 -= 1
            continue
        # Line is outside any #if 0 block
        if not stripped:
            continue
        if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
            continue
        if stripped in ('{', '}', '};', '};', '//'):
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
