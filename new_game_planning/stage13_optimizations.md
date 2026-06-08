# Stage 13 — Renderer optimizations

Performance improvements that **do not block 1.0** — the game already runs on the target
hardware. This collects things that require a serious rework of the pipeline
and/or yield a win mainly on weak hardware (Steam Deck, integrated
graphics, old laptops). Each item is a self-contained task description without
dependency on other documents.

---

## Draw call batching at the PolyPage level

Currently each PolyPage slot makes a separate draw call with its own VBO/EBO/VAO
(per-slot, like the D3D6 original). Batching several draw calls into one large
VBO with a single `glDrawElements` will reduce the number of draw calls from hundreds to
single digits per frame.

**What's needed:**
- A shared VBO with sub-allocation, offset-based indexing.
- Rewrite the `ge_draw_indexed_primitive_*` API to accumulate vertices into a
  shared buffer instead of a per-slot flush.
- Account for state changes (blend, cull, texture address) — you can only batch
  consecutive draw calls with **identical** state. When state changes —
  flush, start a new batch.

**Measurement:** expected win — tens of fps on a weak GPU where the driver overhead
per draw call is high. On a strong GPU there will be almost no difference.

---

## GPU-transform for 3D geometry

Move all three-dimensional rendering (world, objects, effects — **except**
characters and UI) to hardware vertex transforms via shaders, instead of
CPU-transform.

Currently `ge_draw_indexed_primitive_lit` does a CPU-transform (`World ×
Projection`), and the result is drawn as TL (pre-transformed) vertices. This is
a historical approach from the D3D6 era — the vertex shader `tl_vert.glsl`
just passes the vertex through without matrices.

Characters are already on GPU-transform (after Phase 2 skinning — `skin_world_vert.glsl`).
Here we're talking about the rest of the 3D geometry: static meshes (`MESH_draw_poly`),
effects (sprite billboards, particles), etc.

**What's needed:**
- Revive `lit_vert.glsl` (currently doesn't work due to the incompatibility
  of D3D vs GL clip-space conventions) — rework the matrices, viewport transform,
  depth mapping.
- Move `ge_draw_indexed_primitive_lit` calls to the new path.
- **Do NOT touch** UI/HUD — let them stay on the TL path (CPU-transform is
  justified there, the vertex volume is minimal).
- **Do NOT touch** the POLY pipeline as a whole — it's deferred, its trade-off
  is separate.

**Measurement:** offloading the CPU on a scene with lots of static geometry (RTA with
cars, Urban Shakedown). Not critical on a strong CPU, noticeable on the Steam Deck.

**Difficulty:** medium/high — clip-space gotchas will definitely surface,
careful 1:1 verification against the old path is needed.

---

## Texture array / atlas for characters (one draw call per character)

Currently rendering a character's body = **N draw calls** (N = number of body
materials, usually 3–5). On a scene with a dozen NPCs that's tens of extra draw calls.

**Goal:** **one draw call per character** via `GL_TEXTURE_2D_ARRAY` (or
atlas) — all body materials as layers/sub-rects of a single GL texture,
with per-vertex layer/UV-offset selecting the needed one.

### Current state

- The character geometry is already consolidated into **one VBO** per (chunk, MeshID)
  — after Phase 2 skinning. It contains all vertices of all 15 body parts in
  bind-space, with multi-bone weights.
- The per-material index slice is stored in `skin_consolidated_ranges[]` —
  an array of `(index_start, index_count)` pairs per material.
- The draw loop in `figure.cpp` (`FIGURE_draw_hierarchical_prim_recurse`)
  iterates over materials, and for each:
  - resolves `wRealPage` from `pMat->wTexturePage` (see below about JACKET/OFFSET);
  - selects the `fadeTable` (Tint vs Normal by `TEXTURE_PAGE_FLAG_TINT`);
  - calls `ge_skin_world_draw_range(mesh, start, count, ...)` with the index slice
    and the needed texture.

That is, **the geometry is shared, the separate draws are only due to the texture change**.

### What prevents "just merging"

1. **Texture sizes differ.** For `GL_TEXTURE_2D_ARRAY` all layers = one
   size. Padding smaller ones to the size of the largest → memory overuse.
   The alternative is an atlas (rectangle packing), but that's a different algorithm
   and UV mapping is more complex.

2. **Runtime texture resolve.** Some materials are marked with the flags
   `TEXTURE_PAGE_FLAG_JACKET` / `TEXTURE_PAGE_FLAG_OFFSET` — the actual
   page is computed at runtime from the character's `skill` field (for JACKET) or
   the global `tex_page_offset` (for OFFSET). This means the character's set of
   pages depends on the concrete instance, not just on
   `(chunk, MeshID)`. The array cache can't be keyed simply by
   `(chunk, MeshID)` — a composite key `(chunk, MeshID,
   resolved_page_set)` with LRU eviction is needed.

3. **Per-material `fadeTable`.** Currently Tint and Normal are two tables
   passed as a uniform on a per-material basis. With the 1-draw approach we need
   to either pass **both** and select per-vertex with a flag, or merge
   them into one with an extra indexing coordinate.

4. **PolyPage state.** Currently the state is set per material via
   PolyPage by `wRealPage` (`Cull`, `Blend`, `TextureBlend`). For 1-draw
   we need **one** PolyPage state per character. In fact all body materials
   use the same state (`Cull=CCW / Blend=false /
   TextureBlend=ModulateAlpha`) — it can be fixed on the draw API side.

5. **Transparent body parts (`FIGURE_alpha`).** If a character has an
   alpha-blended material, it can't be merged into one draw with the opaque ones
   — the sort order will break. Solution: a separate alpha pass (a second
   draw per character). In the first version we can leave a fallback to the
   per-material loop for such cases.

### Vertex format extension

Add to `GESkinVertex`:
- `uint8_t layer_index` — the layer index in the texture array.
- `uint8_t tint_flag` — 0 = normal fadeTable, 1 = Tint fadeTable.

When building the consolidated mesh (`figure_build_consolidated_skin_world`)
fill these fields per-vertex based on the vertex's source material.

### Array cache

A new cache `texture_array_cache[]` (LRU, capacity ~16-32 entries):
- **Key:** a hash of the array of resolved page IDs (after applying JACKET/OFFSET).
- **Value:** a GL texture array handle + a `material_index → layer` mapping.
- **Creation:** lazily on the first draw of a character with this resolved page set.
- **Layer size:** the maximum W×H across all incoming textures, the rest are
  padded (transparent fill / edge repeat — to be tuned).

### Shader

- `sampler2D u_texture` → `sampler2DArray u_texture_array`.
- In the vertex shader: pass `a_layer_index` to the fragment as `flat int`.
- In the fragment shader: `texture(u_texture_array, vec3(v_uv, v_layer))`.
- Tint flag: add `uniform uint u_fade_table_normal[64]` and
  `u_fade_table_tint[64]`, in the fragment select by the per-vertex flag.

### Phasing

| Step | What to do |
|-----|-----------|
| 1 | API `ge_texture_array_*` (create / upload / destroy `GL_TEXTURE_2D_ARRAY`). In isolation, without integration with figure. |
| 2 | Extend `GESkinVertex` with `layer_index` + `tint_flag`. Fill in `figure_build_consolidated_skin_world`. The shader ignores it for now. Gate: visually 1:1, nothing broken. |
| 3 | LRU cache `texture_array_by_resolved_pages[]`. Build the array on the first draw with a unique resolved page set. |
| 4 | Shader `skin_world_vert.glsl` → sampler2DArray + per-vertex layer. `ge_skin_world_draw_range` takes an array handle instead of PolyPage. |
| 5 | Per-vertex `tint_flag` → the shader selects the fadeTable from the two. |
| 6 | Replace the per-material loop in figure.cpp + smap.cpp + figure.cpp reflection with **one** `ge_skin_world_draw` call per character. Fallback to the per-material loop for alpha-blended cases. |
| 7 | Measurement: number of draw calls before/after, perf on RTA / Urban Shakedown. Sign-off as visually 1:1 on key scenes. |

### What is NOT in the scope of the first iteration

- Atlas (packed atlas) instead of array — an atlas gives better memory packing for
  different sizes, but is more complex (rectangle packing + UV per-vertex offset).
  Array is simpler and sufficient for UC humans (body textures are fairly
  uniform in size).
- `FIGURE_alpha` — fallback to the old path for alpha-blended characters.
- Non-humans (Balrog, Bane, etc.) — they have fewer materials per character, so the
  optimization is less valuable. Do it as a second pass if needed.

### Gate

- **Visually 1:1** with the old path on: RTA intro (lots of NPCs + Darci with
  a weapon), Urban Shakedown (close-up, skill variants of jackets), night /
  fog (per-vertex fadeTable check), a scene with a transparent character
  (fallback path).
- **Perf:** the number of draw calls per character dropped from 3–5 to 1. On
  a heavy scene (10+ NPCs at once) the overall win is tens of draw calls
  → a measurable FPS gain on a weak GPU.

---

## Increasing the draw distance and fog

Push the fog boundary further from the player — modern hardware can easily
handle it, the current values are a legacy of 1999. When this is done,
the related distances need to be brought up, otherwise artifacts will surface at the new
increased range:

- **Detailed shadow distance** — the constant `AENG_SHADOW_MAX_DIST` in
  `aeng.cpp`. Defined right before the shadow selection loops (grep
  `AENG_SHADOW_MAX_DIST`). Currently `0xC0000` (doubled relative to the
  original `0x60000`). After expanding the fog — raise it
  accordingly. Note: the number of simultaneous detailed shadows is
  limited by `AENG_NUM_SHADOWS = 4` (limited by the shadow texture 64×64 =
  2×2 slots) — see the comment there.
- **Bump mapping (crinkle) fade range** — the constants `CRINKLE_FADE_NEAR` /
  `CRINKLE_FADE_FAR` in `facet.cpp` (at the top of the file, grep `CRINKLE_FADE_NEAR`).
  Currently `[0.3, 0.8]` by post-transform Z. Used via the inline helper
  `crinkle_extrude_for_z()` in three places: `FillFacetPoints`,
  `FillFacetPointsCommon`, `FACET_project_crinkled_shadow`. After
  expanding the fog — move FAR closer to 1.0 (or higher, if Z can
  exceed 1.0 for visible polygons — check).
- **Other distances that may be needed:** `AENG_DRAW_DIST`,
  `AENG_DRAW_PEOPLE_DIST`, farfacet distances — check when
  expanding the fog.
