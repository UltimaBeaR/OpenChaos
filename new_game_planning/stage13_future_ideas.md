# Global ideas (part of Stage 13)

Ideas that could radically change the project. They require serious study and a decision on "is this even needed". Most likely will not be implemented.

---

## Skeletal character animations

**Problem:** The original system animates characters as a set of separate "blobs" (one per body part). Gaps are visible at the joints between limbs — especially noticeable between the head and the neck. The artifact is fully reproduced in the original game too, it's just even more visible at high resolution.

**Idea:** Move character animations to a skeletal system with skin weights.

**Workflow concept:**

1. **Resource preparation (once on first launch, or when the originals change):**
   - Original resources (`original_game/fallen/Debug/`) are not touched
   - `openchaos/` — a folder next to the exe for all precomputed derived OpenChaos resources (not just skeletons — any future pre-baked data goes here too)
   - On game launch: if there's no cached data in `openchaos/skeletal/` for a character, or the original's timestamp has changed — run the processing
   - Per-character processing: original model → T-pose → skeleton → skin weights → save the entire rig in `openchaos/skeletal/<character_name>/` (skeleton, weights, T-pose mesh)
   - Subsequent launches: load the ready data from `openchaos/` without recomputing

2. **Programmatic automatic rigging:**
   - The T-pose is reconstructed from the set of body-part pieces by their relative positions in the neutral frame
   - The skeleton is generated automatically from the body-part hierarchy (already present in the original as `body_part_parent_numbers`)
   - Skin weights are computed by the proximity of a vertex to a "joint" (Dual Quaternion Skinning or simple linear blend)
   - Seams between body parts are closed with a blend zone

3. **Runtime:**
   - In-game, instead of drawing each blob separately — we transform a single skinned mesh through bone matrices
   - Bone matrices are computed from the same original animation data as now (the format doesn't change)

**Expected result:** gaps at the limb joints disappear, no more "holes in the neck", characters look like a single body rather than a set of parts.

**Difficulties:**
- Programmatic automatic rigging is a non-trivial task; the quality of blend weights without a manual artist will be a compromise
- We need to decide how to handle body parts with their own textures (each blob is currently drawn with its own texture — merging into one draw call will require a texture atlas or multi-texture)
- Compatibility with the current animation system (figure.cpp / `ge_draw_multi_matrix`) — will most likely require replacing the character rendering path

---

## Full remake from scratch on a new engine

Once the modernized version of the game is fully ready and stable — we can consider a full
rewrite: a different engine, new models, new physics, new code.

The modernized OpenChaos codebase is used as a **reference** — it is significantly
shorter, cleaner and more understandable than the original, which simplifies analyzing the logic when writing new code from scratch.

---

## Migrating the project to Zig

Zig is fully compatible with C (importing C headers, linking with C libraries), but not with C++.
A check of the codebase showed that the project is essentially **C code compiled as C++**:

- 27 classes in the project, ~10 of them in the D3D backend (will go away when D3D is removed)
- The classes are primitive: structs with methods, no virtual methods, no polymorphism
- The only inheritance: `PolyPoint2D : private GEVertexTL` (trivial)
- One template: `DoMerge<T>` in polypage.cpp
- No STL, no exceptions (`catch`/`throw`), no RTTI (`dynamic_cast`)
- `new`/`delete` are almost never used — `MemAlloc`/`MemFree` are used instead
- 263 occurrences of C++ patterns across 98 files — but most of them are `new`/`delete` in D3D code

Migrating to Zig is realistic and can be **gradual**:

1. **Wrapper and entry point** — Zig build system (replaces CMake), main entry point in Zig,
   the C++ part (classes → struct + functions) is rewritten first since Zig doesn't import C++
2. **Including C code directly** — Zig can `@cImport` C headers, all existing C code
   (and the project is essentially C) keeps working unchanged, links with Zig
3. **Iterative migration** — gradually rewrite C files in Zig, one module at a time,
   the rest keep compiling as C through the Zig build system

The main work in the first step is to remove C++ (classes, the single template, `inline` in .cpp).
After that all code is pure C, and Zig can import it directly.

All of the project's external libraries have a pure C API, no problems with Zig:
- **OpenGL** (opengl32) — Zig has built-in support, bindings exist
- **GLAD** (vendored, gl.c) — pure C, `@cImport` directly
- **SDL3** — C API, Zig bindings exist, or `@cImport`
- **OpenAL** — C API, `@cImport` directly

**Risks:** Claude knows Zig well (syntax, build system, @cImport, comptime, allocators,
error handling), but not as deeply as C/C++ — less experience with edge cases, idiomatic
patterns and debugging non-trivial problems. Zig is not yet 1.0 (as of 2026 — 0.13+),
the API can break between versions. The community and ecosystem are significantly smaller than C/C++'s.
Before starting the migration it's advisable to do test tasks in Zig (hello world, @cImport a C
library, SDL+OpenGL window) — to gauge Claude's real level of proficiency with this language
and surface problems before they block the main project.

---

## Porting the renderer to Vulkan (try after 1.0)

**Status:** an idea to work out after a stable 1.0 release. First —
diagnostics and optimization of the render architecture on the current OpenGL (see the
"precondition" below). Vulkan — only if profiling confirms that the bottleneck
is really there.

### What Vulkan does NOT give

Pure GPU rasterization is **not sped up**: the same GPU, the same shaders, the same
pixels. This game's geometry is 1999-era; for any modern /
integrated GPU and for the Steam Deck the scene itself is pennies in terms of GPU load.
If the bottleneck is overdraw/fill — Vulkan won't help.

### What Vulkan actually gives

The win is all on the **CPU/driver** side, and that's exactly our pain profile:

- Drastically less overhead on draw calls and state changes. The game's legacy pipeline
  (poly pages, many small batches, per-object state) generates a lot of this.
  Hypothesis: the bulk of draw calls is rendering 3D models through
  poly pages; moving them to proper (persistent where possible)
  vertex buffers is the main lever, and it helps on **any** API.
- Command buffers can be assembled across multiple threads.
- Full control over memory/synchronization.
- **Steam Deck / Linux:** Vulkan is a first-class native API (the RADV driver
  is excellent). OpenGL there is a second-class citizen (via the Zink/Mesa
  GL layer, works worse). On the main target platform Vulkan is the architecturally
  correct path, not "a bit better".

### Why this is expensive (unlike D3D6 → OpenGL)

D3D6 and OpenGL are the **same paradigm**: immediate-style, implicit
synchronization, memory managed by the driver. D3D6→GL was largely a
mechanical 1:1 mapping, our `ge_*` abstraction was designed for that
immediate model.

Vulkan is a **completely different paradigm**: everything is explicit and on us (allocation and
sub-allocation of GPU memory, residency, barriers/semaphores/fences, prebuilt
command buffers, pipeline state objects, descriptor sets, manual
swapchain). The driver no longer does the heavy lifting for us. This is not
"rewrite the calls", but rethinking the model; the `ge_*` abstraction would have to be
reworked for the explicit model. Hence "months", not "porting
the calls".

### Target architecture (GPU-driven), where it makes sense to head

Few shaders + large shared vertex-index buffers + submitting a batch of
objects with **one CPU command** (multi-draw-indirect / instancing).
Grouping by pipeline state into ~3 buckets: **opaque / transparent /
UI**. Important nuances so as not to fool ourselves:

- **The "god-shader" is not the main lever.** Draw calls are fragmented mostly
  because of different **textures**, not shaders. The real enabler is **bindless**
  (all textures resident, the shader indexes by `textureId`, no
  per-draw bind). The god-shader only solves the shader part and isn't itself
  free on the GPU (it bloats, lower occupancy, can be slower than
  specialized ones) — important on weak hardware/the Deck. In practice —
  a small set + an ubershader with specialization constants, not "one
  shader for everything".
- **"In one call" = one indirect/instanced submit**, not one
  triangle list of everything. The GPU still draws each object; the win is
  the CPU/driver overhead on submit, the GPU fill is the same.
- **Transparency requires back-to-front sorting** → within a single
  megabatch you can't freely reorder. Opaque you can (z-buffer),
  transparent — no. This limits "everything in one call" for the transparent
  bucket.
- **The geometry is heterogeneous:** the static world (batches perfectly) vs
  skinned characters (per-frame vertex updates) vs
  sprites/particles/sky/shadows. The current poly pages also sort/group
  by state — this logic must be reproduced in the new batching system,
  otherwise you'll accidentally spawn a bunch of state changes.
- **Most of the CPU win can be had on OpenGL too** (texture
  arrays/atlas, instancing, persistent buffers,
  `glMultiDrawElementsIndirect` in GL 4.3+). The lever is the **render
  architecture**, not the mere fact of Vulkan. Vulkan only makes the explicit/bindless path
  clean, and it's native on the Deck.

### Bindless and platforms

This game's textures are pennies — residency is not a problem, they'll all fit in VRAM.
Mechanism:

- **Vulkan:** one large descriptor array with all textures (the
  `descriptorIndexing` feature), bound once, into the shader — `textureId`. Clean,
  with no size/format limits. Available everywhere on modern hardware and
  on the Deck.
- **OpenGL:** either `ARB_bindless_texture` (not on all drivers), or a
  portable fallback — texture arrays/atlas + layer index (requires
  the same size/format per layer → fiddling with atlases).
- **macOS on native GL — NO bindless** (Apple is stuck on OpenGL 4.1).
  This is a limitation of Apple's GL, **not the hardware**: Metal itself can do
  argument buffers (≈bindless).

### MoltenVK solves the macOS problem

MoltenVK is a Vulkan→Metal layer (there is no native Vulkan on macOS, only
Metal). It helps **only in the Vulkan world** (has nothing to do with GL). In
the Vulkan world — it solves it: MoltenVK maps Vulkan descriptor indexing onto Metal
argument buffers → **bindless works on macOS**, no atlas hack for the sake of Mac
is needed. Strategically this is inevitable anyway: Apple has deprecated
OpenGL, long-term the only path on Mac is Metal/MoltenVK. Bottom line: one
Vulkan backend = Windows/Linux/Deck natively + macOS via MoltenVK, bindless
everywhere. On OpenGL, Mac stays crippled forever and will hit deprecation.

**MoltenVK caveats:** not 100% Vulkan (a subset, some
extensions/features don't map onto Metal), translation overhead, one more
dependency to support/test.

### Precondition (mandatory before making the decision)

Don't dive into Vulkan on a hypothesis. First — diagnostics on the current OpenGL:
**draw-call and state-change counters per subsystem** (scene / 3D models /
HUD / debug overlays). Confirm that CPU draw-call/driver overhead is the
**dominant** cost on the target hardware (Steam Deck, integrated
graphics), and not our guess. Then — squeeze out what we can on OpenGL (moving
3D models from poly pages to persistent VBOs, fewer state changes,
texture arrays/instancing). Vulkan — only if after this the CPU bottleneck
remains general and significant across the whole game. Moving 3D models to proper
vertex buffers is a lever useful in itself and a precondition for Vulkan
to give any profit at all.
