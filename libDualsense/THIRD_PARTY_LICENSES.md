# Third-party licenses

libDualsense was developed using the open-source projects listed below as
reference implementations for the DualSense HID protocol. Any of them may
have been consulted at any point, and individual fragments (lookup tables,
packing formulas, byte layouts) may have been adapted from them into this
library. Their original copyright notices are reproduced here in full, as
required by their respective licenses.

---

## DualSense-Windows (DS5W)

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

## duaLib

**Upstream:** <https://github.com/WujekFoliarz/duaLib>

The file `src/source/triggerFactory.cpp` inside the duaLib repository carries
its own file-level copyright notice authored by John "Nielk1" Klein, who is
also the author of the widely-referenced DualSense trigger effect gist at
<https://gist.github.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db>. That
file-level copyright takes precedence for the trigger packing logic
specifically.

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

The same copyright notice applies to the standalone Nielk1 DualSense
trigger-effects gist at
<https://gist.github.com/Nielk1/6d54cc2c00d2201ccb8c2720ad7538db>, which
contains the identical MIT notice at the top of the file.

Repository-level copyright for duaLib as a whole:

```
MIT License

Copyright (c) 2025 WujekFoliarz

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

## daidr/dualsense-tester

**Upstream:** <https://github.com/daidr/dualsense-tester>

```
MIT License

Copyright (c) 2023 Xuezhou Dai

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

## rafaelvaloto/Dualsense-Multiplatform

**Upstream:** <https://github.com/rafaelvaloto/Dualsense-Multiplatform>

```
MIT License

Copyright (c) 2025 Rafael Valoto

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

## Paliverse/DualSenseX

**Upstream:** <https://github.com/Paliverse/DualSenseX>

No LICENSE file present in the upstream repository at the time of consultation.
Only the C# UDP example project is public; the main DSX application is closed-
source. Listed here for credit and traceability.

---

## nondebug/dualsense

**Upstream:** <https://github.com/nondebug/dualsense>

No LICENSE file present in the upstream repository at the time of consultation.
Listed here for credit and traceability.

---

## dogtopus DualSense HID descriptor gist

**Upstream:** <https://gist.github.com/dogtopus/894da226d73afb3bdd195df41b3a26aa>

No license notice declared on the gist at the time of consultation. Content
is an HID report descriptor (data, not code). Listed here for credit and
traceability.

---

## Game Controller Collective Wiki — Sony DualSense

**Upstream:** <https://controllers.fandom.com/wiki/Sony_DualSense>

Community-maintained wiki hosted on Fandom. Content is documentation, not
code; no executable code is adapted from this source. Listed here for credit.

---

## Linux kernel `hid-playstation` driver

**Upstream:** <https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/hid/hid-playstation.c>

Referenced **only for cross-verification** of the DualSense IMU
calibration formula (raw int16 gyro/accel samples → deg/s / g). The
formula itself is a well-known mathematical relation (not subject to
copyright) that multiple independent DualSense reverse-engineering
projects arrived at, and our `ds_calibration.cpp` implementation is
written from scratch. Listed here for credit / traceability only; no
source code is adapted from the kernel driver.

(Linux kernel source files are GPL-2 licensed. If a future contributor
ever does adapt actual source code from `hid-playstation.c`, the
GPL-2 implications for the library licensing would need to be
re-evaluated.)
