# Help screens — overview

## Goal

A read-only, in-game reference for the game's **mechanics** and how to use them
(what to press, when, and any conditions). Examples of mechanics to cover:
back-walk (hold E / L1 + stick back), rolls (L2 tap + stick), arrest, ladders,
cable/zipwire jumps, etc. Some discoverable/decorative interactions are
deliberately left out as surprises (e.g. dancing next to a dancer, sitting on a
sofa by backing into it).

This is **not** a key-rebinding screen — 1.0 ships fixed bindings.

Entry point: a new **pause-menu** item first; maybe the main menu later.

## Design decisions (agreed)

- **Screen style:** mission-briefing-style — fullscreen background + text. When
  text overflows: **pages** (prev/next), not scrolling (simpler, predictable).
  Word-wrap already exists (`FONT2D_DrawStringWrap`).
- **One screen per mechanic:** the menu lists mechanics; selecting one opens its
  text screen.
- **Device-aware prompts:** swap the button shown to match the active device.
  `active_input_device` already exists and updates live — values
  `INPUT_DEVICE_KEYBOARD_MOUSE / _XBOX / _DUALSENSE` (so we even distinguish
  Xbox vs DualSense). Steam Deck detection is a TODO.
- **Button glyphs:** Kenney Input Prompts (CC0). See "Glyph infrastructure".
- **Inline rendering (TODO):** a small rich-text layer so a glyph flows inside a
  line of text like a letter — a marker token in the string draws a glyph quad
  sized to the font line height, and its width is counted by the wrap pass.

## Glyph infrastructure (DONE — first commit)

The button-glyph atlases are **embedded into the exe** (so a bare exe can draw
basic things even if the user mis-copies it — no external resource folder
needed):

- 4 Kenney atlases (smaller "default" sheets) live in
  `new_game/src/ui/input_glyphs/assets/` as PNGs and are baked into the exe at
  build time (CMake hex-embed, same mechanism as the GLSL shaders).
- Decoded once at startup with vendored **stb_image** (public domain, single
  header) and uploaded to 4 GPU texture pages (`TEXTURE_page_glyph_*`,
  `POLY_PAGE_GLYPH_*`). Entry point: `input_glyphs_init()`.
- Glyph cell coordinates are **hardcoded** in
  `input_glyph_coords.{h,cpp}` (generated once from the pack's atlas XMLs; the
  XMLs are not kept in the repo). Keyed by Kenney sprite name.
- Atlases are named by **style, not brand**, to avoid carrying trademarks:
  `keyboard` (kb+mouse), `generic` (Xbox-style ABXY → `INPUT_DEVICE_XBOX`),
  `ps` (PlayStation-style → `INPUT_DEVICE_DUALSENSE`), `deck` (Steam Deck-style).
- Credits in `THIRD_PARTY_LICENSES.md` (Kenney + stb_image).

Drawing (also DONE):

- `input_glyph_draw(key, x, y, line_height)` draws the prompt for a logical key
  (`InputGlyphKey`, starter set: JUMP, USE), picking the atlas from
  `active_input_device` and the Kenney sprite per device. The glyph is
  left-aligned at x, centred vertically in `line_height`, and the call returns
  the **advance width** (glyph width + a small inter-glyph gap) so prompts lay
  out inline (`x += input_glyph_draw(...)`) without oversized square gaps.
- **Pixel-perfect:** glyphs are minified far below their 64px source, so the
  atlas uses **mipmaps** (the draw path applies trilinear from `has_mipmaps`),
  and the draw picks a **clean on-screen size** — a power-of-two fraction of 64
  (64/32/16/8, or 128 for 2x) — measured in **real framebuffer pixels**,
  centred in the `size` box and **snapped to whole framebuffer pixels** (via
  `PolyPage::ui_scale_*/ui_offset_*`). This keeps outlines crisp and avoids the
  one-sided blur that arbitrary sub-pixel scaling caused. `ge_texture_load_rgba`
  gained a `gen_mipmaps` flag for this.

Inline rich-text (also DONE):

- `input_glyph_text_draw(str, x, y, wrap_width, text_scale, colour)` draws normal
  FONT2D text with inline glyph tokens `{name}` (e.g. `{jump}`, `{use}`), word-
  wrapped to `wrap_width`. Returns the height drawn. `input_glyph_advance()`
  shares the size math so wrap measuring matches drawing.
- Spacing: a space is inserted between atoms ONLY where the source has one, so
  `{jump}{use}` sits flush and `a {jump} b` keeps its spaces (no extra padding).
- Vertical alignment / crispness: the glyph centre is aligned to the FONT2D
  letter's visual centre (from its quad metrics), plus a small whole-pixel nudge
  (`INPUT_GLYPH_VNUDGE_PX`) to optically centre it on the ink. For the glyph and
  the text to be BOTH crisp AND stay aligned across UI scales, **FONT2D now
  pixel-snaps each letter quad to the real framebuffer grid** (global change in
  font2d.cpp — text on the virtual grid + glyphs on the real grid drifted ±1px as
  the scale changed). Residual ≤1px alignment variance is inherent (letters and
  glyphs have different pixel heights/parities when both are crisp-snapped).
- Colour match: FONT2D text reads slightly COOL (greenish-blue) and a touch
  translucent. A pure-white opaque glyph stood out, so the glyph is drawn tinted
  cool and at reduced alpha (`INPUT_GLYPH_DRAW_COLOUR` — ARGB, alpha for
  brightness + cool RGB for the tint) to match the text.

Help screen UI (DONE — first version, in `ui/menus/gamemenu.cpp` + `help_content.{h,cpp}`):

- Pause menu has a **"How to Play"** item (2nd, after Resume). It reuses the
  `X_CONTROLS` id for nav/confirm but draws a literal label (no fitting localized
  string; 1.0 is English).
- Two new GAMEMENU types: `HELP_LIST` (topic list) and `HELP_DETAIL` (text
  screen). Both custom-rendered (their text is literal OpenChaos content, not
  XLAT ids), with their own bounded selection — they never index `GAMEMENU_menu[]`.
- Navigation: ↑↓ moves the list selection (wraps), confirm opens a topic, cancel/
  ESC goes detail→list→pause. Reuses the existing menu input channels.
- Detail screen: title + body via `input_glyph_text_draw` (smaller, ~0.75x), over
  an **extra full-screen dim** drawn in GAMEMENU's first pass (behind the text).
  The dim **fades in** (full-screen alpha ramp) a bit faster than the body, and
  the body **fades in** synced to the menu reveal. No bottom hint (cancel/ESC
  goes back, consistent with the other menus which show no hint either).
- Content: `HELP_TOPICS[]` in help_content.cpp — **placeholder** topics
  (MOVEMENT / COMBAT / CLIMBING / WEAPONS), short so they fit one screen. Append
  `{ title, body }` to add; bodies take `{jump}`/`{use}` tokens.

Flagged by the user as a **first version — needs more polish** (e.g. detail-screen
layout/visual refinement). Not yet: paging, real content.

### Licensing / trademark notes

- Kenney pack is **CC0** — covers the artwork; no attribution required (credited
  anyway). stb_image is public domain.
- The button **symbols** (PlayStation ✕○□△, Xbox ABXY) are **trademarks** of
  Sony/Microsoft; CC0 does not grant those. Sony has registered the four shapes
  and defends them, but against branding/advertising use — using glyphs as
  **functional** "press this button" prompts is the common, low-risk case.
  Mitigations applied: style-not-brand naming, no PlayStation/Xbox wording in
  our file/identifier names, functional use only. (Kenney sprite-name strings
  still contain "xbox"/"playstation" as internal data keys.)

## Follow-ups (TODO)

1. **Polish the help screens** (first version flagged for more work): detail-screen
   layout/visual refinement, list styling, dim level / fade tuning.
2. **Paging** on the detail screen for bodies longer than one screen.
3. **Real content** — replace the placeholder topics with accurate mechanic
   descriptions (and decide the full topic list).
4. **Steam Deck device detection** (if feasible) to select the `deck` atlas.
5. **Extend the logical-key → Kenney sprite mapping** (`sprite_for` /
   token map in input_glyphs.cpp) beyond the JUMP/USE starter set as content grows.
6. **Font sharpness (optional, cosmetic):** FONT2D is a small bitmap font scaled
   up, so text is softer than the crisp glyphs sitting next to it. Matching them
   would mean improving the font rendering (higher-res font / sharper sampling) —
   a separate effort, deferred. Glyph alpha is already tuned to match brightness.
