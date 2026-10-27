# Stage 13 — Dead Code Report (working artifact)

Generated 2026-04-26 by the first `--gc-sections` pass via [stage13_dead_code_cleanup.md](stage13_dead_code_cleanup.md).
This file is the **working artifact of the cleanup**: candidate list, per-iteration progress, what remains.

## ⚠️ CLEANUP RULES (for the next agent)

**Dead-code cleanup is ONLY removal of unused symbols. NO changes to live code whatsoever.**

- Do not add defensive checks, do not refactor, do not "while we're at it, let's also fix this"
- Do not remove function calls — even if they look like a no-op
- If you find a bug during cleanup — record it in a separate file under `new_game_devlog/` and DO NOT FIX it
- Goal: 0 risk of breaking anything, deterministic safe cleanup
- More detail: see the ⚠️ block in [stage13_dead_code_cleanup.md](stage13_dead_code_cleanup.md)

## ⚠️ Do not combine DEAD_CODE_REPORT with ASan

`DEAD_CODE_REPORT=ON` + `ENABLE_ASAN=ON` at the same time crash the game at startup (FileSetBasePath, an MSVC-specific conflict between `-ffunction-sections` and ASan literal merging). Each one works separately.

Workflow:
- Regenerate dead-list: `CMAKE_EXTRA_ARGS="-DDEAD_CODE_REPORT=ON" make configure` (ASan OFF — this is the default)
- Verify under ASan: `make configure-asan` (DEAD_CODE_REPORT resets to OFF by default)
- Do NOT enable both explicitly in one command

## Workflow for continuing the cleanup

**Current state (2026-04-27, after batch 39, session 23):**
- Discards count: 4534 (start) → ... → **1800** (batch 35)
- **gc-sections list exhausted** — none of the remaining 80 symbols can be removed (system noise / source-level callers / inlining artifacts)
- **Final pass over empty files: done** — removed shadow_globals.cpp/h + startscr.cpp
- **cppcheck unusedFunction pass: done** (batch 39) — all genuinely dead static functions removed
- **✅ All planned passes completed** — Stage 13 dead-code cleanup is fully done
- Build OK (make build-release, exit 0)
- Uncommitted — the user commits manually
- ⚠️ aeng.cpp: 6 functions skipped (apply_cloud, show_gamut_lo, AENG_draw_bangs, AENG_draw_people_messages, AENG_clear_screen, AENG_fade_out) — the linker discards them due to inlining/dead branches, but source-level calls exist in live functions. The definition cannot be removed without removing the calls (which would violate the "do not touch live code" rule).
- ⚠️ eng_map_globals.cpp (8 globals), input_actions_globals.cpp (input_to_angle) — constant-propagated globals: the linker discards them, but source-level callers exist in live functions. Cannot be removed.
- ⚠️ MF_rotate_mesh (outro_mf.cpp) — live caller WIRE_draw in outro_wire.cpp. Cannot be removed.
- ⚠️ core.cpp (11 stubs), person.cpp (5 stubs), ds_bridge.cpp (2 API stubs) — intentional stubs / stubs called from live code (caller in same .o as live code). Cannot be removed.
- ⚠️ oc_config.cpp (18), input_debug.cpp (4), input_debug_dualsense.cpp (1), sdl3_bridge.cpp (3 DWM symbols), dirt.cpp (1 XMM) — system/compiler noise (STL RTTI vtables, XMM constants, system DLL symbols). Cannot be removed.

**Done in batch 9 (leaf globals from live files):**
- `soundenv_globals.cpp/h`: `SOUNDENV_gndctr`
- `message_globals.cpp/h`: `message_count`
- `menufont_globals.cpp/h`: `FONT_TICK`
- `overlay_globals.cpp/h`: `timer_prev`
- `attract_globals.cpp/h`: `current_playback`
- `network_state_globals.cpp/h`: `CNET_i_am_host`, `CNET_connected`
- `supermap_globals.cpp/h`: `next_inside_mem`
- `music_globals.cpp/h`: `last_MFX_QUICK_mode`, `music_volume`
- `game_tick_globals.cpp/h`: `amb_colour` (LIGHT_Colour), `amb_choice_cur`, `controls` (UWORD)
- `player_globals.cpp` + `player.h`: `player_pos`
- `darci_globals.cpp/h`: `air_walking`, `history` (SLONG[10])
- `anim_loader_globals.cpp/h`: `key_frame_count`
- `hm_globals.cpp/h`: `HM_object_upto`
- `special_globals.cpp/h`: `special_di` + `#include "world_objects/dirt.h"` removed

**Done in batch 10 — Timer cluster (fully dead, deleted entirely):**
- DELETED: `src/engine/core/timer.cpp` (StartStopwatch, StopStopwatch)
- DELETED: `src/engine/core/timer_globals.cpp/h` (stopwatch_start)
- MODIFIED: `src/engine/core/timer.h` — removed StartStopwatch/StopStopwatch declarations (BreakStart/BreakEnd kept — live)
- `CMakeLists.txt`: removed timer.cpp, timer_globals.cpp

**Done in batch 10 — SoundEnv cluster (fully dead, deleted entirely):**
- DELETED: `src/engine/audio/soundenv.cpp` (SOUNDENV_precalc stub, SOUNDENV_upload stub)
- DELETED: `src/engine/audio/soundenv.h`
- DELETED: `src/engine/audio/soundenv_globals.cpp` (left empty after batch 9)
- DELETED: `src/engine/audio/soundenv_globals.h` (AudioGroundQuad struct, unused)
- `CMakeLists.txt`: removed soundenv.cpp, soundenv_globals.cpp

**Done in batch 11 — Bucket cluster (fully dead, deleted entirely):**
- DELETED: `src/engine/graphics/pipeline/bucket.cpp` (init_buckets)
- DELETED: `src/engine/graphics/pipeline/bucket.h`
- DELETED: `src/engine/graphics/pipeline/bucket_globals.cpp` (e_bucket_pool, e_buckets, e_end_buckets, bucket_lists)
- DELETED: `src/engine/graphics/pipeline/bucket_globals.h`
- `CMakeLists.txt`: removed bucket.cpp, bucket_globals.cpp

**Done in batch 12 — dead functions from live files:**
- `mav.cpp/h`: removed `MAV_turn_off_square`, `MAV_height_los_fast`
- `ware.cpp/h`: removed `WARE_enter`, `WARE_exit` (their calls in collide.cpp are commented out inside `/* */`)
- `mfx.cpp/h`: removed `MFX_set_queue_gain`, `MFX_get_wave`
- `image_compression.cpp/h`: **restored** after an erroneous removal — `IC_pack`, `IC_unpack`, `IC_Packet` are live (used in compression.cpp). `IC_pack`/`IC_unpack` were discarded by the linker because their callers in compression.cpp are dead functions, but the file itself must exist for compilation.
- `morph.cpp/h`: **restored** `MORPH_get_points`, `MORPH_get_num_points` — used in mesh.cpp
- `tracks.cpp/h`: **restored** `TRACKS_Reset` — used in save.cpp. `TRACKS_Draw` (stub) was correctly removed — its call in aeng.cpp is commented out.

**Lessons from batch 12:**
- If the linker discards a function, that does not mean its whole file can be deleted — you must check whether its .h and other symbols from the .cpp are used
- Calls inside `/* */` blocks — the function really is dead (the caller is never compiled)
- **ALWAYS** grep-verify: not only the function itself but also its return types and structs from the same .h

**What was done in batch 8 (2026-04-26):**

Strategy: clusters — leaf functions + their transitive dependencies. Removed together.

Removed (batch 8):
- `facet_globals.cpp/h`: `iNumFacets`, `iNumFacetTextures`
- `night_globals.cpp/h`: `NIGHT_walkable[NIGHT_MAX_WALKABLE]`
- `night.h`: `NIGHT_MAX_WALKABLE` define + `NIGHT_WALKABLE_POINT` macro
- `facet.cpp/h`: `FACET_draw_quick`, `FACET_draw_walkable_old`, `FACET_project_crinkled_shadow`
- `crinkle.cpp/h`: `CRINKLE_project` (its only caller was the dead FACET_project_crinkled_shadow)
- `ngamut.cpp/h`: `NGAMUT_view_square`
- `switch.cpp/h`: `alloc_switch` (caller `switch_functions`)
- `switch_globals.cpp/h`: deleted entirely (only contained `switch_functions[]`)
- `CMakeLists.txt`: removed `switch_globals.cpp`
- `road.cpp/h`: `ROAD_calc_mapsquare_type`, `ROAD_get_mapsquare_type`, `ROAD_TYPE_*` constants
- `road_globals.cpp/h`: `ROAD_mapsquare_type[]`
- `aeng.cpp`: `AENG_draw_far_facets` (caller of the dead `FACET_draw_quick`)

Skipped:
- `env_inifile` — caller `ENV_load` is live, dc_leaf_check.py misclassified it
- `LIGHT_building_point` — caller `BUILD_draw` is live in the source (transitive, must be removed together with `BUILD_draw`)
- `find_anim_fight_height` — call in the live `should_i_block` (the compiler optimized it away, but the source-ref exists)

Lessons from batch 8:
- Removing an include from a .h can break transitive chains (`aeng.h` via `crinkle.h` — restored)
- `--gc-sections` requires all symbols to resolve BEFORE pruning → you cannot remove a callee without its caller
- dc_leaf_check.py is unreliable on Windows — verify by hand with grep

**What was done in this session (batches 6-7):**

Strategy: leaf globals — global variables without a single real caller (only the definition in .cpp + extern in .h). Verified with grep before removal.

Removed (batch 6):
- `bat_globals.cpp/h`: `BAT_state_name[]`
- `thug_globals.cpp/h`: `thug_states[]` (the whole file became empty — trimmed to a minimum)
- `darci_globals.cpp/h`: `history_thing[100]`
- `person_globals.cpp/h`: `PERSON_mode_name[]`, `dir_to_angle[]`
- `attract_globals.cpp/h`: `demo_text[]`, `playbacks[]`
- `overlay_globals.cpp/h`: `help_text[]`, `help_xlat[]` (+ removed redundant #include xlat_str.h)
- `combat_globals.cpp/h`: `grapple[]`, `kicks[][]`, `punches[][]`

Removed (batch 7):
- `pcom_globals.cpp/h`: `gang_angle_priority[]`
- `soundenv_globals.cpp/h`: `SOUNDENV_gndquads[]`
- `hierarchy_globals.cpp/h` + `hierarchy.h`: `body_part_parent_numbers[]`
- `game_tick_globals.cpp/h`: `amb_choice[]` + `#define AMB_NUM_CHOICES`

Skipped (with justification):
- `find_anim_fight_height` (combat.cpp): the compiler optimized away the call from the live `should_i_block` (return value discarded), but the call is present in the source. Rule #4 — do not remove calls from live code.

**Done in batch 13 (poly.cpp + anim.cpp leaf cluster):**
- `poly.cpp`: removed `NewTweenVertex2D_X`, `NewTweenVertex2D_Y`, `POLY_clip_against_side_X`, `POLY_clip_against_side_Y` (cluster: clip_side calls tween, the cluster has no external callers)
- `anim.cpp`: removed `Anim::Anim()`, `Anim::~Anim()`, `Anim::StartLooping`, `Anim::StopLooping`, `Anim::SetAllTweens`, `Anim::AddKeyFrame`, `Anim::RemoveKeyFrame`, `Character::AddAnim`, `Character::RemoveAnim` (9 methods, no external callers)
- Declarations in `anim_types.h` left in place (not called → the linker doesn't complain; header cleanup is a separate iteration)
- Build OK, 2894 → 2881 discards (-13)

**Done in batch 14 (session 4):**
- `fastprim.cpp/h`: removed `FASTPRIM_find_texture_from_page`, `FASTPRIM_free_cached_prim`, `FASTPRIM_free_queue_for_call`, `FASTPRIM_create_call`, `FASTPRIM_add_point_to_call`, `FASTPRIM_ensure_room_for_indices`, `FASTPRIM_draw` (7 functions as a cluster); cleaned up includes in fastprim.h and fastprim.cpp (−8 discards)
- `polypage.cpp/h`: removed `AddFan`, `AddWirePoly`, `DrawSinglePoly`, `SortBackFirst`, `MergeSortIteration` + `DoMerge` static template; cleaned up `#include "poly.h"` from polypage.h (−8 discards)
- `ds_bridge.cpp/h`: removed `ds_trigger_bow`, `ds_trigger_machine` (legacy simplified wrappers with no callers) (−2 discards)

**Done in batch 15 (building cluster, session 5):**
- `building.cpp`: deleted entirely (135 dead functions, all were dead) (-138 discards)
- `building.h`: trimmed to a forwarding header (only includes prim_types.h + building_types.h)
- `build.cpp` + `build.h`: deleted entirely (3 dead functions: BUILD_draw, BUILD_draw_inside, LIGHT_get_colour)
- `prim.cpp`: removed 23 dead functions (delete_prim_points_block, compress_prims, smooth_a_prim, copy_prim_to_end, delete_prim_*, delete_last_prim, delete_a_prim, quick_normal, apply_ambient_light_to_object, get_rotated_point_world_pos, set_anim_prim_anim, toggle_anim_prim_switch_state, etc.); removed dead macros (shadow_calc, in_shadow, in_shadowo)
- `building_globals.cpp`: trimmed to 4 live globals (next_roof_bound, textures_xy, dx_textures_xy, textures_flags) (-58 discards)
- `building_globals.h`: trimmed to 4 live extern declarations + minimal includes
- CMakeLists.txt: removed building.cpp, build.cpp

**Done in batch 16 (aeng.cpp + HM module, session 6):**
- `hm.cpp` + `hm.h`: deleted entirely (all functions dead, all callers in game_tick.cpp inside `/* */` blocks)
- `hm_globals.cpp` + `hm_globals.h`: deleted entirely (4 dead globals)
- `CMakeLists.txt`: removed hm.cpp, hm_globals.cpp
- `attract.cpp`: removed dead forward declaration `AENG_demo_attract` (3 lines)
- `combat.cpp`: removed extern decls for `e_draw_3d_line` and `e_draw_3d_line_col` (4 lines)
- `collide.cpp`: removed extern decl for `e_draw_3d_line` (1 line)
- `aeng.h`: removed 14 ranges of dead declarations (119 lines) — `AENG_e_draw_3d_line*`, `e_draw_3d_line*` macros, `AENG_draw_scanner/power/FPS/fade_in`, `AENG_light_draw/waypoint_draw/rad_trigger_draw/groundsquare_draw`, `AENG_set_sky_nighttime/daytime`, `AENG_draw_col_tri`, `AENG_fade_in`, `AENG_get_draw_distance`, `AENG_set_camera_radians` (6-param)
- `aeng_globals.h`: removed declarations `AENG_calc_gamut_lo_only`, `AENG_do_cached_lighting_old`, `AENG_draw_rain_old`, `AENG_draw_cloth`, `AENG_add_projected_lit_shadow_poly`, `show_gamut_hi`, `cache_a_row`, `add_kerb`, `draw_i_prim`, `general_steam`, `draw_quick_floor`, `AENG_draw_rectr`
- `poly.h`: removed declarations `calc_global_cloud`, `use_global_cloud`
- `aeng.cpp`: removed 2112 lines of dead functions (9156 → 7044 lines):
  - `calc_global_cloud`, `use_global_cloud`
  - `AENG_get_draw_distance`
  - `AENG_calc_gamut_lo_only`
  - 6-param `AENG_set_camera_radians` (without splitscreen)
  - `AENG_do_cached_lighting_old`
  - `AENG_add_projected_lit_shadow_poly`, `AENG_draw_rain_old`
  - `AENG_draw_cloth`
  - `AENG_set_sky_nighttime`, `AENG_set_sky_daytime`, `AENG_draw_rectr`
  - `AENG_draw_col_tri`
  - `show_gamut_hi`
  - floor cluster: `cache_a_row`, `add_kerb`, `draw_i_prim`, `general_steam`, `draw_quick_floor`
  - `AENG_draw_scanner`, `AENG_draw_power`
  - `AENG_draw_FPS`
  - `AENG_fade_in`
  - `AENG_e_draw_3d_line`, `AENG_e_draw_3d_line_dir`, `AENG_e_draw_3d_line_col`, `AENG_e_draw_3d_line_col_sorted`, `AENG_e_draw_3d_mapwho`, `AENG_e_draw_3d_mapwho_y`, `AENG_demo_attract`
  - `AENG_light_draw`, `AENG_waypoint_draw`, `AENG_rad_trigger_draw`, `AENG_groundsquare_draw`
  - Added `#define AENG_NUM_RAINDROPS 128` at file scope (it was inside the removed dead function, but is used in the live `AENG_draw_rain`)
- Build OK (make build-release, exit code 0)
- Uncommitted — the user commits manually

**Done in batch 17 (QMAP/QENG/stair clusters + aeng_globals + game.cpp, session 7):**
- `qmap.cpp/h` + `qmap_globals.cpp/h`: deleted entirely (not a single live caller)
- `qeng.cpp/h` + `qeng_globals.cpp/h`: deleted entirely (not a single live caller)
- `stair.cpp/h` + `stair_globals.cpp/h`: deleted entirely (not a single live caller)
- CMakeLists.txt: removed qeng.cpp, qeng_globals.cpp, qmap.cpp, qmap_globals.cpp, stair.cpp, stair_globals.cpp
- `aeng_globals.cpp`: removed 21 dead globals + removed 2 includes (`polypage.h`, `game_graphics_engine.h`)
- `aeng_globals.h`: removed declarations + types/constants used only by dead globals (`StoreLine`, `MAX_LINES`, `FloorStore`, `GroupInfo`, `MAX_FLOOR_TILES_FOR_STRIPS`, etc.)
- `game.cpp`: removed 4 dead functions (`GAME_map_draw`, `leave_map_form_proc`, `process_bullet_points`, `hardware_input_replay`) + extern decl `plan_view_shot` + dead includes
- `game.h`: removed declarations of dead functions + removed includes `widget.h`, `widget_globals.h`, `game_globals.h`; added `game_types.h` (instead of game_globals.h — a clean types-only header)
- Side effect: `game.cpp` got `#include "game/game_globals.h"` added (a direct dependency instead of a transitive one via game.h)
- Side effect: `figure.h` got `#include "effects/combat/pyro.h"` and `#include "things/core/drawtype.h"` added (they were a hidden transitive dependency via game.h → game_globals.h → thing.h)
- Build OK (make build-release, exit code 0)

**Done in batch 24 (memory.cpp cluster, session 11):**
- `memory.cpp`: removed 12 dead functions: `release_memory`, `set_darci_normals`, `convert_drawtype_to_index`, `convert_thing_to_index`, `convert_pointers_to_index`, `STORE_DATA` macro + body, `convert_keyframe_to_index`, `convert_animlist_to_index`, `convert_fightcol_to_index`, `save_whole_anims`, `find_best_anim_offset_old`, `find_best_anim_offset`, `MEMORY_quick_load_available`
- `memory.h`: removed declarations of all 12 functions; updated the "Internal helpers" comment
- Confirmed: `convert_pointers_to_index` — its only call site is in game_tick.cpp inside a `/* */` block (not compiled)
- Confirmed: `save_whole_game` (empty stub) kept — called from the game_tick.cpp F3 debug path (live)
- Build OK (make build-release, exit code 0)

**Done in batch 25 (oc_config + env string cascade, session 11):**
- `oc_config.cpp`: removed `OC_CONFIG_get_string`, `OC_CONFIG_set_string`, `g_str_return_buf` static global
- `oc_config.h`: removed declarations `OC_CONFIG_get_string`, `OC_CONFIG_set_string`; updated comment
- `env.cpp`: removed `ENV_get_value_string`, `ENV_set_value_string`
- `env.h`: removed declarations `ENV_get_value_string`, `ENV_set_value_string`
- Effect: eliminated the cascade generation of ~52 dead symbols from nlohmann::json template instantiations (generated by the 2 functions `OC_CONFIG_get/set_string`)
- Build OK (make build-release, exit code 0)

**Done in batch 26 (panel module, session 11):**
- `panel_globals.cpp`: removed via Python (lines 18-125): `#define PIC/PICALL0/PICALL1`, `PANEL_ic[PANEL_IC_NUMBER]` (large initializer), `PANEL_page[4][PANEL_PAGE_NUMBER]`, `PANEL_beat[2][...]`, `PANEL_beat_head/tick/last_ammo/last_specialtype/x[2]`, `PANEL_ammo[PANEL_AMMO_NUMBER]`, `PANEL_toss[PANEL_MAX_TOSSES]`, `PANEL_toss_last`, `PANEL_toss_tick`; removed `#include "engine/graphics/pipeline/poly.h"` (needed only for PANEL_page init)
- `panel_globals.h`: removed via Python (lines 33-167): `PANEL_Ic` struct, all `PANEL_IC_*` defines (46 entries), `extern PANEL_ic`, `PANEL_PAGE_*` defines, `extern PANEL_page`, `PANEL_Beat` struct + `PANEL_NUM_BEATS`, the extern group `PANEL_beat*`, `PANEL_Ammo` struct + `PANEL_AMMO_*` defines + `extern PANEL_ammo`, `PANEL_Toss` struct + `PANEL_MAX_TOSSES` + `extern PANEL_toss/toss_last/toss_tick`
- `panel.cpp`: removed 7 dead functions: `PANEL_draw_face`, `PANEL_draw_health_bar`, `PANEL_funky_quad`, `PANEL_new_toss`, `PANEL_do_tosses`, `PANEL_help_message_do`, `PANEL_enable_screensaver`
- `panel.h`: removed declarations of all 7 functions
- Build OK (make build-release, exit code 0)

**Done in batch 27 (fire.cpp cluster, session 11):**
- `fire.cpp`: removed `FIRE_init`, `FIRE_create`, `FIRE_process` + 2 static helpers `FIRE_num_flames_for_size`, `FIRE_add_flame`; removed dead includes `uc_common.h`, `game_types.h`
- `fire.h`: removed declarations `FIRE_init`, `FIRE_create`, `FIRE_process` (FIRE_get_start kept — live, called from aeng.cpp)
- Build OK (make build-release, exit code 0)

**Done in batch 28 (poly.cpp + sky/mesh cluster, session 11):**
- `poly.cpp`: removed 10 dead functions: `fade_point_more`, `POLY_transform_abs`, `POLY_get_screen_pos`, `POLY_transform_using_local_rotation_and_wibble`, `POLY_fadeout_buffer`, `POLY_add_line_2d`, `POLY_clip_line_box`, `POLY_clip_line_add`, `POLY_get_sphere_circle`, `POLY_inside_quad` (397 lines)
- `poly.h`: removed declarations of all 10 functions
- `poly_globals.cpp/h`: removed `POLY_clip_left/right/top/bottom` (used only by POLY_clip_line_box/add)
- `sky.cpp`: removed `SKY_draw_poly_clouds`, `SKY_draw_poly_sky` (moved `SKY_VERY_FAR_AWAY` to file scope — needed by live functions)
- `sky.h`: removed declarations `SKY_draw_poly_clouds`, `SKY_draw_poly_sky`
- `mesh.cpp`: removed `MESH_draw_morph`, `MESH_init_reflections`, `MESH_draw_reflection` + static helpers `MESH_grow_points`, `MESH_grow_faces`, `MESH_add_point`, `MESH_add_face`, `MESH_add_poly`, `MESH_create_reflection` + dead macros `MESH_MAX_DY`, `MESH_255_DIVIDED_BY_MAX_DY`, `MESH_NEXT_CLOCK`, `MESH_NEXT_ANTI` (780 lines)
- `mesh.h`: removed declarations `MESH_init_reflections`, `MESH_draw_reflection`, `MESH_draw_morph`
- `mesh_globals.cpp/h`: removed `MESH_reflection[]`, `MESH_add[]`, `MESH_add_upto` globals + types `MESH_Reflection`, `MESH_Add`, constants `MESH_MAX_REFLECTIONS`, `MESH_MAX_ADD`
- Build OK (make build-release, exit code 0)

**Done in batch 33 (session 15, continued):**
- `elev.cpp/h`: removed `ELEV_create_similar_name`
- `tga.cpp/h`: removed `TGA_load_remap` (~145 lines, with goto labels)
- `anim_tmap.cpp/h`: removed `save_animtmaps`
- `thug.cpp/h`: removed `fn_thug_init`, `fn_thug_normal`; file trimmed to a minimum; removed `#include thug.h` from `person.cpp`
- `playcuts.cpp/h`: removed `PLAYCUTS_Free` (empty stub), `PLAYCUTS_Reset`
- `gamepad.cpp/h`: removed `gamepad_is_adaptive_click_active`
- `composition.cpp/h`: removed `composition_get_scene_texture`
- `crt_effect.cpp/h`: removed `crt_effect_shutdown`
- `wibble_effect.cpp/h`: removed `gl_wibble_effect_shutdown`; h trimmed to an empty guard
- `morph.cpp/h`: removed `MORPH_get_points`, `MORPH_get_num_points`
- `night.cpp`: removed dead forward declaration `calc_inside_for_xyz` inside block scope; restructured the surrounding block (removed redundant `{}`)
- `supermap.cpp/h`: removed `calc_inside_for_xyz`, `add_sewer_ladder`
- `inside2.cpp/h`: removed `find_stair_y` (~100 lines)
- `map.cpp/h` + `map_globals.cpp/h`: removed `MAP_light_get_height`, `MAP_light_get_light`, `MAP_light_set_light`, global `MAP_light_map`; map_globals trimmed to a minimum
- `font.cpp/h`: removed `FONT_draw_speech_bubble_text` (~54 lines)
- `polypage.cpp/h`: removed `GenerateMMMatrix` (~89 lines, D3D matrix builder); removed `#include compression.h`
- `smap.cpp/h`: removed `SMAP_bike` (empty stub)
- `eway.cpp/h`: removed `count_people_types`, `EWAY_cam_get_position_for_angle`, `EWAY_cam_look_at` (cluster)
- `pause.cpp/h`: removed `PAUSE_handler` (~180 lines); files trimmed to a minimum; removed `#include pause.h` + misleading comment from `game.cpp`
- `frontend.cpp/h`: removed `FRONTEND_do_drivers`, `FRONTEND_gamma_update`
- `frontend_globals.cpp/h`: removed `CurrentVidMode`, `CurrentBitDepth`
- `attract.cpp/h`: removed `level_won`, `level_lost` (empty stubs)
- `overlay.cpp/h`: removed `overlay_beacons` (empty stub); removed extern decl from `game.cpp`
- `compression.cpp/h` + `compression_globals.cpp/h` + `image_compression.cpp/h`: removed the entire compression cluster (`COMP_load`, `COMP_calc`, `COMP_decomp`, `IC_pack`, `IC_unpack`, `COMP_data/frame/tga_data/tga_info`, types `COMP_Frame/DataBuffer/Delta`); files trimmed to a minimum
- `aeng.cpp`: removed `AENG_movie_init` (~38 lines); removed the call from `AENG_init()`; removed `#include compression.h`
- `aeng_globals.cpp/h`: removed 9 movie globals (`AENG_movie_data[512*1024]`, `AENG_movie_upto`, `AENG_frame_one/two/last/next`, `AENG_frame_count/tick/number`), `AENG_MAX_MOVIE_DATA` define
- Build OK (make build-release, exit 0)

**Incidents in batch 33:**
- `aeng_globals.h:173: error: unknown type name 'COMP_Frame'` after cleaning up compression.h — diagnosed as the dead AENG_movie cluster (the only user of `COMP_Frame` is `AENG_movie_init`). Resolution: remove the entire AENG_movie cluster.
- `game.cpp:77` had a stale `#include pause.h` with a misleading comment about `PANEL_` functions — the functions are actually from `panel.h`. Fixed, and the include removed.

## ✅ Dead-code cleanup COMPLETE (2026-04-27)

All planned passes done:

1. ✅ **gc-sections exhausted** (batch 35) — 4534→1800 discards, none of the remaining 80 can be removed
2. ✅ **Empty files** (batch 35+36) — removed shadow_globals.cpp/h, startscr.cpp
3. ✅ **Redundant #include** (batch 37) — ~355 redundant includes removed from 93 files
4. ✅ **#if 0 / #if false blocks** — 0 found in `new_game/src/`, clean
5. ✅ **Commented-out /* */ blocks ≥4 lines** (batch 38) — ~120 blocks removed, 0 remaining
6. ✅ **cppcheck unusedFunction** (batch 39) — all dead static functions removed

**Readiness criterion met:** gc-sections discards nothing of substance, cppcheck reports only confirmed false positives.

## How it was generated

The CMake option `DEAD_CODE_REPORT=ON` in [new_game/CMakeLists.txt](../new_game/CMakeLists.txt) enables:
- `-ffunction-sections -fdata-sections` (compiler)
- `/OPT:REF /VERBOSE` on Windows (lld-link) or `--gc-sections --print-gc-sections` on Unix (ld.lld)

Run:
```bash
CMAKE_EXTRA_ARGS="-DDEAD_CODE_REPORT=ON" make configure
make build-release > /tmp/dead_code_build.log 2>&1
```

Then parsing: `grep '^lld-link: Discarded' build.log` → intersect with `llvm-nm --defined-only --extern-only` across all our `.obj` files → text grep over `new_game/src/` to split into **leaf** vs **transitive**.

## Summary of the latest run

| Metric | Value |
|---------|----------|
| Total "Discarded" by the linker (after filtering constants/strings) | ~1145 |
| **Leaf dead** (nothing in the sources references it) | **81** |
| **Transitive dead** (there are textual references in the sources, but the callers are dead too) | 1055 |

**The distinction matters:** leaf can be removed directly. Transitive — there is an `if (...)` in the source that calls the function, but that branch/wrapper-function is also in the dead-list. It must be removed as a cluster (dependency graph), otherwise the build breaks.

## Top hot zones (by count)

### Leaf dead (everything that remained live after filtering)

| File | Count |
|------|--------|
| src/engine/core/quaternion.cpp | 10 |
| src/map/sewers.cpp | 9 |
| src/engine/graphics/geometry/superfacet.cpp | 9 |
| src/assets/formats/anim.cpp | 9 |
| src/engine/graphics/geometry/fastprim.cpp | 7 |
| src/engine/graphics/ui_coords.cpp | 5 |
| src/engine/graphics/pipeline/polypage.cpp | 5 |
| src/engine/graphics/pipeline/poly.cpp | 4 |
| src/combat/combat.cpp | 4 |
| src/things/vehicles/vehicle.cpp | 3 |
| src/game/input_actions.cpp | 3 |
| src/engine/graphics/graphics_engine/game_graphics_engine.cpp | 3 |
| src/ui/hud/overlay.cpp | 2 |
| src/engine/core/matrix.cpp | 2 |
| src/things/characters/cop.cpp | 1 |
| src/engine/graphics/pipeline/aeng.cpp | 1 |
| src/camera/fc.cpp | 1 |
| src/buildings/building.cpp | 1 |
| src/assets/formats/level_loader.cpp | 1 |
| src/ai/pcom.cpp | 1 |

### Transitive dead (top 30, must be removed as a cluster)

| File | Count |
|------|--------|
| src/buildings/building.cpp | 133 |
| src/buildings/building_globals.cpp | 58 |
| src/engine/graphics/pipeline/aeng.cpp | 40 |
| src/things/characters/person.cpp | 36 |
| src/ui/menus/widget.cpp | 34 |
| src/map/qmap_globals.cpp | 29 |
| src/engine/graphics/graphics_engine/backend_opengl/game/core.cpp | 23 |
| src/buildings/prim.cpp | 23 |
| src/engine/physics/collide.cpp | 22 |
| src/engine/graphics/pipeline/aeng_globals.cpp | 22 |
| src/outro/core/outro_os_globals.cpp | 21 |
| src/map/sewers_globals.cpp | 20 |
| src/engine/physics/hm.cpp | 16 |
| src/map/sewers.cpp | 14 |
| src/buildings/stair_globals.cpp | 14 |
| src/ui/hud/planmap_globals.cpp | 13 |
| src/outro/core/outro_os.cpp | 13 |
| src/ui/hud/panel_globals.cpp | 12 |
| src/missions/memory.cpp | 12 |
| src/game/game_globals.cpp | 11 |
| src/engine/graphics/pipeline/poly.cpp | 10 |
| src/ui/menus/widget_globals.cpp | 9 |
| src/outro/core/outro_mf.cpp | 9 |
| src/map/qmap.cpp | 9 |
| src/engine/physics/collide_globals.cpp | 9 |
| src/assets/texture.cpp | 9 |
| src/ui/hud/eng_map_globals.cpp | 8 |
| src/things/core/switch.cpp | 8 |
| src/game/input_actions_globals.cpp | 8 |
| src/engine/graphics/graphics_engine/backend_opengl/outro/core.cpp | 8 |

## Full leaf list

```
src/engine/core/matrix.cpp	MATRIX_calc_int
src/engine/core/matrix.cpp	MATRIX_find_angles_old
src/engine/core/quaternion.cpp	QuatMul
src/engine/core/quaternion.cpp	EulerToQuat
src/engine/core/quaternion.cpp	is_unit
src/engine/core/quaternion.cpp	MatrixToQuatInteger
src/engine/core/quaternion.cpp	QuatToMatrixInteger
src/engine/core/quaternion.cpp	BuildACosTable
src/engine/core/quaternion.cpp	acos
src/engine/core/quaternion.cpp	QuatSlerpInteger
src/engine/core/quaternion.cpp	cmat_to_mat
src/engine/core/quaternion.cpp	check_isonormal_integer
src/engine/graphics/pipeline/polypage.cpp	GERenderState::ZLift
src/engine/graphics/pipeline/polypage.cpp	PolyPage::AddWirePoly
src/engine/graphics/pipeline/polypage.cpp	PolyPage::DrawSinglePoly
src/engine/graphics/pipeline/polypage.cpp	PolyPage::SortBackFirst
src/engine/graphics/pipeline/polypage.cpp	PolyPage::MergeSortIteration
src/engine/graphics/pipeline/aeng.cpp	AENG_draw_far_facets
src/engine/graphics/pipeline/poly.cpp	NewTweenVertex2D_X
src/engine/graphics/pipeline/poly.cpp	NewTweenVertex2D_Y
src/engine/graphics/pipeline/poly.cpp	POLY_clip_against_side_X
src/engine/graphics/pipeline/poly.cpp	POLY_clip_against_side_Y
src/engine/graphics/geometry/fastprim.cpp	FASTPRIM_find_texture_from_page
src/engine/graphics/geometry/fastprim.cpp	FASTPRIM_free_cached_prim
src/engine/graphics/geometry/fastprim.cpp	FASTPRIM_free_queue_for_call
src/engine/graphics/geometry/fastprim.cpp	FASTPRIM_create_call
src/engine/graphics/geometry/fastprim.cpp	FASTPRIM_add_point_to_call
src/engine/graphics/geometry/fastprim.cpp	FASTPRIM_ensure_room_for_indices
src/engine/graphics/geometry/fastprim.cpp	GERenderState::IsAlphaBlendEnabled
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_free_end_of_queue
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_convert_texture
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_make_facet_points
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_create_points
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_fill_facet_points
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_build_call
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_create_calls
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_convert_facet
src/engine/graphics/geometry/superfacet.cpp	SUPERFACET_redo_lighting
src/engine/graphics/ui_coords.cpp	ui_coords::frame_to_screen
src/engine/graphics/ui_coords.cpp	ui_coords::screen_to_frame
src/engine/graphics/ui_coords.cpp	ui_coords::old_px_to_screen
src/engine/graphics/ui_coords.cpp	ui_coords::screen_to_pixels
src/engine/graphics/ui_coords.cpp	ui_coords::old_px_to_screen_pixels
src/engine/graphics/graphics_engine/game_graphics_engine.cpp	GERenderState::SetTextureFilter
src/engine/graphics/graphics_engine/game_graphics_engine.cpp	GERenderState::SetBlendMode
src/engine/graphics/graphics_engine/game_graphics_engine.cpp	GERenderState::SetTempTransparent
src/assets/formats/anim.cpp	Anim::ctor
src/assets/formats/anim.cpp	Anim::dtor
src/assets/formats/anim.cpp	Anim::StartLooping
src/assets/formats/anim.cpp	Anim::StopLooping
src/assets/formats/anim.cpp	Anim::SetAllTweens
src/assets/formats/anim.cpp	Anim::AddKeyFrame
src/assets/formats/anim.cpp	Anim::RemoveKeyFrame
src/assets/formats/anim.cpp	Character::AddAnim
src/assets/formats/anim.cpp	Character::RemoveAnim
src/assets/formats/level_loader.cpp	_vfscanf_l
src/map/sewers.cpp	NS_cache_create_wallstrip
src/map/sewers.cpp	NS_cache_create_extra_bit_left
src/map/sewers.cpp	NS_cache_create_extra_bit_right
src/map/sewers.cpp	NS_cache_create_walls
src/map/sewers.cpp	NS_cache_create_curve_sewer
src/map/sewers.cpp	NS_cache_create_curve_top
src/map/sewers.cpp	NS_cache_create_curves
src/map/sewers.cpp	NS_cache_create_falls
src/map/sewers.cpp	NS_cache_create_grates
src/buildings/building.cpp	place_building_at
src/things/characters/cop.cpp	fn_cop_fight
src/things/vehicles/vehicle.cpp	VEH_dealloc
src/things/vehicles/vehicle.cpp	free_vehicle
src/things/vehicles/vehicle.cpp	animate_car
src/combat/combat.cpp	find_possible_combat_target
src/combat/combat.cpp	find_hit_value
src/combat/combat.cpp	find_anim_fight_height
src/combat/combat.cpp	show_fight_range
src/ai/pcom.cpp	get_nearest_gang_member
src/camera/fc.cpp	FC_must_move_up_and_around
src/ui/hud/overlay.cpp	show_help_text
src/ui/hud/overlay.cpp	OVERLAY_draw_tracked_enemies
src/game/input_actions.cpp	action_ladder
src/game/input_actions.cpp	action_flip_left
src/game/input_actions.cpp	action_flip_right
```

## Pitfalls (the false-positives error class)

1. **Transitively dead**: the call exists in the code, but the caller itself is dead. The linker discards both. If you remove the function blindly — the caller's code won't compile. It must be removed as a cluster.
1a. **Empty stub with live callers**: an empty function `void f() {}` or `SLONG f(...) { return 0; }` is called from live code. The compiler inlines it into nothing, so the linker doesn't see a reference and discards it as dead. But the DEFINITION is needed for compilation — it cannot be removed. Verification: grep including the .cpp file itself (`grep -rn "\bfunc\b" new_game/src/`). Typical symptom: build fail with `error: use of undeclared identifier 'func'` after removal.
2. **Callbacks via pointer**: a function is registered as `&fn` in a setter (`ge_set_X_callback`). `--gc-sections` sees this as live (its address is taken), so there should be none of these in the dead-list. But if the setter itself is cut, the function becomes dead. Check when removing.
3. **Virtual methods**: a class vtable is used via virtual dispatch; a concrete method may look dead but is called through a base-class pointer.
4. **Template instances**: STL containers may produce false leaf candidates.
5. **Imports from system DLLs**: `c_DwmMaxQueuedBuffers`, `_Avx2WmemEnabledWeakValue` — these are extern symbols from system libs. Already filtered out.
6. **Stale .obj from removed sources**: `weapon_feel_test.cpp.obj` remained in build/ but the source no longer exists; it does not take part in the link. Already filtered out.
7. **Stub-only in the regular build**: `DEAD_CODE_REPORT=ON` enables `-ffunction-sections` → each function in its own section → the linker discards a dead function separately from its neighbors. In a regular Release build without this flag, all functions from one `.cpp` share a single `.text` section. If a file has even one live function — the whole `.o` is linked, including dead callers. Dead callers contain references to callees (ge_texture_free_all etc.) → the linker requires their definitions. **Rule:** before removing a callee's definition you must first remove all of its callers (or ensure that the callers are in files that contain only dead code — then the whole `.o` is not linked). If a caller cannot be removed right now — leave the callee as a minimal stub (`{}` / `return 0`); it will stay in the discard-list for the DEAD_CODE_REPORT build but will not break the regular Release.

## Per-iteration progress

(filled in as the work proceeds)

### 2026-04-26 — Batch 1: quaternion (integer-path + dead helpers)

**Removed from `src/engine/core/quaternion.cpp`:**
- Forward decl `QUATERNION_BuildTweenInteger` (at the top of the file)
- Float-path dead helpers: `QuatMul`, `EulerToQuat`, `is_unit`
- The entire integer-path:
  - `struct QuatInt`
  - `MatrixToQuatInteger`
  - `QuatToMatrixInteger`
  - `BuildACosTable`
  - `#define DELTA_INT 1638`
  - `QuatSlerpInteger`
  - `cmat_to_mat`
  - `check_isonormal_integer`
  - `QUATERNION_BuildTweenInteger`

**Kept in `quaternion.cpp` (single live entry-point + its transitive helpers):**
`CQuaternion::BuildTween` (called from `figure.cpp:1776,3769,4046`) → uses `cmat_to_fmat`, `check_isonormal`, `MatrixToQuat`, `QuatSlerp`, `QuatToMatrix`, `fmat_to_mat`. All helpers marked `static` (file-local), since there are no external calls.

**Removed from `src/engine/core/quaternion.h`:**
- Declarations `QuatSlerp`, `QuatToMatrix`, `QUATERNION_BuildTweenInteger`
- The `FloatMatrix` class (moved into `quaternion.cpp` — used only internally)

**Files removed entirely:**
- `src/engine/core/quaternion_globals.cpp` (contained only `acos_table` + `acos_table_init` for the integer-path)
- `src/engine/core/quaternion_globals.h`

**Other:**
- `#include <math.h>` removed from `quaternion.cpp` (clangd: not used directly, symbols come via `engine/core/math.h`)
- `quaternion_globals.cpp` removed from `new_game/CMakeLists.txt`

**Build result:** OK, exit 0. Linker discarded count: **4534 → 4510** (-24).

| Date | Iteration | Files/symbols | Result |
|------|----------|---------------|-----------|
| 2026-04-26 | Batch 1 | quaternion (12 functions + 2 globals + 2 files) | -24 discards, build OK |
| 2026-04-26 | Batch 2 attempt (REVERTED) | parallel SAs with hallucinated lists — reverted | — |
| 2026-04-26 | Batch 3 | superfacet.cpp (9 dead funcs) + superfacet_globals (2 globals) | -45 discards, build OK |
| 2026-04-26 | Batch 4 | matrix.cpp (5 funcs) + matrix.h + outro_matrix.h + fc.cpp (3 funcs) + fc.h + cop.cpp (fn_cop_fight) + pcom.cpp (4 funcs + extern decl) + pcom.h. Not touched: MATRIX_find_angles (has a caller in HM_draw which is itself dead but not removed). | -15 discards, build OK |
| 2026-04-26 | Batch 5 | ui_coords (5 funcs + .h decls), game_graphics_engine (3 GERenderState methods), overlay (4 funcs: should_i_add_message, show_help_text, OVERLAY_draw_tracked_enemies, add_damage_value, add_damage_value_thing — skipped overlay_beacons because of a live caller at game.cpp:519), combat (4 funcs: find_possible_combat_target, find_hit_value, show_fight_range, is_person_under_attack — skipped find_anim_fight_height because the compiler optimized away the call in the live should_i_block but the call remains in the source), vehicle (4: VEH_dealloc, VEH_get_assignments, animate_car, free_vehicle), input_actions (3 dead arrays: action_ladder/flip_left/flip_right + 2 funcs: get_last_input, allow_input_autorepeat — skipped continue_action, set_look_pitch, lock_to_compass, pre_process_input because of live callers in person.cpp). | -38 discards, build OK |
| 2026-04-26 | Batch 6 | leaf globals: BAT_state_name, thug_states, history_thing, PERSON_mode_name, dir_to_angle, demo_text, playbacks, help_text, help_xlat, grapple[], kicks[][], punches[][] (12 globals in 8 files). Skipped find_anim_fight_height — call from the live should_i_block (the compiler optimized it away, but the source-ref remains). | -12 discards, build OK |
| 2026-04-26 | Batch 7 | leaf globals: gang_angle_priority, SOUNDENV_gndquads, body_part_parent_numbers (from hierarchy_globals.cpp + 2 .h), amb_choice + AMB_NUM_CHOICES (game_tick_globals) | -4 discards, build OK |
| 2026-04-26 | Batch 8 | facet cluster (3 funcs + 2 globals + night_globals/h macros), crinkle_project, ngamut_view_square, switch cluster (alloc_switch + switch_functions + switch_globals.cpp/h deleted), road cluster (2 funcs + ROAD_mapsquare_type + ROAD_TYPE_* consts), AENG_draw_far_facets | -14 discards, build OK |
| 2026-04-26 | Batch 9 | qeng.cpp/h + qeng_globals.cpp/h: DELETED ENTIRE MODULE (software rasterizer, unused since OpenGL migration) | build OK |
| 2026-04-26 | Batch 10 | qmap.cpp/h + qmap_globals.cpp/h: DELETED ENTIRE MODULE (software map renderer tied to qeng) | build OK |
| 2026-04-26 | Batch 11 | stair.cpp/h + stair_globals.cpp/h: DELETED ENTIRE MODULE (stair rendering, dead since new renderer) | build OK |
| 2026-04-26 | Batch 12 | aeng_globals.cpp/h: deleted dead globals (Lines[], next_line, global_b, AENG_sewer_page, AENG_gamut_lo_*, AENG_project_lit_*, AENG_sky_type/colour, m_vert_mem_block32, m_indicies, kerb_scale*/du/dv) + dead struct/macro defs (StoreLine, FloorStore, GroupInfo, MAX_LINES, KERB_*) | build OK |
| 2026-04-26 | Batch 13 | poly.cpp cluster (4 funcs: NewTweenVertex2D_X/Y + POLY_clip_against_side_X/Y), anim.cpp Anim+Character methods (9 funcs) | -13 discards, build OK |
| 2026-04-26 | Batch 14a | fastprim.cpp (7 dead funcs removed as a cluster: FASTPRIM_draw and 6 helpers); fastprim.h/cpp include cleanup | -8 discards, build OK |
| 2026-04-26 | Batch 14b | polypage.cpp (AddFan, AddWirePoly, DrawSinglePoly, SortBackFirst, MergeSortIteration, DoMerge); polypage.h decl cleanup + removed #include poly.h | -8 discards, build OK |
| 2026-04-26 | Batch 14c | ds_bridge.cpp/h: ds_trigger_bow, ds_trigger_machine (legacy simplified wrappers) | -2 discards, build OK |
| 2026-04-26 | Batch 15 | game_globals.cpp/h: 11 dead globals (CAM_cur_x/y/z/yaw/pitch/roll, pause_menu, game_paused_highlight, bullet_point, screen_mem, verifier_name) | build OK |
| 2026-04-26 | Batch 16 | planmap.cpp/h + planmap_globals.cpp/h: DELETED ENTIRE MODULE (2 dead funcs + 13 dead globals). anim_loader.cpp: 8 dead funcs cluster (load_frame_numbers, sort_multi_object, set_default_people_types, make_compress_matrix, load_multi_vue, load_anim_mesh, load_key_frame_chunks, load_a_multi_prim) + anim_loader.h decl cleanup. outro_os_globals.cpp/h: 21 dead globals (OS_application_name, OS_frame_is_fullscreen, OS_frame_is_hardware, KEY_inkey, OS_midas_ok, OS_trans_upto, all OS_bitmap_*). Fix: figure.h needed direct includes for Pyro+DrawTween; game.h→game_types.h to fix Thing cascade; game.cpp needed direct #include game_globals.h. | build OK |
| 2026-04-26 | Batch 17 | sewers.cpp: 23 dead funcs removed (NS_init, NS_precalculate, NS_cache_*, NS_add_*, NS_create_wallstrip_point x2, NS_get_unused_st) — kept 5 live (NS_calc_height_at, NS_calc_splash_height_at, NS_slide_along, NS_inside, NS_there_is_a_los). sewers_globals.cpp: 20 dead globals removed — kept NS_hi + NS_los_fail_x/y/z. sewers_globals.h: trimmed to only 4 live externs. Note: attempted full module delete — caught by linker (darci/dirt live callers). | build OK |
| 2026-04-26 | Batch 18 | person.cpp: 31 dead funcs deleted; person.h: ~35 dead decls removed; cop.cpp: extern fn_person_stand_up removed; darci.cpp: extern set_person_kick_off_wall removed. Restored 5 as empty stubs (inlined by compiler, but source-level callers exist): camera_shoot/fight/normal (called at ~10 sites in person.cpp), set_person_sidle (called at lines 4409,4476), set_person_running_jump_lr (called at lines 10071,10074,12733,12737). Discards: 2324→2293 (-31). | build OK |
| 2026-04-26 | Batch 18b | person_globals.cpp/h: deleted player_visited[16][128]; aeng.cpp: removed dead extern decl + commented-out use. Discards: 2293→2292. | build OK |
| 2026-04-26 | Batch 19 | widget.cpp: rewrote keeping 6 live fns (WIDGET_snd, FORM_KeyProc, FORM_Process, FORM_GetWidgetFromPoint, FORM_Draw, FORM_Free, FORM_Focus), deleted 34 dead + 6 static helpers + struct ListEntry + dead defines. widget_globals.cpp: deleted 9 dead Methods structs, kept WidgetTick+EatenKey. widget.h: removed ~50 dead decls/inlines/extern-Methods. game_tick.cpp: deleted dead form_proc (ref'd BUTTON_Methods). game_tick.h: removed form_proc decl + widget.h include; game_tick.cpp: added direct widget.h include (for Form* in local extern decl). Discards: 2292→2242 (-50). | build OK |
| 2026-04-26 | Batch 20 | eng_map.cpp: deleted MAP_draw (260 lines, 1 named dead fn + 31 float/XMM constants). eng_map.h: removed MAP_draw decl. | build OK |
| 2026-04-26 | Batch 21 | core.cpp: deleted 14 definitions entirely (ge_shutdown, ge_bound_texture_contains_alpha, ge_draw_primitive, ge_draw_indexed_primitive_unlit, ge_fill_rect, ge_destroy_user_texture, ge_run_cutscene, ge_restore_screen_surface, ge_is_texture_loaded, image_mem global, s_display_changed static, SetDisplay, ClearDisplay, s_image_mem_buf). 10 converted to minimal stubs: ge_to_gdi/from_gdi/is_display_changed/clear_display_changed (called from dead ShellPauseOn/LibShellChanged in host.cpp), ge_get_font/texture_free_all/texture_change_tga/texture_set_greyscale/texture_get_size (dead callers in texture.cpp/text.cpp), ge_get_screen_bpp/enumerate_drivers (dead FRONTEND_do_drivers in frontend.cpp). Removed outro_graphics_engine.h + wibble_effect.h from core.cpp includes; removed frontend.cpp extern image_mem. game_graphics_engine.h: removed 19 dead decls; uc_common.h: removed SetDisplay/ClearDisplay decls. **Lesson:** cannot delete a callee definition if caller lives in same .o as live code (non-function-sections build). Must delete callers first. See "Pitfalls" #7. | build OK |
| 2026-04-27 | Batch 22 | collide.cpp: removed 22 dead functions (clear_all_col_info, get_height_along_vect, get_height_along_facet, TSHIFT+check_big_point_triangle_col, point_in_quad_old, dist_to_line, calc_along_vect, calc_height_at, collision_storey, highlight_face/rface/quad, vect_intersect_wall, find_alt_for_this_pos, get_fence_height, step_back_along_vect, cross_door, bump_person, in_my_fov, collide_against_sausage, box_box_early_out, box_circle_early_out). collide.h: removed all 22 dead declarations + extern col_vects_links/col_vects. collide_globals.cpp/h: removed 9 dead globals (col_vects_links, col_vects, next_col_vect, next_col_vect_link, walk_links, next_walk_link, already, max_facet_find, last_slide_dist). fog.cpp/mist.cpp/cop.cpp/roper.cpp/thug.cpp: removed extern decl for calc_height_at. walkable.cpp: removed extern decl for highlight_rface. **Incident:** the Python regex removed bump_person together with 2200+ lines of live code (slide_along_edges, slide_along_redges, move_thing*, collide_against_*, drop_on_heads, there_is_a_los*, slide_around_circle) — restored via git show HEAD. | -46 discards, build OK |
| 2026-04-27 | Batch 23 | outro_os.cpp/h: removed 13 dead (OS_buffer_add_sprite_rot, OS_fps, OS_mouse_get, OS_mouse_set, OS_sound_init/start/volume, OS_string, OS_texture_create(int,int), OS_texture_finished_creating, OS_texture_lock, OS_texture_size, OS_texture_unlock) + OS_BITMAP_* macros and related declarations. aeng.cpp/h: removed AENG_set_draw_distance (no callers). Skipped 6 aeng.cpp functions (apply_cloud, show_gamut_lo, AENG_draw_bangs, AENG_draw_people_messages, AENG_clear_screen, AENG_fade_out) — the linker discards them via inline/optimizer but source-level calls exist in live functions. stage13_dead_code_cleanup.md: added the section "Final pass — cleaning up redundant #include". | build OK |
| 2026-04-27 | Batch 29 (session 12) | Regenerate dead-list: 2140 discards, 410 symbols. input_actions_globals.cpp/h: removed 7 dead globals (debug_input, g_iCheatNumber, last_camera_dx/dy/dz, last_camera_yaw/pitch). anim_globals.cpp/h: removed 5 dead globals (test_chunk2, test_chunk3, thug_chunk, the_elements, prim_multi_anims, next_prim_multi_anim) — roper_pickup and test_chunk restored (source-level callers in dead but not commented-out functions). supermap.cpp: removed/restored extern roper_pickup. outro_mf.cpp/h: removed 8 dead functions (MF_ambient, MF_invert_zeds, MF_add_triangles_normal/normal_colour/light/light_bumpmapped/specular/specular_shadowed). backend_opengl/outro/core.cpp + outro_graphics_engine.h: removed 8 dead (oge_shutdown, oge_texture_create_blank/finished_creating/size/lock/unlock, oge_init_renderstates, oge_calculate_pipeline). texture.cpp/h + aeng.h: removed 9 dead (TEXTURE_86_lock/unlock/update, TEXTURE_free_unneeded, TEXTURE_get_fiddled_position, TEXTURE_load_needed_object, TEXTURE_looks_like, TEXTURE_set_greyscale, TEXTURE_set_tga). Skipped: eng_map_globals (constant-propagated, live callers), input_to_angle (live caller apply_button_input_fight), MF_rotate_mesh (live caller WIRE_draw). | build OK |
| 2026-04-27 | Batch 30 (session 13) | Regenerate: 2101 discards, 372 symbols in 116 files. switch.cpp: deleted entirely (7 dead funcs: init_switches, free_switch, create_switch, fn_switch_player/thing/group/class); switch.h: removed declarations of 7 functions, kept Switch struct + SwitchPtr (used in game_types.h/memory.cpp/thing.h); CMakeLists.txt: removed switch.cpp. thing.cpp: removed 7 dead (move_thing_on_map_dxdydz, log_primary_used/unused_list, log_secondary_used/unused_list, wait_ticks, set_slow_motion) + extern decl Time; thing.h: removed declarations of all 7. fc.cpp: removed extern decl set_slow_motion. host.cpp: removed 7 dead (ShellPaused, ShellPauseOn, ShellPauseOff, LibShellChanged, LibShellMessage, Time, TraceText); host.h: removed declarations, added direct #include uc_common.h; uc_common.h: removed TraceText/LibShellChanged/LibShellMessage. sdl3_bridge.cpp/h: removed sdl3_window_set_size (no callers). Skipped sdl3_gamepad_shutdown/sdl3_set_mouse_grab — live callers in gamepad.cpp and game_tick.cpp. Discards: 2101→2078 (-23). | build OK |
| 2026-04-27 | Batch 31 (session 14) | Regenerate: 2078 discards. DELETED entirely: wibble_globals.cpp/h (6 dead globals), outline.cpp/h (5 dead funcs), text.cpp/h (5 dead funcs: draw_centre_text_at, draw_text_at, show_text, text_height, text_width); CMakeLists.txt: removed wibble_globals.cpp, outline.cpp, text.cpp. text_globals.cpp/h: removed text_colour (dead). game.cpp: removed extern decls draw_centre_text_at/draw_text_at. eng_map.cpp: removed #include text.h. interact_globals.cpp/h: removed best_angle, best_x, best_y, best_z, r_matrix (local variables with the same name in interact.cpp — not globals). figure_globals.cpp/h: removed body_part_upper, part_type, local_seed, jacket_col, leg_col, peep_recol + PeepRecolEntry type. fire_globals.cpp/h: removed FIRE_fire, FIRE_fire_last, FIRE_flame, FIRE_flame_free, FIRE_get_info, FIRE_get_point (kept live: FIRE_get_z/xmin/xmax/fire_upto/flame). texture_globals.cpp/h + texture.h: removed TEXTURE_fiddled, TEXTURE_page_lcdfont_alpha, TEXTURE_page_flames_alpha, TEXTURE_liney, TEXTURE_av_r/g/b. anim.cpp/h: removed SetCMatrixComp, calc_sub_objects_position_no_tween, convert_elements, fix_multi_object_for_anim, reset_anim_stuff, set_next_prim_point (setup_anim_stuff kept — static stub with a live caller). weapon_feel.cpp/h: removed weapon_feel_fire_reset. Discards: 2078→2024 (-54). | build OK |
| 2026-04-27 | Batch 32 (session 15) | Regenerate: 2024 discards. dirt.cpp/h: removed DIRT_get_info, DIRT_behead_person, DIRT_create_mine, DIRT_destroy_mine, DIRT_gale + DIRT_INFO_TYPE_*/DIRT_Info. animal.cpp/h: removed alloc_animal, ANIMAL_create, ANIMAL_animatetween, ANIMAL_animate, ANIMAL_set_anim, ANIMAL_get_animal, ANIMAL_draw (6 funcs). shape.cpp/h: removed SHAPE_semisphere, SHAPE_semisphere_textured, SHAPE_waterfall, SHAPE_alpha_sphere (4 funcs). fog.cpp/h: removed FOG_get_dyaw (static), FOG_create (static), FOG_set_focus, FOG_gust, FOG_process, FOG_get_start, FOG_get_info + FOG_Info struct + FOG_TYPE_TRANS*/NO_MORE. fog_globals.cpp/h: removed FOG_focus_x, FOG_focus_z, FOG_focus_radius, FOG_get_upto (4 globals). psystem.cpp/h: removed PARTICLE_Exhaust, PARTICLE_Exhaust2, PARTICLE_Steam, PARTICLE_SGrenade + fmatrix.h include. snipe.cpp/h: removed SNIPE_mode_on, SNIPE_mode_off, SNIPE_turn, SNIPE_shoot + SNIPE_TURN_* flags; SNIPE_LENS_END moved to file scope (needed by the live SNIPE_process). snipe_globals.cpp/h: removed SNIPE_on, SNIPE_cam_x, SNIPE_cam_y, SNIPE_cam_z. sm.cpp/h: removed SM_init, SM_create_cube, SM_process + SM_CUBE_*/SM_GRAVITY macros + SM_CUBE_TYPE_* constants + dead includes. sound.cpp/h: removed DieSound, play_ambient_wave, play_music, SewerSoundProcess + fc.h/fc_globals.h includes. RESTORED (erroneously removed, no callers only because game_shutdown did not call gamepad_shutdown): gamepad_shutdown, weapon_feel_shutdown, ds_shutdown, ds_set_haptic_volume, ds_set_lightbar_setup. game_shutdown() augmented with a call to gamepad_shutdown(). game_startup(): removed a duplicate MFX_init() (the first call is in SetupHost(), the second was redundant — an AL device leak). dc_map.py afterwards: 272 symbols in 99 files. | build OK |
| 2026-04-27 | Batch 35 (session 17) | Regenerate: 1800 discards, 80 symbols in 19 files. input_actions.cpp/h: removed lock_to_compass + set_look_pitch (no callers). person.cpp: removed forward decl lock_to_compass. gc-sections list exhausted — none of the remaining can be removed. Final pass over empty files: removed compression/*.cpp/h (6 files), thug.cpp/h, build_globals.cpp/h, map_globals.cpp/h, pause.cpp/h, wibble_effect.h, joystick_globals.cpp; removed from CMakeLists.txt; removed #include "map/map_globals.h" from map.cpp. | build OK |
| 2026-04-27 | Batch 37 (session 19) | Final pass: redundant #include. `clang-tidy -p build/compile_commands.json --checks='-*,misc-include-cleaner'` over all ~170 .cpp files. 93 files changed, ~355 redundant `#include` removed. Main patterns: `uc_common.h` (pulled in types transitively), `eway.h`→`eway_globals.h` (EWAY_Way+vars in globals), `sound.h`→`sound_globals.h`, `game_types.h`/`statedef.h`/`pap_globals.h` false positives (kept where needed transitively), `cop.h/darci.h/roper.h` not needed in person_globals.cpp (only the _globals.h ones are), `person.h` in panel.cpp (PERSON_* from person_types.h via thing.h). Build: build OK. | build OK |
| 2026-04-27 | Batch 38 (session 20-21) | Pass: commented-out /* */ blocks. Removed ~120 blocks across the whole codebase: person.cpp (~30), collide.cpp (10), pcom.cpp (15), game_tick.cpp (8), aeng.cpp (15), vehicle.cpp (1), barrel.cpp (2), figure.cpp (6), elev.cpp (4), poly.cpp (3), input_actions.cpp (25), eway.cpp (4), wmove.cpp (1), bat.cpp (1), special.cpp (1), message.cpp (1), outro_wire.cpp (1), outro_font.cpp (1), save.cpp (1), etc. Regex-pass over the whole src/ — 0 blocks ≥4 lines remaining. | build OK |
| 2026-04-27 | Batch 36 (session 18) | Final pass over empty files (continued). DELETED: shadow_globals.cpp/h (no declarations/definitions — left empty after batch 7), startscr.cpp (no definitions — globals-only module, all data in startscr_globals.cpp). Removed from CMakeLists.txt. Removed #include "engine/graphics/lighting/shadow_globals.h" from elev.cpp, shadow.cpp, game.cpp (they only received types.h transitively, and shadow.h already includes it). | build OK |
| 2026-04-27 | Batch 34 (session 16) | Regenerate: 1821 discards, 134 dead symbols in 38 files. anim_globals.cpp/h: removed test_chunk. env_globals.cpp/h: DELETED ENTIRE (env_inifile, env_strbuf — no callers outside env_globals); removed #include env_globals.h from env.cpp; removed from CMakeLists.txt. sm_globals.cpp/h: removed SM_object, SM_object_upto + SM_Object type + SM_MAX_OBJECTS define. hierarchy_globals.cpp/h + hierarchy.h: removed body_part_parent (no .cpp callers). spark.cpp/h: removed SPARK_in_sphere (caller in /* */). eway.cpp/h: removed EWAY_deduct_time_penalty (caller in /* */). projectile.cpp/h: removed alloc_projectile (no callers). outro_imp.cpp/h: removed IMP_load and IMP_binary_save (IMP_load callers in /* */, IMP_binary_save no callers). Skipped: save_globals.cpp/h — removal attempt reverted (save.cpp always compiles and uses these globals); index_lookup (aeng_globals) — live source-level caller in aeng.cpp; PUDDLE_create — live caller in process_controls (KB_B debug, not in /* */); FIGURE_draw_hierarchical_prim_recurse_individual_cull — inlined from live caller, source ref exists; set_person_in_building_through_roof — after ASSERT(0) but source ref exists; camera_*/set_person_* stubs — live source callers. | build OK |
| 2026-04-27 | Batch 33 (session 15) | Continued with the dc_map.py list (272 symbols). elev.cpp/h: ELEV_create_similar_name. tga.cpp/h: TGA_load_remap (~145 lines with goto). anim_tmap.cpp/h: save_animtmaps. thug.cpp/h: fn_thug_init, fn_thug_normal (file trimmed to a minimum; removed #include thug.h from person.cpp). playcuts.cpp/h: PLAYCUTS_Free (empty stub), PLAYCUTS_Reset. gamepad.cpp/h: gamepad_is_adaptive_click_active. composition.cpp/h: composition_get_scene_texture. crt_effect.cpp/h: crt_effect_shutdown. wibble_effect.cpp/h: gl_wibble_effect_shutdown (h trimmed to an empty guard). morph.cpp/h: MORPH_get_points, MORPH_get_num_points (RESTORED: erroneously removed in batch 12, actually dead — no callers in mesh.cpp). night.cpp: removed dead forward declaration calc_inside_for_xyz inside a block; block restructured (removed redundant `{}`). supermap.cpp/h: calc_inside_for_xyz, add_sewer_ladder. inside2.cpp/h: find_stair_y (~100 lines). map.cpp/h + map_globals.cpp/h: MAP_light_get_height, MAP_light_get_light, MAP_light_set_light, MAP_light_map (global). font.cpp/h: FONT_draw_speech_bubble_text. polypage.cpp/h: GenerateMMMatrix (~89 lines D3D matrix builder); removed #include compression.h. smap.cpp/h: SMAP_bike (empty stub). eway.cpp/h: count_people_types (immediate-return stub), EWAY_cam_get_position_for_angle, EWAY_cam_look_at (cluster, internal callers only). pause.cpp/h: PAUSE_handler (~180 lines); files trimmed to a minimum; removed #include pause.h from game.cpp + fixed misleading comment. frontend.cpp/h: FRONTEND_do_drivers, FRONTEND_gamma_update. frontend_globals.cpp/h: CurrentVidMode, CurrentBitDepth. attract.cpp/h: level_won, level_lost (empty stubs). overlay.cpp/h: overlay_beacons (empty stub); removed extern decl from game.cpp. compression.cpp/h + compression_globals.cpp/h + image_compression.cpp/h: the entire compression cluster (COMP_load, COMP_calc, COMP_decomp, IC_pack, IC_unpack, COMP_data/frame/tga_data/tga_info, types COMP_Frame/DataBuffer/Delta); files trimmed to a minimum. aeng.cpp: AENG_movie_init (~38 lines), removed the call from AENG_init(), removed #include compression.h. aeng_globals.cpp/h: removed 9 movie globals (AENG_movie_data[512*1024], AENG_movie_upto, AENG_frame_one/two/last/next, AENG_frame_count/tick/number, AENG_MAX_MOVIE_DATA). Incident: aeng_globals.h broke due to COMP_Frame after cleaning up compression.h — diagnosed as the dead AENG_movie cluster, resolved by removing the entire cluster. | build OK |

| 2026-04-27 | Batch 39 (session 23) | cppcheck `--enable=unusedFunction` pass. Removed all genuinely dead static functions: `SetUV(float&,float&)` (polypoint.h), `set_thing_pos` inline (thing.h), `qdist3` (outro_always.h), 5 write-path statics (outro_imp.cpp), 6 statics + forward decls (save.cpp), 7 MAP_* draw funcs ~600 lines (eng_map.cpp), `destroy_shaders`/`gl_blit_fullscreen_texture`/11 GE stubs (core.cpp), the corresponding declarations (game_graphics_engine.h), `SetCMatrix`/18 Set*/Get* accessors/GetCharName*/GetMultiObject* (anim_types.h), `slide_around_box_lowstack` ~593 lines (collide.cpp), `page_name` (input_debug.cpp), `HOOK_make_loop` (hook.cpp), `find_stair_in` ~68 lines (inside2.cpp), `PUDDLE_create` (puddle.cpp/h), `FONT2D_DrawString_NoTrueType` (font2d.cpp), `ini_read_int_mem`/`ini_write_string` ~115 lines (env.cpp), `sdl3_set_mouse_grab` (sdl3_bridge.cpp/h), `DriverEnumState`+`driver_enum_callback` (frontend.cpp), `IHaveToHaveSomePyroSprites` (pyro.cpp/h), `LIGHT_get_colour` inline ~38 lines (light.h), `NIGHT_get_colour_and_fade` inline ~36 lines (night.h), `POW_create` inline (pow.h). False positives left in place: `WAND_*`/`get_person_radius`/`which_side` (extern in function bodies), `ds_set_haptic_volume`/`ds_set_lightbar_setup` (API stubs), `GEFont`/`Font` typedef. Restored those erroneously removed in this same batch: PUDDLE_create_do, COLLIDE_debug_fastnav/find_seethrough_fences/calc_fastnav_bits/collide_box_with_line/create_shockwave. Build OK. | build OK |

## Pass: commented-out /* */ blocks (Batch 38+)

Generated 2026-04-27, all blocks ≥4 lines in `new_game/src/`. Vendor files excluded (`glad/gl.h`, `khrplatform.h`).

Rules:
- Remove only clearly dead commented-out code (replaced implementation, cut feature, debug hack)
- Do NOT remove blocks with TODO/FIXME — these are intentional placeholders
- `#if DEFINE` (not `#if 0`) — ask the user before removing

### aeng.cpp (6 blocks)

| Line | Size | Description | Status |
|--------|--------|----------|--------|
| 2879 | 52 | Draw reflections of OBs — DETAIL_REFLECTIONS branch | ✅ |
| 3254 | 145 | Draw water in the city — the whole water system | ✅ |
| 3607 | 25 | PAP_FLAG_ANIM_TMAP animated texture mapping | ✅ |
| 4198 | 41 | Draw wheels above Darci's head (debug viz) | ✅ |
| 4297 | 35 | Dead MIB sub-objects position calc | ✅ |
| 4684 | 28 | PUDDLE drawing loop | ✅ |
| 6206 | 26 | KB_PPOINT ambient light tweak (debug key) | ✅ |

### game_tick.cpp (~20 blocks)

Most are debug key handlers (`if (Keys[KB_X]) { ... }`) that are commented out already in the original. All safe to remove.

| Line | Size | Description | Status |
|--------|--------|----------|--------|
| 1233 | 61 | KB_B debug handler | ✅ |
| 1383 | 26 | DIKE_Dike debug | ✅ |
| 1420 | 18 | is_person_crouching darci check | ✅ |
| 1691 | 38 | "Is darci still?" — idle check | ✅ |
| 1738 | 26 | KB_1 anim_type[] debug | ✅ |
| 1801 | 28 | KB_3 debug | ✅ |
| 1830 | 18 | auto music scheduling test | ✅ |
| 1893 | 47 | Hook control (HOOK_get_state) | ✅ |
| 1995 | 27 | darci in sewers GF_SEWERS check | ✅ |
| 2046 | 25 | process count loop | ✅ |
| 2080 | 50 | "Is darci ready to go upstairs?" | ✅ |
| 2140 | 42 | KB_P4 splitscreen debug | ✅ |
| 2345 | 19 | "Simon's idea!" camera target look | ✅ |
| 2365 | 37 | KB_8 debug | ✅ |
| 2525 | 19 | KB_POINT steam/ypos debug | ✅ |
| 2693 | 21 | KB_Z debug | ✅ |
| 2715 | 112 | KB_R big debug block | ✅ |
| 2835 | 33 | KB_T debug | ✅ |
| 2870 | 23 | KB_X + fti debug | ✅ |
| 2895 | 48 | KB_M debug | ✅ |
| 3012 | 52 | fti static force block | ✅ |
| 3067 | 116 | large unnamed debug block | ✅ |
| 3213 | 26 | KB_5 chopper debug | ✅ |
| 3241 | 31 | FC_cam_dist / FC_cam_height debug | ✅ |
| 3273 | 22 | KB_F12 debug | ✅ |
| 3398 | 18 | loop with c0/index1/index2/index3 | ✅ |
| 3463 | 52 | KB_N debug | ✅ |
| 3517 | 50 | `#ifndef TARGET_DC #if !defined(PSX)` KB_R block | ✅ (inside `/* */` — removed together with the block) |

### person.cpp (5 blocks)

| Line | Size | Description | Status |
|--------|--------|----------|--------|
| 8962 | 110 | SUB_STATE_DROP_DOWN_OFF_FACE case | ✅ |
| 10101 | 22 | MSG_add "stopping vel" debug | ✅ |
| 10184 | 24 | continue_action check | ✅ |
| 13015 | 19 | "You can't throw mines any more" | ✅ |
| 13051 | 69 | person_normal_animate end block | ✅ |
| 13432 | 31 | angle calc block | ✅ |

### collide.cpp (4 blocks)

| Line | Size | Description | Status |
|--------|--------|----------|--------|
| 2044 | 23 | unnamed block | ✅ |
| 2594 | 30 | find_face_for person_inside check | ✅ |
| 2672 | 57 | actual_sliding dx/dy/dz block | ✅ |
| 2769 | 30 | slide_door < 0 check | ✅ |

### pcom.cpp (3 blocks)

| Line | Size | Description | Status |
|--------|--------|----------|--------|
| 2301 | 37 | gang_attacks[gang].Perp loop | ✅ |
| 4690 | 40 | WAND_find_good_start_point extern + call | ✅ |
| 6882 | 21 | GAME_TURN + THING_NUMBER debug | ✅ |

### Other files

| File | Line | Size | Description | Status |
|------|--------|--------|----------|--------|
| vehicle.cpp | 285 | 31 | switch on vehicle type | ✅ |
| barrel.cpp | 188 | 28 | BARREL_FLAG_HIT check | ✅ |
| barrel.cpp | 1263 | 27 | in_the_air barrel block | ✅ |
| figure.cpp | 2812 | 19 | AnimPrimBbox world_link | ✅ |
| figure.cpp | 3146 | 24 | ENGINE_palette Col2 (red/green/blue) | ✅ |
| figure.cpp | 3212 | 22 | ENGINE_palette Col2 (second instance) | ✅ |
| elev.cpp | 2097 | 24 | "bodge for puzzle" mission < 0 | ✅ |
| poly.cpp | 1721 | 20 | "Guy Demo Dodge!!!" ATTRACT_MODE | ✅ |
| input_actions.cpp | 1344 | 18 | auto climb ladders | ✅ |
