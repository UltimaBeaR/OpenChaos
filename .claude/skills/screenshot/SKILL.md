---
name: screenshot
description: >
  Take a screenshot of the game window (Fallen.exe). Use when the user asks
  to see the game screen, or when you need to see what's currently rendered.
  Never take screenshots proactively — only when the user explicitly asks or
  confirms. Trigger when: user says "скрин", "screenshot", "покажи что на экране",
  "сделай скрин", "что сейчас видно", "скринь", "сфоткай", "покажи экран",
  or when you suggest taking a screenshot and the user agrees.
---

# Screenshot — capture the game window

## When to use

- User **explicitly** says "сделай скрин", "screenshot", "скринь" — just do it
- You **think** a screenshot would help but user didn't ask — ask first: "Сделать скриншот?"
- **Never take screenshots without permission**

## Capture command

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

**If PowerShell complains about duplicate type `ScrCap`** — change `ScrCap` to `ScrCap2`, `ScrCap3`, etc. (types persist per PowerShell session).

## After capturing

1. Read the screenshot: `Read` tool on `c:\WORK\OpenChaos\_CLAUDE_SCREEN.png`
2. Show the user a clickable link: `[screenshot](_CLAUDE_SCREEN.png)`
3. Describe what you see
4. File is overwritten each time, gitignored

## Technical details

- `SetProcessDPIAware()` — required for correct coordinates on scaled displays
- `DwmGetWindowAttribute(hwnd, 9)` — real window bounds without shadow
- `IsIconic` + `ShowWindow(9)` — restores if minimized
- `CopyFromScreen` — works with OpenGL (unlike PrintWindow)
