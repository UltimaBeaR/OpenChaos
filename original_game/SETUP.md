# Setup Guide — Original Game

Building the original Urban Chaos source code (`original_game/fallen/`).

This is the **read-only reference build** — used to verify original game behavior.
Do not modify files in `original_game/`.

---

## Prerequisites

### Visual Studio 2026 Community

Download from [visualstudio.microsoft.com](https://visualstudio.microsoft.com/).

During installation, select the **Desktop development with C++** workload —
this includes MSVC, Windows SDK, vcpkg, and other essentials automatically.

### Urban Chaos (legal copy)

Steam version recommended: [store.steampowered.com/app/243060/Urban_Chaos/](https://store.steampowered.com/app/243060/Urban_Chaos/)

---

## First-time setup

### Step 1 — Copy game resources

Create the folder `original_game/fallen/Debug/` if it doesn't exist,
then copy everything from your Urban Chaos installation **except `.exe` and `.dll` files** into it.

Steam default path:
```
C:\Program Files (x86)\Steam\steamapps\common\Urban Chaos\
```

After this, `original_game/fallen/Debug/` should contain `clumps/`, `levels/`, `Textures/`, etc.

### Step 2 — Set up vcpkg integration

The project uses vcpkg (bundled with VS) for SDL2 and OpenAL.
Run once from **Developer Command Prompt for VS 2026**:

```
"C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg\vcpkg.exe" integrate install
```

This makes Visual Studio automatically find and link vcpkg packages when building.

### Step 3 — Build

Open `original_game/fallen/Fallen.sln` in Visual Studio 2026
and build the **Debug | x86** configuration.

Output: `original_game/fallen/Debug/Fallen.exe`

### Step 4 — Run

Run `original_game/fallen/Debug/Fallen.exe` directly, or launch from inside Visual Studio.
The working directory must be `original_game/fallen/Debug/` (Visual Studio sets this automatically).

---

## Notes

- Only **Debug Win32** is regularly tested. The Release configuration exists but may need adjustments.
- `original_game/fallen/Debug/` and `Release/` are gitignored — game resources are not committed.
