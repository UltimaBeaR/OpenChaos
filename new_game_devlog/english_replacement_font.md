# Localization-safe English font (alt FONT2D + alt MENUFONT atlases)

Status: help body (FONT2D path) + help menu/list/title + pause "Help" item
(MENUFONT path). Both font VISUALS are **DONE & approved**:
- MENU font — weathered look (soft edges, halo glow, short frequent grunge streaks
  with carved holes), baked into the atlas.
- HELP BODY font (FONT2D) — rendered SMALLER in the atlas (anti-aliased downscale +
  edge-contrast), letters centred in their cells, translucency matched to the inline
  glyphs; the inline glyphs were de-tinted (neutral) + re-centred, and the scroll
  arrows de-tinted + opacity-matched. A real word-wrap bug (measure used the game
  font, draw used ours) was fixed in passing.

The F9 dev console was also routed onto the alt atlas (it draws via
`FONT2D_DrawString` which already takes a `page` param — just pass
`POLY_PAGE_FONT2D_ALT`; colours unchanged).

Fixed-English HUD popup messages (`CONSOLE_text` / `PANEL_new_text` → the
`PANEL_text[]` bubble list) were handled too: that queue is SHARED with localised
text (NPC dialogue, cop lines), so a per-message `alt_font` flag was added. The new
`CONSOLE_text_en` / `PANEL_new_text_alt` set it (verbatim, no printf) and the draw
picks `POLY_PAGE_FONT2D_ALT`; plain `CONSOLE_text` / `PANEL_new_text` stay on the
game font for localised/resource text.

The rule: **every 100% English, non-resource string → alt font.**
- Alt (`CONSOLE_text_en` / `PANEL_new_text_alt`): cheats, F9 command replies, debug
  toggles, the EWAY waypoint DEV-error strings (`"Waypoint %d ..."`, formatted in
  code — NOT mission text despite the `EWAY_message` name), and dev hints like
  `"There is room behind Darci"`.
- Game font (untouched): NPC conversations (EWAY conversation `str`), cop dialogue
  (`XLAT_str(X_GET_DOWN)` / `X_CANT_SHOOT_COP`), and the fake-wander lines
  (`EWAY_get_fake_wander_message`, which is language-dependent) — all genuinely
  localised / resource text.

Tracked as a post-1.0 task in
`new_game_planning/known_issues_and_bugs_post_1_0.md` (section "UI и опции").

## The problem

Our build keeps some text **always in English** (text we added that isn't in the
original game): the in-game **help** screen (pause menu → `help`, with the
controls scheme + mechanic descriptions), debug/console, etc.

Unofficial localizations (e.g. fan Russian patches) **overwrite the game's font
atlases** — `font.TGA`/`multifontPC.tga` (FONT2D) and `olyfont2.tga` (MenuFont) —
replacing Latin glyph cells with Cyrillic. Those are **game data resources**, not
our code. Their translations only cover text that was already in the game, not our
custom additions. Result: our always-English text gets drawn with Cyrillic/garbage
glyphs (broken) under such a patch.

### Hard constraints (why it's not trivial)

1. **Licensing:** we must NOT ship/embed the game's `.tga` atlases (game data, not
   ours). We CAN embed code-level fonts: `font8x8` (public domain) and the original
   `FONT` 5×9 (`uc_orig`, MIT — the original MuckyFoot source is MIT).
2. **No runtime detection:** you can't tell a Russified install from an English one
   at runtime (no marker; atlases differ per patch). So you can't do "English →
   one thing, Russian → another" — one behaviour for all installs.
3. A different-looking font for our text would jar in a *normal* English install.
   But since (2) forbids detection, and the help is on its own screen, we accept a
   slightly different (embedded) font for the help/custom text.

Decision (1.0 stop-gap): render our always-English text through a **license-clean
embedded font** baked into our own texture, so it never depends on the game atlas.
Proper full localization (own atlases incl. Cyrillic) is a separate post-1.0
question — NOT decided.

## The architecture (texture/table swap, NOT a new text system)

Key idea (user's): **don't rewrite the font systems**. Generate a replacement
atlas with the SAME draw path, and only swap which texture + glyph metrics the
draw uses, for our text only.

FONT2D already takes a `page` parameter, so it was the natural first target.

Pieces:

- **Source glyphs:** `font8x8` (public domain, 8×8). Defined in
  `game/missing_resources.cpp` as `g_font8x8_basic[95][8]` (made `extern`),
  declared in `engine/graphics/text/font8x8.h`. Same data the "files not found"
  screen uses.
- **Texture slot:** `TEXTURE_page_font2d_alt = TEXTURE_NUM_STANDARD + 80` (free
  slot; range allocated to +110). Assigned + filled in `assets/texture.cpp` right
  after `FONT2D_init`. Uploaded from RAM via `ge_texture_load_rgba()`.
- **POLY page:** `POLY_PAGE_FONT2D_ALT = POLY_NUM_PAGES - 91` (the one free gap in
  the -N sequence). `poly_render.cpp` has a `case` for it: identical render state
  to `POLY_PAGE_FONT2D` but `SET_TEXTURE(TEXTURE_page_font2d_alt)`.
- **Alt glyph table:** `FONT2D_letter_alt[]` (static in `font2d.cpp`) — the alt
  font's OWN u/v + widths (see "consistent widths" below). The normal
  `FONT2D_letter[]` is untouched.
- **Generator:** `FONT2D_build_alt_atlas(slot)` in `font2d.cpp` — rasterises
  font8x8 into a 256×256 RGBA buffer in our own grid, fills `FONT2D_letter_alt`,
  uploads. Called once at startup after `FONT2D_init` (needs the atlas size).
- **Draw branch:** `FONT2D_DrawLetter` picks `FONT2D_letter_alt[]` when
  `page == POLY_PAGE_FONT2D_ALT`, else `FONT2D_letter[]`. One line, normal text
  unaffected.
- **Layout widths:** `FONT2D_GetLetterWidthAlt()` mirrors `FONT2D_GetLetterWidth`
  but reads the alt table — so text laid out for the alt font wraps/advances to
  match the alt draw.

### Colour / transparency (works for free)

The alt atlas glyphs are **opaque white**. The FONT2D render state is
`ModulateAlpha`, so the draw multiplies the texture by the vertex colour (`rgb`)
and alpha (`fade`). ⇒ the SAME alt font draws in **any colour / any transparency**
just by passing different `rgb`/`fade` to `FONT2D_DrawLetter`. The menu (later
slice) will reuse it with its own colour — no extra code.

### Effects (dithering etc.) — easy by design

The alt atlas is **generated by us in RAM**, so any effect (dithering, outline,
glow) can be baked in at generation time, or we generate variant atlases and
switch the page. Not done yet; the hook is there.

## Rendering rules learned the hard way (DON'T regress these)

The glyphs MUST be rendered at the **SAME INTEGER scale on BOTH axes** — exactly
what the "files not found" screen does (each source pixel → a `SCALE`×`SCALE`
block). `FONT2D_ALT_SCALE = 2` → 16×16 glyphs, the right size for the ~16px line.

Why integer + equal on both axes (the bugs we hit, in order):

1. **Fractional scale (1.25×, 8→10):** non-integer scaling of a bitmap makes some
   strokes 1px and some 2px, and AA leaves a faint thin right edge → "T right edge
   squeezed to 1px, middle normal", inconsistent `.`/`:` widths.
2. **1× wide × 2× tall:** unequal axes → letters compressed horizontally and `.`
   stretched vertically.
3. **Fix = 2× on both axes, integer:** every pixel square, no stretch. Matches the
   reference (the missing-resources screen). This is the version the user approved.

Other details:
- **Proportional advance:** each glyph's drawn/advance WIDTH is trimmed to its
  source ink columns (`lo..hi`, scaled), so spacing stays proportional (narrow
  'i', wide 'M') without per-letter horizontal stretch.
- **Atlas stays 256×256.** `FONT2D_DrawLetter` converts width→UV with a hardcoded
  `/256`, so `FONT2D_letter_alt.width`/`u`/`v` are in 256-atlas-pixel space.
- **Vertical region is 18px** (`FONT2D_ALT_CELL_H`, mirrors the draw's `v..v+18/256`
  sample span). The glyph is centred in it (`top_pad`).

### Size reduction + anti-aliasing (UPDATE — supersedes the integer-2× above)

The 2× integer render (16px glyph, hard edges) read TOO BIG next to the old game
font (font8x8 fills its cell more) and a bit harsh. The body atlas now:
- renders each 8×8 glyph into an explicit `FONT2D_ALT_BOX`-px square (currently 12,
  < the 18px cell) — SMALLER letters at the SAME line height, since the draw samples
  the whole 18px cell into the fixed quad, so visible size = BOX / CELL_H of it;
- does this with an **area-averaged supersampled downscale** (`FONT2D_ALT_SS` sub-
  samples) → anti-aliased edges baked as ALPHA (rgb stays white so the vertex colour
  still tints), then a contrast remap (`FONT2D_ALT_EDGE_GAIN`) sharpens the edge back
  so it isn't mushy;
- centres the glyph in its advance cell (the trailing inter-letter gap is split
  half-left / half-right via a DRAW-only x shift in `FONT2D_DrawLetter`, advance
  unchanged) so narrow glyphs aren't shoved left.

### Constants (font2d.cpp)

```
FONT2D_ALT_BOX           = 12   // glyph square in the cell (< CELL_H 18). SMALLER = smaller text
FONT2D_ALT_SS            = 4    // supersample per axis for the AA downscale
FONT2D_ALT_EDGE_GAIN     = 2.2  // edge contrast (1.0 = soft linear AA, higher = crisper)
FONT2D_ALT_CELL_H        = 18   // draw samples v..v+18/256
FONT2D_ALT_ATLAS_SIZE    = 256  // tied to the /256 UV math in FONT2D_DrawLetter
FONT2D_ALT_GAP_X         = 2
FONT2D_ALT_LETTER_SPACING= 2    // inter-letter gap (atlas px), alt page only; halved into bearings
```

## How the help body uses it (input_glyphs.cpp)

The help DETAIL body is rich-text (text + inline button glyphs), rendered by
`input_glyph_text_draw` / `input_glyph_text_draw_scrolled` →
`draw_one_line` → `FONT2D_DrawLetter`.

- `draw_one_line` got a `font_page` param; the help callers pass
  `POLY_PAGE_FONT2D_ALT` (so text draws via the alt atlas).
- `build_lines` / `text_char_width` got a `use_alt` flag (default false); the help
  callers pass `true` so wrap/advance use `FONT2D_GetLetterWidthAlt` (matching the
  alt draw). The **catalog** path (`input_prompt_catalog_draw_scrolled`, the
  input-test screen) is left on the normal page/widths.
  - **Wrap bug fixed:** `build_lines`' normal-text branch was measuring with the GAME
    font (`text_char_width` called without `use_alt`) while the body draws with OURS.
    Harmless when the two fonts had near-equal widths; once the body font was shrunk
    the widths diverged and lines overflowed the wrap width. Now it passes `use_alt`.
- **Inter-letter spacing:** font8x8 glyphs are tight (no side bearing), so the normal
  `width+1` advance left letters almost touching. The alt page uses a wider gap
  (`FONT2D_ALT_LETTER_SPACING`, atlas px) — normal text keeps `+1`. FONT2D draws each
  glyph at its ink width and advances separately, so a bigger advance alone opens a
  clean gap (no atlas change). The gap is then split into equal left/right bearings
  by the draw-only centring shift (see size section). Layout and draw read the same
  gap so wrapping matches.
- **Shared body translucency:** the body text, the inline glyphs and the scroll
  arrows all draw at one opacity, `INPUT_GLYPH_TEXT_BASE_ALPHA` (input_glyphs.h), so
  they read as one element. The text gets it by scaling its fade by that alpha; the
  glyph colour is built from it; the arrows multiply their fade by it.
- **Inline glyphs:** de-tinted to NEUTRAL white (the old bluish tint matched the old
  game font; the replacement font isn't blue) and vertically centred on the FONT2D
  letter cell (`glyph_y_off`, a pixel-snapped SHIFT — never a rescale, so the
  pixel-perfect glyph is preserved). Scroll arrows likewise de-tinted to white.

## Files touched

- `engine/graphics/text/font8x8.h` (new) — extern decl of the PD glyph table.
- `game/missing_resources.cpp` — `FONT8X8` → `extern g_font8x8_basic` (shared).
- `engine/graphics/text/font2d.cpp` / `.h` — `FONT2D_letter_alt`,
  `FONT2D_build_alt_atlas`, alt branch in `FONT2D_DrawLetter`,
  `FONT2D_GetLetterWidthAlt`.
- `assets/texture.cpp` + `texture.h` + `texture_globals.{h,cpp}` —
  `TEXTURE_page_font2d_alt` slot + generate/upload.
- `engine/graphics/pipeline/poly.h` — `POLY_PAGE_FONT2D_ALT`.
- `engine/graphics/pipeline/poly_render.cpp` — render-state case.
- `ui/input_glyphs/input_glyphs.cpp` — `font_page` + `use_alt` threading for the
  help text path.

## The MENU font slice (alt MENUFONT)

The menu font (olyfont2.tga, the proportional menu/title font with the fade-in
"reveal wipe") is the SECOND atlas a localisation overwrites. The help screens use
it for: the topic-LIST heading + items, and the topic-DETAIL TITLE. (The detail
BODY is the FONT2D path above.)

### Why the mechanism differs from FONT2D

FONT2D's draw takes a `page` parameter, so we branch on the page inside the draw.
The MENU font instead draws through **globals** — `FontInfo[256]` (per-char glyph
table) + `FontPage` (the POLY page) — with no per-call page argument. So instead
of a branch we **swap those globals** for the duration of our draw and restore
them after. That's `MENUFONT_AltScope` (RAII): ctor saves `FontInfo`+`FontPage`
and copies the alt table / alt page in; dtor restores. Zero changes to the
MENUFONT draw/fade-in code — it just reads whatever's in the globals. Cost: two
~5 KB `memcpy`s per help frame (help screen only, not a gameplay path).

### Render path / blend (additive, not ModulateAlpha)

The menu font page (`POLY_PAGE_NEWFONT_INVERSE`) is **additive** (`One`/`One`,
`Modulate` texture blend): glyphs are bright-on-black and ADD onto the scene; the
fade-in passes a grey `colour` = brightness on all channels. Our alt atlas is
**white ink on black**, so with the same additive state a white glyph adds the
vertex colour ⇒ grey text at brightness `fade`, exactly like olyfont2. The alt
page `POLY_PAGE_MENUFONT_ALT` copies NEWFONT_INVERSE's state and only swaps the
texture. Colour/brightness already work (vertex colour); a coloured menu (vs the
current grey) is a later tuning step (the fade-in path currently hardcodes grey
from `fade`).

### All-caps (matches the game menu)

The game menu font renders ALL-CAPS (olyfont2 has no lowercase; `MENUFONT_MergeLower`
copies each uppercase glyph into the lowercase slot). To stay consistent with the
surrounding menus (the pause menu still uses olyfont2), the alt menu font does the
same: it rasterises only the non-lowercase printable set and copies uppercase →
lowercase in `FontInfo_alt`. So "Help" → "HELP", topic titles all-caps. (font8x8
HAS lowercase; switching to real mixed case is a one-line change for later if we
ever move the whole menu to the alt font.)

### Sizing (taller than wide) — separate from the FONT2D body

The menu fade-in draws glyphs at their NATIVE `FontInfo.width/height` (no scale
multiply), so the stored size IS the on-screen size (drawn 1:1 with the atlas). The
game menu letters are taller than wide, so the glyph box is taller than wide:
- **Width** = integer scale of the 8px source: `MENUFONT_ALT_SCALE_X = 3` → 24px
  (crisp).
- **Height** = an EXPLICIT pixel target `MENUFONT_ALT_GLYPH_H` (currently 29, tuned
  against the game font), NOT an integer multiple of 8 — the 8 source rows are
  resampled (nearest) into it. This lets the height be fine-tuned ~1px at a time
  without touching the width OR the streak/grunge size (those are in atlas px, so
  they stay put when the height changes). The weathering blur hides the slightly
  uneven row heights from the non-integer ratio.

The stored `width`/`height` also include the halo margin (see effects), so the drawn
quad has room for the glow. (Independent of the FONT2D help-BODY, a different atlas.)

### Weathered-look effects (baked into the atlas)

The game menu font is soft, slightly glowing and grungy (irregular bright/dark
horizontal streaks with carved-out holes). We approximate that on the hard font8x8
mask, ALL at atlas-generation time — runtime stays colour/alpha/quad-size only. The
builder works in a float intensity buffer then bakes the result into the RGBA:

1. **Rasterise** the hard 0/1 glyph mask (offset by a blank `HALO_MARGIN` on every
   side so the blur/halo has room).
2. **Core** = small blur (`CORE_BLUR_R`) × `CORE_GAIN` → soft anti-aliased edges
   with solid stroke cores. The gain sets letter-edge crispness (higher = sharper).
3. **Halo** = a wider, fainter blur (`HALO_BLUR_R` × `HALO_STRENGTH`) → the glow /
   "ареол" around the letter. The sharp letter = `max(core, halo)`.
4. **Grunge** = two octaves of value noise stretched SHORT-in-X / THIN-in-Y → short,
   frequent horizontal bands, as a per-pixel multiplier: positive noise BRIGHTENS
   (toward white, `GRUNGE_BRIGHT`); negative noise CARVES toward a FULL hole once it
   passes a threshold ramp (`CARVE_START`..`CARVE_FULL`) — real holes, not a dim wash.
5. **Blur ONLY the grunge multiplier** (`STREAK_BLUR_R`, blended by `STREAK_BLUR_MIX`
   so it can soften less than a whole pixel) → soft streak/hole edges WITHOUT
   blurring the letter shape. Final intensity = sharp letter × soft grunge.

**Determinism:** the grunge comes from a pure positional integer hash
(`menufont_alt_hash01`) seeded by a hardcoded constant (`MENUFONT_ALT_GRUNGE_SEED`) —
NOT `rand()` or any global RNG. The atlas is byte-identical every launch and fully
independent of any randomness the game uses elsewhere. Never tie the seed to a
runtime value.

**Atlas size:** the halo margins push it past 256, so the menu atlas is **512×512**
(~1 MB, generated once at startup then freed from RAM — only the GPU texture stays).

### Files touched (menu slice)

- `engine/graphics/text/menufont.{h,cpp}` — `FontInfo_alt`, `MENUFONT_alt_page`,
  `MENUFONT_build_alt_atlas`, `MENUFONT_AltScope`. (font8x8.h + ge_texture_load_rgba
  includes added.)
- `engine/graphics/pipeline/poly.h` — `POLY_PAGE_MENUFONT_ALT` (N=111, the last
  free special-page slot).
- `engine/graphics/pipeline/poly_render.cpp` — render-state case (mirror
  NEWFONT_INVERSE, alt texture).
- `assets/texture.{cpp,h}` + `texture_globals.{h,cpp}` — `TEXTURE_page_menufont_alt`
  (`TEXTURE_NUM_STANDARD + 81`), built right after the FONT2D alt atlas.
- `ui/menus/gamemenu.cpp` — `MENUFONT_AltScope` wraps: the help-list draw, the
  help-detail TITLE block (the detail BODY already uses the FONT2D alt path), AND
  the pause menu's "Help" item itself (X_CONTROLS) — that label is our always-English
  literal, so it goes through the alt atlas; the OTHER pause items are localized
  game strings (XLAT_str) and stay on the game atlas so their translation shows.

### Menu-slice constants (menufont.cpp) — all TUNABLE, these are the approved values

```
// Geometry
MENUFONT_ALT_ATLAS_SIZE = 512   // halo margins don't fit 256
MENUFONT_ALT_SCALE_X    = 3     // 8*X = 24px glyph width (integer-scaled, crisp)
MENUFONT_ALT_GLYPH_H    = 29    // explicit px height (source resampled), taller than wide
MENUFONT_ALT_GAP        = 2     // cell bleed-guard gap
MENUFONT_ALT_HALO_MARGIN= 4     // blank ring around each glyph for the glow/blur
MENUFONT_ALT_SPACING_SRC_PX = 2 // inter-letter gap in SOURCE px, * SCALE_X
MENUFONT_ALT_SPACE_SRC_PX   = 4 // space advance in SOURCE px, * SCALE_X
// Edge softening (the letter)
MENUFONT_ALT_CORE_BLUR_R = 1
MENUFONT_ALT_CORE_GAIN   = 2.9  // higher = sharper letter edges
// Halo / glow
MENUFONT_ALT_HALO_BLUR_R   = 3
MENUFONT_ALT_HALO_STRENGTH = 0.75
// Grunge streaks (bright) + carved holes (dark)
MENUFONT_ALT_GRUNGE_BRIGHT = 0.9
MENUFONT_ALT_CARVE_START   = 0.28   // |neg noise| below this: no hole
MENUFONT_ALT_CARVE_FULL    = 0.60   // at/above this: full hole
MENUFONT_ALT_STREAK_LEN1/THICK1 = 5.8 / 1.15   // broad octave (short, frequent)
MENUFONT_ALT_STREAK_LEN2/THICK2 = 2.3 / 0.6    // fine octave
MENUFONT_ALT_GRUNGE_W1/W2  = 0.65 / 0.35       // octave weights
MENUFONT_ALT_STREAK_BLUR_R = 1, _MIX = 0.5     // soften streaks only, < 1px
MENUFONT_ALT_GRUNGE_SEED   = 0x9E3779B9         // fixed pattern (determinism)
```

The letter-spacing baked-strip note below still applies, but the cell is now
`GLYPH_W + 2*HALO_MARGIN + SPACING_PX + GAP` wide × `GLYPH_H + 2*HALO_MARGIN + GAP`
tall, and the UV box is extended by `HALO_MARGIN` on every side so the glow shows.

### Letter spacing — must be baked into the atlas, NOT added to the advance

First attempt added the gap to `FontInfo.width` (the advance). That did NOTHING
visible, because the fade-in draw (`MENUFONT_fadein_char`) **stretches each glyph's
atlas region across its full `width` AND advances by `width`** — so a bigger width
just stretches the glyph to fill it, with the next letter still flush against it.
(The glyph's UV covered only the ink, so the ink was stretched wider — no gap.)

Fix: bake the spacing into the atlas as an **empty strip to the RIGHT of the ink**,
and extend the glyph's UV to cover ink + strip. Now the draw maps ink 1:1 (no
stretch) and the blank strip (additive-black = invisible) becomes the gap. The cell
is widened to `GLYPH_PX + SPACING_PX + GAP` to hold the strip; `ink_x + ink_w <=
GLYPH_PX` guarantees the strip stays inside the cell and never samples the next
glyph. This mirrors how the game's menu font stores per-glyph padding inside each
atlas cell. Spacing is in SOURCE font pixels (×SCALE) so the gap scales with the
font (the game menu font has wide letter spacing); it's a starting value to tune.

## Pending / next slices

- **DONE:** menu-font visual, help-body visual (size + AA + edge contrast), inline
  glyph de-tint/re-centre + shared translucency, scroll arrows, and the F9 console
  (routed onto the alt atlas).
- **Pause menu / other menus**: still olyfont2 (break under localisation). Only the
  help screens were routed to the alt menu font for now. Routing the whole menu is
  the same `MENUFONT_AltScope` wrap (decide later; would also let us go mixed-case).
- A COLOURED (vs grey) menu and baked effects beyond what's there are optional
  future polish, not required.
- Possible refinement: bigger FONT2D atlas + reworked `/256` UV math if we ever
  want a higher-res source (the user explicitly does NOT want a TTF — font8x8 is
  the accepted look).
