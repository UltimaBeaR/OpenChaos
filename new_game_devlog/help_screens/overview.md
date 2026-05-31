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
  text overflows the window it **scrolls** line-by-line (▲/▼ arrows + up/down
  nav), drawing only the whole lines that fit. (Originally planned as prev/next
  pages; line scrolling turned out simpler and less disorienting.)
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
- **Small-size fallback:** below `GLYPH_CLEAN_MIN_PX` (32 real px) a clean size
  would be 16/8 — so tiny it renders at ~half the text height and looks broken
  next to the letters. There the clean-size rule is dropped and the glyph is
  **resized to fill the line height** instead (blurry off-mip sampling, but
  legible and matched to the text). ≥32px keeps the crisp clean-size path.

Inline rich-text (also DONE):

- `input_glyph_text_draw(str, x, y, wrap_width, text_scale, colour)` draws normal
  FONT2D text with inline glyph tokens `{name}` (e.g. `{jump}`, `{use}`), word-
  wrapped to `wrap_width`. Returns the height drawn. `input_glyph_advance()`
  shares the size math so wrap measuring matches drawing.
- `input_glyph_text_draw_scrolled(...)` is the scrollable variant: it lays the
  WHOLE string out into wrapped lines once (so the wraps are deterministic and
  never shift as you scroll), then draws only the whole lines that fit a given
  view height starting at a caller-held first-line index (which it clamps and
  writes back), and returns the total line count — enough for the caller to show
  scroll arrows. Both share one line-builder, so wrapping can't diverge.
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

Help screen UI (DONE, in `ui/menus/gamemenu.cpp` + `help_content.{h,cpp}`):

- Pause menu has a **"How to Play"** item (2nd, after Resume). It reuses the
  `X_CONTROLS` id for nav/confirm but draws a literal label (no fitting localized
  string; 1.0 is English).
- Two new GAMEMENU types: `HELP_LIST` (topic list) and `HELP_DETAIL` (text
  screen). Both custom-rendered (their text is literal OpenChaos content, not
  XLAT ids), with their own bounded selection — they never index `GAMEMENU_menu[]`.
- The **topic list** draws in the framed CENTER_CENTER scope (like the other
  menus). The **detail screen** draws in a uniform **full-window** scope
  (`PolyPage::push_uniform_fullscreen_ui_mode`): it spans the whole window with
  the title near the top and the body across nearly the full width — left/right
  padding is a fixed amount in VIRTUAL (pre-scale) px, so the physical margin
  grows on a bigger display and shrinks on a small window — while keeping
  text/glyphs square (no aspect distortion).
- Navigation: in the list ↑↓ moves the selection (wraps), confirm opens a topic,
  cancel/ESC goes detail→list→pause. In the detail screen ↑↓ **scroll** the body.
- Scrolling: the body lays out across a window from below the title to near the
  bottom and draws only the whole lines that fit; `s_help_detail_scroll_line` is
  the first visible line. Step = **1 line** per nav, with its **own fast
  auto-repeat** (separate `InputAutoRepeat`, ~55 ms vs the 150 ms menu default)
  so a held key creeps smoothly. **▲/▼ arrows** (solid triangles on the colour
  page, faded in with the body) show only when there's more above / below.
- **Reveal:** the title's left→right MENUFONT wipe is offset to start at the
  title's own left edge, so it animates almost at once instead of waiting for the
  wipe to crawl from virtual x=0 to the centred title (a long "nothing yet" dead
  time on a wide window). The body fades in as a sharp ease-in BURST that lands at
  a set fraction of the reveal and then holds at full; the arrows fade with it.
  (All tunable via the `*_FADE_*` / `*_WIPE_LEAD` constants in gamemenu.cpp.)
- **Backdrop:** an extra full-screen dim drawn in GAMEMENU's first pass (behind
  the text), fading in fast (well before the body). No bottom hint (cancel/ESC
  goes back, like the other menus).
- **Full-window clip caveat (fixed):** the POLY 2D clip window is the virtual
  640×480 screen with `clip_bottom` fixed at 480, so on a tall window the body
  rows below virtual y=480 were silently culled at `POLY_add_quad` time (text cut
  mid-screen, no arrow). `GAMEMENU_draw_help_detail` now widens
  `POLY_screen_clip_*` to the full window extent for its own draws and restores
  them after. (The horizontal twin of this bug — `clip_right` — was already fixed
  earlier in `POLY_camera_set`.) See the warning on
  `push_uniform_fullscreen_ui_mode`.
- Content: `HELP_TOPICS[]` in help_content.cpp — **placeholder** topics
  (MOVEMENT long enough to scroll; COMBAT / CLIMBING / WEAPONS short). Append
  `{ title, body }` to add; bodies take `{jump}`/`{use}` tokens and any length
  (they scroll).

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

1. **Real content** — replace the placeholder topics with accurate mechanic
   descriptions (and decide the full topic list). The MOVEMENT body is a long
   placeholder used to exercise scrolling.
2. **Steam Deck device detection** (if feasible) to select the `deck` atlas.
3. **Extend the logical-key → Kenney sprite mapping** (`sprite_for` /
   token map in input_glyphs.cpp) beyond the JUMP/USE starter set as content grows.
4. **Font sharpness (optional, cosmetic):** FONT2D is a small bitmap font scaled
   up, so text is softer than the crisp glyphs sitting next to it. Matching them
   would mean improving the font rendering (higher-res font / sharper sampling) —
   a separate effort, deferred. Glyph alpha is already tuned to match brightness.

Done since the first version: full-window (undistorted) detail layout, line
scrolling with fast auto-repeat + ▲/▼ arrows, small-glyph resize fallback, and
the full-window 2D-clip fix. Paging was superseded by scrolling.
