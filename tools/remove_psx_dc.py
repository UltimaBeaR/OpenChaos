#!/usr/bin/env python3
"""
Remove PSX and TARGET_DC dead code from new_game/fallen source files.

Rules:
  #ifdef PSX / TARGET_DC / BUILD_PSX / VERSION_PSX
      → remove the if-branch entirely, keep #else branch (if any)
  #ifndef PSX / TARGET_DC / BUILD_PSX
      → keep the if-branch (remove the guard line), remove #else branch
  #if defined(PSX) || defined(TARGET_DC)  and variants
      → remove if-branch, keep #else branch
  #if !defined(TARGET_DC) / !defined(PSX)
      → keep if-branch (remove guard), remove #else branch
  #elif inside a PSX/DC block
      → convert to #if (preserves the condition for the next branch)

IMPORTANT - block comment handling:
  If a removed PSX/DC block contains an unmatched /* (i.e. the */ closing it
  appears AFTER the #endif), the script continues suppressing output until
  the comment is closed. This handles constructs like:
      #ifdef PSX
      /*
      ... PSX-only stuff ...
      #endif
      */        ← this */ must also be suppressed
"""

import re
import os
import sys
from pathlib import Path

BASE_DIR = Path(r"c:\WORK\OpenChaos\new_game\fallen")
PROCESS_EXTS = {'.cpp', '.h', '.c', '.hpp'}

# Directories to skip (editor folders stay until later, build artifacts, etc.)
SKIP_DIRS = {
    'Editor', 'GEdit', 'Ledit', 'SeEdit', 'LeEdit', 'sedit',
    'Debug', 'Release', 'vcpkg_installed', 'dxinstall',
    'AutoRun', 'SoundConverter', 'Loader',
}

# These flags are never defined on PC → #ifdef FLAG means "remove this branch"
REMOVED_FLAGS = re.compile(
    r'^PSX$|^TARGET_DC$|^BUILD_PSX$|^VERSION_PSX$',
    re.IGNORECASE
)

# These flags are never defined on PC → #ifndef FLAG / #if !defined means "keep this branch"
KEPT_NOT_FLAGS = re.compile(
    r'^PSX$|^TARGET_DC$|^BUILD_PSX$',
    re.IGNORECASE
)


def count_comment_delta(line):
    """
    Return the net change in block-comment nesting for this line.
    Counts /* as +1 and */ as -1, ignoring those inside line comments or strings.
    This is a best-effort approximation (handles 99% of real code).
    """
    # Strip line comments first (// ...)
    # (We don't handle strings here — edge case not present in this codebase)
    without_line_comment = re.sub(r'//.*$', '', line)
    opens = without_line_comment.count('/*')
    closes = without_line_comment.count('*/')
    return opens - closes


def parse_condition(stripped):
    """
    Analyze a preprocessor condition line.
    Returns:
      'remove'  - this is a PSX/DC positive condition (remove if-branch, keep else)
      'keep'    - this is a PSX/DC negative condition (keep if-branch, remove else)
      'regular' - not PSX/DC related
      'complex' - partially PSX/DC related (needs manual review)
    """
    m_ifdef = re.match(r'^#\s*ifdef\s+(\w+)', stripped)
    m_ifndef = re.match(r'^#\s*ifndef\s+(\w+)', stripped)
    m_if = re.match(r'^#\s*if\b(.*)', stripped)

    if m_ifdef:
        flag = m_ifdef.group(1)
        if REMOVED_FLAGS.match(flag):
            return 'remove'
        return 'regular'

    if m_ifndef:
        flag = m_ifndef.group(1)
        if KEPT_NOT_FLAGS.match(flag):
            return 'keep'
        return 'regular'

    if m_if:
        expr = m_if.group(1).strip()
        expr = re.sub(r'//.*$', '', expr).strip()

        # Bare flag name used as integer: #if PSX (evaluates to 0 on PC)
        bare_flag = re.fullmatch(r'(\w+)', expr)
        if bare_flag and REMOVED_FLAGS.match(bare_flag.group(1)):
            return 'remove'

        # Pure PSX/DC positive conditions
        pure_remove = re.fullmatch(
            r'defined\s*\(\s*(?:PSX|TARGET_DC|BUILD_PSX|VERSION_PSX)\s*\)'
            r'(?:\s*\|\|\s*defined\s*\(\s*(?:PSX|TARGET_DC|BUILD_PSX|VERSION_PSX)\s*\))*',
            expr, re.IGNORECASE
        )
        if pure_remove:
            return 'remove'

        # Pure PSX/DC negative conditions
        pure_keep = re.fullmatch(
            r'!\s*defined\s*\(\s*(?:PSX|TARGET_DC|BUILD_PSX)\s*\)'
            r'(?:\s*&&\s*!\s*defined\s*\(\s*(?:PSX|TARGET_DC|BUILD_PSX)\s*\))*',
            expr, re.IGNORECASE
        )
        if pure_keep:
            return 'keep'

        # Mixed: contains PSX/DC but also other flags → needs manual review
        has_psx_dc = bool(re.search(r'\b(PSX|TARGET_DC|BUILD_PSX|VERSION_PSX)\b', expr, re.IGNORECASE))
        if has_psx_dc:
            return 'complex'

    return 'regular'


def process_file(path):
    """
    Process a single file.
    Returns (new_lines, changed, complex_issues)
    """
    try:
        with open(path, 'r', encoding='latin-1', newline='') as f:
            lines = f.readlines()
    except Exception as e:
        return None, False, [f"READ ERROR: {e}"]

    result = []
    complex_issues = []

    # Stack frames: [is_psx_dc, output, inverted]
    #   is_psx_dc: this frame was created for a PSX/DC directive
    #   output:    True = currently outputting lines
    #   inverted:  True = the original condition was negative (ifndef/!defined)
    stack = []

    # Tracks unmatched /* that were opened while output was suppressed.
    # If > 0 after popping a suppressed block, we continue suppressing until closed.
    open_comments_in_suppressed = 0

    def should_output():
        if open_comments_in_suppressed > 0:
            return False
        return all(f[1] for f in stack) if stack else True

    for lineno, line in enumerate(lines, 1):
        stripped = line.strip()

        # If we're suppressing due to an unclosed block comment from removed code,
        # keep draining until the comment closes.
        if open_comments_in_suppressed > 0:
            open_comments_in_suppressed += count_comment_delta(line)
            if open_comments_in_suppressed < 0:
                open_comments_in_suppressed = 0
            # Suppress this line regardless
            continue

        was_suppressing = not should_output()

        # --- Preprocessor directive? ---
        is_directive = re.match(r'^#\s*(ifdef|ifndef|if|else|elif|endif)\b', stripped)

        if re.match(r'^#\s*(ifdef|ifndef|if)\b', stripped):
            kind = parse_condition(stripped)

            if kind == 'remove':
                stack.append([True, False, False])
                continue  # don't emit

            elif kind == 'keep':
                stack.append([True, True, True])
                continue  # don't emit (removes the guard)

            elif kind == 'complex':
                complex_issues.append(
                    f"  Line {lineno}: complex PSX/DC condition: {stripped}"
                )
                stack.append([False, should_output(), False])
                if should_output():
                    result.append(line)
                continue

            else:  # regular
                stack.append([False, True, False])
                if should_output():
                    result.append(line)
                continue

        elif re.match(r'^#\s*else\b', stripped):
            if stack and stack[-1][0]:  # top is PSX/DC frame
                # Before toggling, if we were suppressing and have open comments, carry them
                if not stack[-1][1]:
                    # We were suppressing the if-branch; check for unclosed comments
                    pass  # open_comments_in_suppressed already tracked below
                stack[-1][1] = not stack[-1][1]
                continue  # don't emit
            else:
                if should_output():
                    result.append(line)
                continue

        elif re.match(r'^#\s*elif\b', stripped):
            if stack and stack[-1][0]:  # top is PSX/DC frame
                # PSX/DC branch ended; convert #elif to #if for the next branch
                stack.pop()
                new_line = re.sub(r'^(\s*)#\s*elif\b', r'\1#if', line, count=1)
                if should_output():
                    result.append(new_line)
                stack.append([False, True, False])
                continue
            else:
                if should_output():
                    result.append(line)
                continue

        elif re.match(r'^#\s*endif\b', stripped):
            if stack and stack[-1][0]:  # top is PSX/DC frame
                stack.pop()
                continue  # don't emit
            else:
                if stack:
                    stack.pop()
                if should_output():
                    result.append(line)
                continue

        else:
            # Regular (non-directive) line
            if should_output():
                result.append(line)
            else:
                # We're suppressing — track block comments opened while suppressed
                open_comments_in_suppressed += count_comment_delta(line)
                if open_comments_in_suppressed < 0:
                    open_comments_in_suppressed = 0
            continue

    new_content = ''.join(result)
    original_content = ''.join(lines)
    changed = new_content != original_content

    return result, changed, complex_issues


def main():
    dry_run = '--dry-run' in sys.argv

    if dry_run:
        print("=== DRY RUN (no files will be modified) ===\n")
    else:
        print("=== LIVE RUN (files will be modified) ===\n")

    files_to_process = []
    for root, dirs, files in os.walk(BASE_DIR):
        root_path = Path(root)
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
        for fname in files:
            if Path(fname).suffix.lower() in PROCESS_EXTS:
                files_to_process.append(root_path / fname)

    changed_files = []
    complex_files = []
    unchanged_count = 0

    for fpath in sorted(files_to_process):
        result_lines, changed, issues = process_file(fpath)

        if issues:
            complex_files.append((fpath, issues))

        if changed:
            changed_files.append(fpath)
            rel = fpath.relative_to(BASE_DIR)
            print(f"  CHANGED: {rel}")
            if not dry_run and result_lines is not None:
                content = ''.join(result_lines)
                with open(fpath, 'w', encoding='latin-1', newline='') as f:
                    f.write(content)
        else:
            unchanged_count += 1

    print(f"\nTotal changed: {len(changed_files)}")
    print(f"Total unchanged: {unchanged_count}")

    if complex_files:
        print(f"\n=== COMPLEX CASES (manual review needed): {len(complex_files)} files ===")
        for fpath, issues in complex_files:
            print(f"\n{fpath.relative_to(BASE_DIR)}:")
            for issue in issues:
                print(issue)
    else:
        print("\nNo complex cases found.")


if __name__ == '__main__':
    main()
