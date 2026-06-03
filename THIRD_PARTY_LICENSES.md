# Third-Party Licenses

This file lists all third-party libraries used by OpenChaos and their licenses.

All third-party libraries are **statically linked** into the single OpenChaos
executable on every platform (no separate DLL/.so/.dylib files are shipped).

> **Maintainer note:** the version numbers below must match the versions vcpkg
> actually builds. Re-check them on every release (see the `release` skill).
> The exact versions live in `new_game/vcpkg_installed/<triplet>/share/<port>/vcpkg.spdx.json`
> (field `versionInfo`; the trailing `#N` is the vcpkg port revision, not the
> library version).

---

## SDL3

- **Website:** https://www.libsdl.org/
- **Version:** 3.4.2
- **License:** zlib
- **Installed via:** vcpkg
- **Linking:** static (built into the executable)

```
Copyright (C) 1997-2025 Sam Lantinga <slouken@libsdl.org>

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
```

---

## OpenAL Soft

- **Website:** https://openal-soft.org/
- **Version:** 1.25.1
- **Source (this exact version):** https://github.com/kcat/openal-soft/releases/tag/1.25.1
- **License:** LGPL-2.0 (GNU Library General Public License, version 2)
- **Installed via:** vcpkg
- **Linking:** static (built into the executable, all platforms)

```
Copyright (C) 1999-2014 by authors.
Authors: Chris Robinson, et al.

OpenAL Soft is licensed under the GNU Library General Public License, Version 2.
Full license text: https://www.gnu.org/licenses/old-licenses/lgpl-2.0.html
Full source code:  https://github.com/kcat/openal-soft
```

OpenAL Soft is statically linked into OpenChaos and is used unmodified. The
LGPL permits static linking provided the user can relink the application with a
modified version of the library. OpenChaos satisfies this because the complete
application source code and build instructions are publicly available, so anyone
can substitute a modified OpenAL Soft and rebuild. The exact library version
used in a given build is linked above.

---

## fmt

- **Website:** https://fmt.dev/
- **Version:** 12.1.0
- **Source:** https://github.com/fmtlib/fmt
- **License:** MIT (with binary embedding exception)
- **Installed via:** vcpkg (transitive dependency of OpenAL Soft)
- **Linking:** static (built into the executable)

```
Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

--- Optional exception to the license ---

As an exception, if, as a result of your compiling your source code, portions
of this Software are embedded into a machine-executable object form of such
source code, you may redistribute such embedded portions in such object form
without including the above copyright and permission notices.
```

---

## FFmpeg

- **Website:** https://ffmpeg.org/
- **Version:** 8.0.1
- **Source (this exact version):** https://ffmpeg.org/releases/ffmpeg-8.0.1.tar.gz (tag `n8.0.1` at https://github.com/FFmpeg/FFmpeg)
- **License:** LGPL-2.1 (no GPL or nonfree components are enabled)
- **Installed via:** vcpkg
- **Linking:** static (built into the executable, all platforms)
- **Components used:** libavcodec, libavformat, libavutil, libswresample, libswscale

```
Copyright (C) 2000-2025 FFmpeg developers.

FFmpeg is licensed under the GNU Lesser General Public License, Version 2.1.
Full license text: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
Full source code:  https://github.com/FFmpeg/FFmpeg
```

FFmpeg is built with only LGPL components (no `--enable-gpl`, no nonfree codecs),
is statically linked into OpenChaos, and is used unmodified. The LGPL permits
static linking provided the user can relink the application with a modified
version of the library. OpenChaos satisfies this because the complete
application source code and build instructions are publicly available, so anyone
can substitute a modified FFmpeg and rebuild. The exact library version used in
a given build is linked above.

---

## nlohmann/json

- **Website:** https://json.nlohmann.me/
- **Version:** 3.12.0
- **Source:** https://github.com/nlohmann/json
- **License:** MIT
- **Installed via:** vcpkg (header-only)

```
MIT License

Copyright (c) 2013-2025 Niels Lohmann

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## stb_image

- **Source:** https://github.com/nothings/stb
- **License:** Public Domain (or MIT, dual-licensed)
- **Vendored at:** `new_game/src/third_party/stb_image.h`
- **Usage:** decodes the embedded input-prompt PNG atlases at startup.

```
stb_image — v2.30 — public domain image loader — http://nothings.org/stb

This software is available under 2 licenses -- choose whichever you prefer:

ALTERNATIVE A - MIT License
Copyright (c) 2017 Sean Barrett
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
```

---

## Kenney Input Prompts

- **Website:** https://kenney.nl/assets/input-prompts
- **License:** CC0 1.0 (public domain dedication)
- **Vendored at:** `new_game/src/ui/input_glyphs/assets/` (four atlas PNGs,
  embedded into the executable at build time).
- **Usage:** on-screen input-prompt glyphs (keyboard & mouse, Xbox,
  DualSense/PlayStation, Steam Deck).

```
Kenney Input Prompts — CC0 1.0 Universal (public domain dedication).
The artwork is released under CC0 by Kenney (https://kenney.nl).

Note: the platform button SYMBOLS depicted (e.g. PlayStation shapes, Xbox
A/B/X/Y, the Steam logo) are trademarks of their respective owners. They are
used here only as functional input prompts to indicate controls, not as a
claim of affiliation or endorsement.
```

---

## font8x8 (font8x8_basic)

- **Source:** https://github.com/dhepper/font8x8
- **License:** Public Domain
- **Embedded at:** `new_game/src/game/missing_resources.cpp` (8x8 glyph table).
- **Usage:** the embedded bitmap font for the "game files not found" startup
  screen, which must draw without any external resources.

```
8x8 monochrome bitmap fonts for rendering
Author: Daniel Hepper <daniel@hepper.net>
License: Public Domain
Based on the public-domain IBM PC OEM VGA fonts (Marcel Sondaar / IBM).
```

---

## libDualsense

DualSense controller support uses an OpenChaos-owned library
(`libDualsense/` at repo root, sibling to `new_game/`). Third-party
attributions for code adapted into libDualsense are in
`libDualsense/THIRD_PARTY_LICENSES.md`.
