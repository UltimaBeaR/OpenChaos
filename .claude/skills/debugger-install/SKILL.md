---
name: debugger-install
description: >
  Install cdb.exe (Windows SDK Debugging Tools) for live process debugging.
  Use when the debugger skill reports cdb.exe is not found, or when you need
  cdb.exe and it's not installed. This is a one-time setup — once installed,
  cdb.exe persists across sessions.
---

# Install cdb.exe (Windows Debugging Tools)

## When to use

When `debugger` skill can't find cdb.exe at:
`/c/Program Files (x86)/Windows Kits/10/Debuggers/x64/cdb.exe`

## Installation steps

### 1. Download the SDK installer
```bash
curl -L -o /tmp/winsdksetup.exe "https://go.microsoft.com/fwlink/?linkid=2272610"
```

### 2. Install only the debugging tools feature
```bash
powershell -c "Start-Process -FilePath '$(cygpath -w /tmp/winsdksetup.exe)' -ArgumentList '/features OptionId.WindowsDesktopDebuggers /quiet /norestart' -Wait"
```

### 3. Verify
```bash
ls "/c/Program Files (x86)/Windows Kits/10/Debuggers/x64/cdb.exe" && echo "SUCCESS"
```

## What does NOT work (tested 2026-03-31)

- `winget install Microsoft.WinDbg` — installs the GUI Store app, NOT the CLI `cdb.exe`
- `winget install Microsoft.WindowsSDK --override "/features OptionId.WindowsDesktopDebuggers"` — `--override` is ignored
- `lldb -p <PID>` — crashes with `error: unable to find 'python311.dll'` (system has Python 3.13)

## Result

After installation: `C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe`
