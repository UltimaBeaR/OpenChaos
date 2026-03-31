---
name: debugger-install
description: >
  Install cdb.exe (Windows SDK Debugging Tools) for live process debugging.
  Use when the debugger skill reports cdb.exe is not found.
---

# Install cdb.exe (Windows Debugging Tools)

## When to use
When `debugger` skill can't find cdb.exe at:
`/c/Program Files (x86)/Windows Kits/10/Debuggers/x64/cdb.exe`

## What works (tested 2026-03-31)

The Windows SDK is typically already installed (for building), but the
"Debugging Tools" feature is NOT included by default. Must be added separately.

### Step 1: Download the SDK installer
```bash
curl -L -o /tmp/winsdksetup.exe "https://go.microsoft.com/fwlink/?linkid=2272610"
```

### Step 2: Install only the debugging tools feature
```bash
powershell -c "Start-Process -FilePath '$(cygpath -w /tmp/winsdksetup.exe)' -ArgumentList '/features OptionId.WindowsDesktopDebuggers /quiet /norestart' -Wait"
```

### Step 3: Verify
```bash
ls "/c/Program Files (x86)/Windows Kits/10/Debuggers/x64/cdb.exe" && echo "SUCCESS"
```

## What does NOT work

- `winget install Microsoft.WinDbg` — installs the GUI Store app (WinDbg Preview),
  which does NOT include the CLI `cdb.exe`. Useless for automation.
- `winget install Microsoft.WindowsSDK --override "/features OptionId.WindowsDesktopDebuggers"` —
  the `--override` flag is ignored by winget for this package.
- `lldb -p <PID>` — LLVM's lldb is installed but crashes with
  `error: unable to find 'python311.dll'` (system has Python 3.13, lldb wants 3.11).
  Not usable without matching Python version.

## Result
After installation, cdb.exe is at:
`C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe`
