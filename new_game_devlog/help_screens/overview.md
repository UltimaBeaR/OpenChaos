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

1. **Inline rich-text glyphs + paging** for the help screen text — a glyph
   flowing inside a line of text (marker token → glyph quad on the baseline,
   counted by the wrap pass), and pages when text overflows. (Single-glyph draw
   + pixel-perfect are done; this is the in-text integration.)
2. **The help screen UI itself:** pause-menu item → mechanics list → per-mechanic
   text screen. Plus deciding which mechanics to document.
3. **Steam Deck device detection** (if feasible) to select the `deck` atlas.
4. **Extend the logical-key → Kenney sprite mapping** (`sprite_for` in
   input_glyphs.cpp) beyond the JUMP/USE starter set as help content grows.
