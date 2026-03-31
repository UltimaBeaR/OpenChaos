---
name: screenshot
description: >
  Take a screenshot of the game window (Fallen.exe). Use when the user asks
  to see the game screen, or when you need to see what's rendered.
  NEVER take screenshots proactively — only when user explicitly asks or confirms.
  TRIGGER: user says "скрин", "screenshot", "покажи что на экране", "сделай скрин",
  "что сейчас видно".
---

# Screenshot — capture the game window

## When to use
- If user **explicitly** says "сделай скрин", "screenshot", "скринь" — just do it, don't ask
- If you **think** a screenshot would help but user didn't explicitly ask — ask first: "Сделать скриншот?"
- **NEVER take screenshots proactively without permission**

## How to capture

Run this PowerShell command (note: class name must be unique per session —
if PowerShell complains about duplicate type, change `ScrCap` to `ScrCap2` etc.):

```bash
powershell -c "
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public class ScrCap {
    [DllImport(\"user32.dll\")] public static extern bool SetProcessDPIAware();
    [DllImport(\"user32.dll\")] public static extern bool IsIconic(IntPtr hWnd);
    [DllImport(\"user32.dll\")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport(\"user32.dll\")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport(\"dwmapi.dll\")] public static extern int DwmGetWindowAttribute(IntPtr hwnd, int attr, out RECT rect, int size);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
}
'@
[ScrCap]::SetProcessDPIAware() | Out-Null
\$proc = Get-Process Fallen -ErrorAction SilentlyContinue
if (-not \$proc) { Write-Host 'Fallen not running'; exit 1 }
\$hwnd = \$proc.MainWindowHandle
if ([ScrCap]::IsIconic(\$hwnd)) { [ScrCap]::ShowWindow(\$hwnd, 9) | Out-Null }
[ScrCap]::SetForegroundWindow(\$hwnd) | Out-Null
Start-Sleep -Milliseconds 300
\$rect = New-Object ScrCap+RECT
[ScrCap]::DwmGetWindowAttribute(\$hwnd, 9, [ref]\$rect, [Runtime.InteropServices.Marshal]::SizeOf(\$rect)) | Out-Null
\$w = \$rect.Right - \$rect.Left; \$h = \$rect.Bottom - \$rect.Top
\$bmp = New-Object System.Drawing.Bitmap(\$w, \$h)
\$g = [System.Drawing.Graphics]::FromImage(\$bmp)
\$g.CopyFromScreen(\$rect.Left, \$rect.Top, 0, 0, (New-Object System.Drawing.Size(\$w, \$h)))
\$g.Dispose()
\$stream = [System.IO.File]::Create('C:\WORK\OpenChaos\_CLAUDE_SCREEN.png')
\$bmp.Save(\$stream, [System.Drawing.Imaging.ImageFormat]::Png)
\$stream.Close(); \$bmp.Dispose()
Write-Host 'OK'
" 2>&1
```

## After capturing

1. Read the screenshot: `Read` tool on `c:\WORK\OpenChaos\_CLAUDE_SCREEN.png`
2. Show the user a clickable link: `[screenshot](_CLAUDE_SCREEN.png)`
3. Describe what you see
4. File is overwritten each time (no history), gitignored

## Key details

- `SetProcessDPIAware()` — required, otherwise window coordinates are wrong on scaled displays
- `DwmGetWindowAttribute(hwnd, 9=DWMWA_EXTENDED_FRAME_BOUNDS)` — gives real window bounds without shadow
- `IsIconic` + `ShowWindow(hwnd, 9=SW_RESTORE)` — restores if minimized
- `SetForegroundWindow` + sleep — brings window to front before capture
- `CopyFromScreen` — captures the screen area (works with OpenGL unlike PrintWindow)
