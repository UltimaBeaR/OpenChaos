# Third-party licenses

This library contains code and lookup tables adapted from the following
open-source projects. All are MIT-licensed and their original copyright
notices are reproduced below in full, as required by the MIT license.

---

## DualSense-Windows (DS5W) — CRC32 table

**Files affected:** `ds_crc.cpp` (CRC32 lookup table and seed constant)

**Upstream:** <https://github.com/Ohjurot/DualSense-Windows>

```
MIT License

Copyright (c) 2020 Ludwig Füchsl

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

## duaLib — trigger effect factories

**Files affected:** `ds_trigger.cpp` (byte-level packing logic for all
adaptive trigger effect modes: Off, Feedback, Weapon, Vibration, Bow, Machine)

**Upstream:** <https://github.com/WujekFoliarz/duaLib>
(file `src/source/triggerFactory.cpp`)

The `triggerFactory.cpp` file inside the duaLib repository carries its own
file-level copyright notice authored by John "Nielk1" Klein, who is also the
author of the widely-referenced DualSense trigger effect gist at
<https://gist.github.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db>. That
file-level copyright takes precedence for the packing logic specifically.

```
MIT License

Copyright (c) 2021-2022 John "Nielk1" Klein

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

Repository-level copyright for duaLib as a whole (for completeness):

```
MIT License

Copyright (c) 2025 WujekFoliarz

[Full MIT terms — identical to the notices above.]
```

---

## Reverse-engineering references (no code copied)

The following projects were consulted as reverse-engineering references for
HID report offsets and bit layouts. No code is copied from them, so no
license attribution is legally required — they are listed here for credit:

- **nondebug/dualsense** — `dualsense-explorer.html`, protocol RE
  <https://github.com/nondebug/dualsense>
- **gcc-wiki Sony DualSense HID reference** — community protocol notes
- **rafaelvaloto/Dualsense-Multiplatform** — structural reference for HID
  report normalization (USB vs BT) during initial development
  <https://github.com/rafaelvaloto/Dualsense-Multiplatform>
