# Stage 12 — findings index

Полный ID-индекс находок из `original_game_compare/docs/release_changes/`.
Этот файл — **рабочая память Claude**, не для ревью. План решений с блоками — [`plan.md`](plan.md).

## Легенда

**Decision:**
- `PORT` — портируем из `new/` в нашу базу
- `SKIP` — не трогаем (out-of-scope, dead code, D3D6, PSX, editor, dev tooling)
- `OPEN` — требует обсуждения при ревью / решения пользователя
- `AUDIT` — сначала проверить текущее состояние нашей базы (inverted findings / ambiguous)
- `DONE` — уже сделано на Stage 5 или ранее
- `SELF` — у нас свой путь лучше (например своя sort прозрачных)
- `INVERT-KEEP` — inverted direction, наша pre-release сторона правильная, ничего не делаем (при условии что оно у нас действительно есть — см. AUDIT)

**Size:** T = trivial (1-10 строк) · M = medium (10-100) · L = large (100+)

**Source:** ссылка на секцию в соответствующем `release_changes/*.md` отчёте (1P = first pass, VP = verification pass, DP = deep pass)

---

## physics_maths (PHY)

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| PHY-01 | `height_above_anything` Ware branch | collide.cpp | DEFERRED 2026-04-14 | T | **НЕ bugfix, behavior change.** Pre-release: `if(1\|\|Ware)` — else мёртв, всегда NEAR_BELOW. Release: убрали `1\|\|`, non-Ware → FIND_ANYFACE. Наш код выкинул мёртвую ветку (= pre-release семантически). Отложено до retail feel-check. См. plan.md Открытые вопросы #27 |
| PHY-02 | `calc_map_height_at` mz→z typo | collide.cpp | INVERT-KEEP | T | 2026-04-14: наша база корректна `(mx, mz)`, release ввёл регрессию `(mx, z)` — не портируем |
| PHY-03 | `create_shockwave` `just_people` removal | collide.cpp | PORT P2 | T | API simplification, чище |
| PHY-04 | `insert/remove_collision_vect` runtime pool | collide.cpp | SKIP | M | для destructible props, нет сейчас |
| PHY-05 | `SUB_STATE_WALKING_BACKWARDS` edge-slide guard uncomment | collide.cpp | PORT P1 | T | backwards-ходящие NPC перестанут snap-slide |
| PHY-06 | `slide_around_box` retry loop deletion | collide.cpp | OPEN | T | inverted, release убрал retry механизм |
| PHY-07 | `LOS_FLAG_INCLUDE_CARS` removal | collide.cpp | PORT P2 | T | API simplification |
| PHY-08 | `SLIDE_ALONG_FLAG_JUMPING` dead flag removal | collide.cpp | SKIP | T | dead code уже |
| PHY-09 | `walk_links` editor pool deletion | collide.cpp | SKIP | T | editor-only, 120KB |

## engine_internals (ENG)

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| ENG-01 | `COMMON(TYPE)` macro 4→8 fields | Structs.h | SKIP | L | огромный ABI cascade, Stage 13 territory |
| ENG-02 | MemTable public API | memory.h | PORT P2 | T | если нужно — иначе SKIP |
| ENG-03 | save_table Extra counts reduced | memory.cpp | AUDIT | T | проверить наши текущие значения |
| ENG-04 | Per-level Maximum fudge removal | memory.cpp | SKIP | T | dev hack |
| ENG-05 | `flag_v_faces` function deletion | memory.cpp | AUDIT | M | где сейчас ставится FACE_FLAG_VERTICAL |
| ENG-06 | load_whole_game init calls removed | memory.cpp | AUDIT | T | проверить наш init path |
| ENG-07 | EWAY char-160 patch removal | memory.cpp | SKIP | T | fixed at data-build |
| ENG-08 | `count -= Extra` save format break | memory.cpp | SKIP | T | связано с ENG-01 |
| ENG-09 | Dreamcast wad writer deletion | memory.cpp | SKIP | L | DC only |
| ENG-10 | Door/Sound FX save_table rows removal | memory.cpp | AUDIT | M | что у нас сейчас хранится |
| ENG-11 | bike.h include add | memory.cpp | SKIP | T | placeholder |
| ENG-12 | person warmup 40→10 iterations | memory.cpp | PORT P1 | T | loading time |
| ENG-13 | MEMORY_quick_save/load MF_Fopen → fopen | memory.cpp | PORT P2 | T | API cleanup |
| ENG-14 | CLASS_ANIM_PRIM `#ifdef` dispatch | memory.cpp | SKIP | T | dead ifdef |
| ENG-15 | OB_compress malloc swap | ob.cpp | PORT P2 | T | cleanup |
| ENG-16 | envmap transparent-face suppression removal | ob.cpp | OPEN | M | cause unclear, verify |
| ENG-17 | ob_allowed_to_be_walkable PSX gate | ob.cpp | SKIP | T | PSX only |
| ENG-18 | **`OB_remove` missing return uncomment** | ob.cpp | DONE 2026-04-14 | T | критический баг итерации |

## io_stdlib (IO)

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| IO-01 | FileSetBasePath removal | StdFile.cpp | AUDIT | T | наш path resolution другой |
| IO-02 | MemReAlloc + MFnew/MFdelete removal | StdMem.cpp | SKIP | T | cleanup |
| IO-03 | `Root()` integer sqrt | StdMaths.cpp | OPEN | M | determинизм, обсудить при ревью |
| IO-04 | main() TCHAR→CBYTE | MFStdLib.h | SKIP | T | уже int main(int, char**) |
| IO-05 | debug macros hard-stubbed | MFStdLib.h | SKIP | T | infonly |
| IO-06 | WaveParams API publication | MFStdLib.h | SKIP | T | мы через miniaudio |

## audio (AUD)

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| AUD-01 | `GAME_TURN & 0x3f` rain/ambience gate removal | Sound.cpp | PORT P0 | T | ambience мог умереть на 64 тика |
| AUD-02 | Wind retry jitter + MFX_QUEUED | Sound.cpp | PORT P0 | T | gusts перекрывали друг друга |
| AUD-03 | city_animals 7→11 (add S_DOG1-4) | Sound.cpp | PORT P1 | T | pool expansion |
| AUD-04 | Snow wolf S_AMB_WOLF1→3 | Sound.cpp | PORT P1 | T | likely broken WOLF1 |
| AUD-05 | SOUND_FXMapping static array | Sound.h | AUDIT | T | какое у нас сейчас |
| AUD-06 | MUSIC per-Ware override removal | music.cpp | OPEN | T | inverted, надо проверить в retail |
| AUD-07 | Wind index ordering fix (WIND2↔WIND5) | sound_id.cpp | DONE 2026-04-14 | T | индексный баг |
| AUD-08 | FrontLoop.wav removal | sound_id.cpp | AUDIT | T | если у нас есть — удалить |

## map_world (MAP)

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| MAP-01 | DFacet/DBuilding unions | supermap.h | SKIP | T | inverted, struct layout для inside rooms |
| MAP-02 | LIGHT_COLOURED flag + mono fallback | light.h | SKIP | T | inverted, Glide/PSX only |
| MAP-03 | NIGHT_LIGHT_MULTIPLIER macro | night.cpp | SKIP | T | tuning hook, no runtime delta |
| MAP-04 | SOFTWARE darkening in night.cpp | night.cpp | SKIP | T | software path |
| MAP-05 | FACET_FLAG_DLIT removal | night.cpp | AUDIT | T | где ставится dynamic light |
| MAP-06 | fc.cpp camera LOS-pushback gating | fc.cpp | INVERT-KEEP | M | inverted, pre-release строжее |
| MAP-07 | EWAY_evaluate_condition negate + 3rd param | eway.cpp | AUDIT | M | должно быть у нас |
| MAP-08 | EWAY music-stopping | eway.cpp | AUDIT | T | inverted pre-release fix |
| MAP-09 | EWAY_cam goinactive 2-frame blend | eway.cpp | AUDIT | T | inverted |
| MAP-10 | Tripwire length 0x500 + LOS flag | eway.cpp | AUDIT | T | inverted |
| MAP-11 | Semtex waypoint 124 exclusion | eway.cpp | AUDIT | T | inverted |
| MAP-12 | Shockwave on vehicle hit | eway.cpp | PORT P2 | T | matches final |
| MAP-13 | inside2.cpp MAV-nav addition | inside2.cpp | SKIP | L | WIP в new/, unfinished |
| MAP-14 | calc_inside_for_xyz helper | supermap.cpp | SKIP | T | только если inside-nav |
| MAP-15 | remove_walkable_from_map 50-iter cap | build2.cpp | PORT P1 | T | safety net |
| MAP-16 | DOOR_Door static array | door.cpp | PORT P2 | T | encapsulation |
| MAP-17 | NIGHT_Dlight extra fields | night.h | AUDIT | T | для FACET_FLAG_DLIT |
| MAP-18 | NIGHT_amb_norm externs | night.h | SKIP | T | no port needed |
| MAP-19 | NIGHT_get_d3d_colour_and_fade removal | night.h | SKIP | T | inverted dead |
| MAP-20 | TomsPrimObject scaffolding removal | prim.h | SKIP | T | inverted dead |
| MAP-21 | GAME_SCALE float→double | prim.h | SKIP | T | cosmetic |
| MAP-22 | MAX_PRIM_POINTS runtime→static | building.h | AUDIT | T | проверить наши значения |
| MAP-23 | FILE_OPEN_ERROR check (night.cpp) | night.cpp | AUDIT | T | должно быть у нас |
| MAP-24 | Defensive ASSERTs — mixed pick | engineMap/eway/elev | AUDIT | T | case-by-case |
| MAP-25 | EWAY_conv_ambient reset | eway.cpp | AUDIT | T | inverted |
| MAP-26 | EWAY_COND_WITHIN early return | eway.cpp | AUDIT | T | inverted |
| MAP-27 | FC_camera MARKS_MACHINE block | fc.cpp | SKIP | T | dead both sides |
| MAP-28 | build_tims_ingame PSX pipeline | supermap.cpp | SKIP | L | PSX only |
| MAP-29 | edit_info.HideMap + PSX jacket | Building.cpp | SKIP | T | editor+PSX |
| MAP-30 | PAP height helpers `#if 0` | pap.cpp | SKIP | T | dead |
| MAP-31 | road.cpp gpost3 exclusion + PSX branch | road.cpp | AUDIT | T | есть ли у нас |
| MAP-32 | ELEV_load_level civ/car PSX gating | elev.cpp | SKIP | T | PSX only |
| MAP-33 | RunCutscene(2) vs (3) | elev.cpp | OPEN | T | cutscene timing, verify |
| MAP-34 | ELEV FILE_OPEN_ERROR check | elev.cpp | AUDIT | T | должно быть у нас |
| MAP-35 | TEXTURE_load_needed 4-arg | elev.cpp | SKIP | T | API change |
| MAP-36 | WMOVE_create PSX guard | eway.cpp | SKIP | T | PSX |
| MAP-37 | PSX guards in eway.cpp | eway.cpp | SKIP | T | PSX |
| MAP-38 | crap_levels list tweak | eway.cpp | SKIP | T | data |

## mission_game (MIS)

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| MIS-01 | Grab-edge 4→2 point test | interact.cpp | OPEN | T | verify in retail |
| MIS-02 | Mission::Flags CBYTE→BOOL | Mission.h | INVERT-KEEP | T | inverted, pre-release правильнее |
| MIS-03 | DrawTween/DrawMesh empty shells fill | drawtype.h | AUDIT | T | что у нас сейчас |
| MIS-04 | load_needed_anim_prims per-level gating | io.cpp | AUDIT | T | inverted memory opt |
| MIS-05 | WARE_roof_tex memset | io.cpp | AUDIT | T | inverted leak fix |
| MIS-06 | game_loop cleanups block | Game.cpp | AUDIT | M | inverted polish |
| MIS-07 | save.cpp MF_Fopen + ULONG + dup leak | save.cpp | AUDIT | T | inverted |
| MIS-08 | process_paused_mode (pre uses GAMEMENU) | Game.cpp | AUDIT | M | inverted, наша сторона |
| MIS-09 | Null-deref guard GS_LEVEL_WON | Game.cpp | AUDIT | T | inverted |
| MIS-10 | game_startup loadscreen order | Game.cpp | AUDIT | T | inverted UX |
| MIS-11 | GetInputDevice signature | Game.cpp | SKIP | T | SDL |
| MIS-12 | game_fini early call | Game.cpp | AUDIT | T | inverted |
| MIS-13 | FC_process call move | Game.cpp | OPEN | T | locate call |
| MIS-14 | SOFTWARE path | Game.cpp | SKIP | T | software |
| MIS-15 | ftol_init / DX device verify | Main.cpp | SKIP | T | x86/DX6 |
| MIS-16 | FileSetBasePath("") call | Main.cpp | SKIP | T | CWD reset |
| MIS-17 | load_game_map .p/.i extension | io.cpp | SKIP | T | PSX |
| MIS-18 | load_level_anim_prims bat skip PSX | io.cpp | SKIP | T | PSX |
| MIS-19 | context_music nightclub mode=0 | Controls.cpp | OPEN | T | verify retail |
| MIS-20 | playcuts MFX_update→render | playcuts.cpp | SKIP | T | API rename |
| MIS-21 | TEXTURE_load_needed 4-arg (Game.cpp) | Game.cpp | SKIP | T | API |

## graphics_rendering (GFX) — 60+ findings

### 1st pass

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| GFX-01 | apply_cloud real impl | facet.cpp | PORT P0 | M | cloud shadows на фасадах |
| GFX-02 | FACET_FLAG_IN_SEWERS invisible routing | facet.cpp | PORT P0 | T | sewer facets |
| GFX-03 | Shadow Z-bias removal (DC_SHADOW_Z_ADJUST) | shape.cpp | SELF | M | у нас OpenGL depth offset |
| GFX-04 | Moon easter egg (MANONMOON) | sky.cpp | PORT P2 | T | content |
| GFX-05 | POLY_add_poly unified pipeline | poly.cpp | SELF | L | свой GE |
| GFX-06 | POLY_transform saturate_z split | poly.cpp | SELF | T | свой GE |
| GFX-07 | POLY_shadow buffer enlarge | poly.h | DONE 2026-04-14 | T | оба размера уже 8192 — no-op, применено для консистентности с release |
| GFX-08 | Pyro throttle removal | drawxtra.cpp | PORT P1 | M | убрать frame-budget throttle |
| GFX-09 | sw_hack / SOFTWARE flag | aeng/poly/facet etc | SKIP | L | software path |
| GFX-10 | renderstate fix_directx Permedia2 | renderstate.cpp | SKIP | M | D3D6 specific |
| GFX-11 | Engine.h CameraRAngle restore | Engine.h | AUDIT | T | проверить |
| GFX-12 | TEXTURE_load_needed signature simplify | aeng.h/texture.cpp | SKIP | L | API change |
| GFX-13 | polypage TEX_EMBED removal | polypage.cpp | SELF | M | OpenGL slой |
| GFX-14 | USE_D3D_VBUF compile switch | vertexbuffer | SKIP | L | D3D VB |
| GFX-15 | oval.cpp Z-fight removal | oval.cpp | OPEN | T | unclear if regression |
| GFX-16 | aeng.cpp dead-code pruning | aeng.cpp | SKIP | L | deletion, neutral |
| GFX-17 | **FillFacetPoints foundation==2 UV fix** | facet.cpp | PORT P0 | T | stretchy pants bug |

### Verification pass

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| GFX-18 | aeng.cpp reordered correction | aeng.cpp | — | — | infonly |
| GFX-19 | **AENG_upper/lower & 63 mask drop** | aeng.cpp | DONE 2026-04-14 | T | с GFX-37 lock-step, 31 masks removed |
| GFX-20 | Car color palette swap (debug→muted) | aeng.cpp | OPEN | T | verify retail |
| GFX-21 | Figure LOD distance removal | aeng.cpp | PORT P1 | T | pop-in fix |
| GFX-22 | Mesh fade arg 0xff→0 | aeng/shape | OPEN | T | cause unclear |
| GFX-23 | AENG_draw_dirt rewrite | aeng.cpp | SELF | L | наш Stage 5 fix работает |
| GFX-24 | Draw order shuffle | aeng.cpp | AUDIT | M | сверить с нашим transparent sort |
| GFX-25 | POLY_frame_draw_sewater new API | poly.cpp | AUDIT | M | может быть в GE |
| GFX-26 | POLY_frame_draw fog colour tint | poly.cpp | AUDIT | M | сверить |
| GFX-27 | INDOORS alpha/fog override | poly.cpp | AUDIT | M | сверить |
| GFX-28 | POLY_fadeout_point re-enable (facet ~8 sites) | facet.cpp | PORT P1 | T | vertex fog |
| GFX-29 | FacetPoints WrapJustOnce call | facet.cpp | PORT P1 | T | pairs with GFX-17 |
| GFX-30 | FacetPoints Common helper removal | facet.cpp | SKIP | T | dead |
| GFX-31 | draw_facet_foundation new helper | facet.cpp | AUDIT | M | call sites |
| GFX-32 | cable_draw sw_hack colour | facet.cpp | SKIP | T | software |
| GFX-33 | Texture pages loaded (people3 tier) | texture.cpp | AUDIT | M | есть ли people3 в ресурсах |
| GFX-34 | ZBIAS per-page unchanged | polyrenderstate.cpp | — | — | infonly |
| GFX-35 | POLY_add_quad sort_to_front 4-arg | sprite.cpp/poly.h | AUDIT | T | наш API |
| GFX-36 | AENG SOFTWARE/sw_hack gates + CurDrawDistance | aeng.cpp | PART | M | CurDrawDistance portable, sw skip |

### Deep pass

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| GFX-37 | **AENG_upper/lower full size resize** | aeng.cpp | DONE 2026-04-14 | T | resize to [MAP_WIDTH][MAP_HEIGHT] |
| GFX-38 | CurDrawDistance fixed-point drop + ENV | aeng.cpp | AUDIT | M | какое у нас представление |
| GFX-39 | AENG_cam_matrix removal | aeng.cpp | AUDIT | T | есть ли consumers |
| GFX-40 | AENG_init texture hook + POLY_frame_init | aeng.cpp | PORT P2 | T | init order |
| GFX-41 | AENG_calc_gamut_lo_only removal | aeng.cpp | PORT P2 | M | simplification |
| GFX-42 | AENG_draw_rain shading wrap | aeng.cpp | — | T | infonly |
| GFX-43 | SHADOW_Z_BIAS_BODGE macro removal | aeng.cpp | SKIP | T | dead |
| GFX-44 | show_facet debug helper restore | aeng.cpp | SKIP | T | debug |
| GFX-45 | Float literal 0.0001f→0.0001 | aeng.cpp | SKIP | T | cosmetic |
| GFX-46 | AENG_draw_dirt PRIM/MORPH types | aeng.cpp | SKIP | L | часть GFX-23 rewrite |
| GFX-47 | AENG_nswater struct | aeng.cpp | AUDIT | T | pairs с GFX-25 |
| GFX-48 | Floor quads backface cull TRUE | aeng.cpp | PORT P1 | T | floor not visible from below |
| GFX-49 | Moon distance AENG_DRAW_DIST | aeng.cpp | PORT P1 | T | binocular zoom |
| GFX-50 | SOFTWARE gates on prims | aeng.cpp | SKIP | T | software |
| GFX-51 | AENG_draw_warehouse facet filter removal | aeng.cpp | PORT P1 | T | крыши через non-NORMAL |
| GFX-52 | Warehouse balloon local restore | aeng.cpp | PORT P2 | T | cleanup |
| GFX-53 | Draw order dirt gate (indoors + detail) | aeng.cpp | PORT P1 | T | no dirt indoors |
| GFX-54 | AENG_draw_box_around_door sw_hack branch | aeng.cpp | SKIP | T | software |
| GFX-55 | Far-facets dead counter | aeng.cpp | — | T | infonly |
| GFX-56 | AENG_draw sw_hack toggle | aeng.cpp | SKIP | T | software |
| GFX-57 | AENG_flip/blit SW path | aeng.cpp | SKIP | T | software |
| GFX-58 | draw_debug_lines helper | aeng.cpp | PORT P2 | T | для нашей отладки |
| GFX-59 | Misc float literal changes | aeng.cpp | SKIP | T | cosmetic |
| GFX-60 | POLY scaffolding removal (flush_local_rot) | aeng.cpp | SELF | T | наш фикс Stage 5 лучше |
| GFX-61 | REALLY_SET→SET | aeng.cpp | SKIP | T | caching |
| GFX-62 | ENV_set detail toggles re-enable | aeng.cpp | PORT P1 | T | настройки сохраняются |
| GFX-63 | FPS counter locals | aeng.cpp | SKIP | T | cosmetic |

## animation (ANM)

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| ANM-01 | TomsPrimObject MultiMatrix rip | figure.cpp | SKIP | L | D3D6 opt |
| ANM-02 | FIGURE_draw calc_global_cloud re-enable | figure.cpp | PORT P0 | T | fog на людях |
| ANM-03 | FIGURE_draw null ae1/ae2 guard | figure.cpp | DONE 2026-04-14 | T | crash guard |
| ANM-04 | Grenade fade flag 0xff→0 | figure.cpp | PORT P1 | T | fog на гранате |
| ANM-05 | FIGURE_draw_prim_tween_warped cloud | figure.cpp | PORT P0 | T | fog на warped prims |
| ANM-06 | 912-line TPO helpers drop | figure.cpp | SKIP | L | consequence ANM-01 |
| ANM-07 | draw_flames LOD_mul | figure.cpp | PORT P1 | T | denser flames |
| ANM-08 | draw_steam POLY_set_local_rotation_none removal | figure.cpp | AUDIT | T | сверить |
| ANM-09 | MESH_draw_guts fog re-enable | mesh.cpp | PORT P0 | M | fog на мешах |
| ANM-10 | MESH_draw_reflection fadeout | mesh.cpp | PORT P1 | T | reflections |
| ANM-11 | MESH_draw_morph cloud | mesh.cpp | PORT P0 | T | fog на morph |
| ANM-12 | MESH_draw_guts crumple path fog | mesh.cpp | PORT P0 | T | crumple fog |
| ANM-13 | **MESH_draw_guts FACE_FLAG_ANIMATE** | mesh.cpp | PORT P0 | M | scrolling textures dead |
| ANM-14 | MESH_draw_guts ModulateD3DColours | mesh.cpp | PORT P1 | T | tinting |
| ANM-15 | MESH_draw_guts untextured ASSERT removal | mesh.cpp | DONE 2026-04-14 | T | crash fix |
| ANM-16 | Anim.cpp PSX build split | Anim.cpp | SKIP | L | PSX split |
| ANM-17 | Anim.cpp ROPER2 strangle/headbutt gate | Anim.cpp | SKIP | T | PSX |
| ANM-18 | Anim.cpp carry animations gate | Anim.cpp | SKIP | T | PSX |
| ANM-19 | convert_to_psx_gke helper | Anim.cpp | SKIP | T | PSX |
| ANM-20 | morph.cpp MF_Fopen → fopen | morph.cpp | PORT P2 | T | API cleanup |
| ANM-21 | MESH reflections MemAlloc→malloc | mesh.cpp | PORT P2 | T | API |
| ANM-22 | mesh.cpp fastprim include removal | mesh.cpp | AUDIT | T | cleanup |
| ANM-23 | CALC_CAR_CRUMPLE dev macro | mesh.cpp | SKIP | T | dev |
| ANM-24 | Adami hardware lighting two-pass | figure.cpp | SKIP | M | D3D6 only |
| ANM-25 | sw_hack lighting doubling | figure.cpp | SKIP | T | software |
| ANM-26 | Dead head-lookat matrix | figure.cpp | SKIP | M | `if (0)` scaffold |
| ANM-27 | Dead locked-limb offset | figure.cpp | SKIP | M | `if (0)` scaffold |

## particles_effects (PRT)

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| PRT-01 | **FIRE_Flame struct fill** | fire.cpp | PORT P0 | M | огонь не анимируется |
| PRT-02 | FIRE_Fire struct expand (size/shrink/x/y/z) | fire.cpp | PORT P0 | T | pairs PRT-01 |
| PRT-03 | FIRE_add_flame randomization | fire.cpp | PORT P0 | T | pairs PRT-01 |
| PRT-04 | **FIRE_process free-list fix** | fire.cpp | DONE (already applied) | T | 2026-04-14 audit: наш код уже имеет `*prev = fl->next;` |
| PRT-05 | DIRT encapsulation (struct TU-local) | dirt.h/cpp | PORT P2 | T | cleanup |
| PRT-06 | DIRT_MARK macro removal | dirt.h | PORT P2 | T | cleanup |
| PRT-07 | **psystem first_pass/prev_tick decl + tick_diff** | psystem.cpp | PORT P0 | T | не должен даже собираться у pre |
| PRT-08 | **psystem local_ratio computation** | psystem.cpp | PORT P0 | T | critical |
| PRT-09 | psystem PFLAG_HURTPEOPLE throttle | psystem.cpp | PORT P0 | T | balance |
| PRT-10 | **psystem iterator advance free path** | psystem.cpp | DONE (already applied) | T | 2026-04-14 audit: psystem state fix уже применён |
| PRT-11 | **psystem iterator advance normal path** | psystem.cpp | DONE (already applied) | T | 2026-04-14 audit: psystem state fix уже применён |
| PRT-12 | psystem PFLAG_RESIZE2 init | psystem.cpp | PORT P1 | T | never set |
| PRT-13 | psystem FADE type SLONG | psystem.cpp | PORT P1 | T | underflow |
| PRT-14 | psystem menu-pause early-out removal | psystem.cpp | OPEN | T | verify |
| PRT-15 | pyro global_spang cap removal | pyro.cpp | PORT P0 | T | "missing sparks" |
| PRT-16 | pyro init TRACE | pyro.cpp | SKIP | T | debug |
| PRT-17 | pyro bonfire sound removal | pyro.cpp | PORT P1 | T | verify retail |
| PRT-18 | pyro thing ASSERT removal | pyro.cpp | SKIP | T | debug |
| PRT-19 | MAX_PYROS MIKE halving | pyro.h | SKIP | T | dev |
| PRT-20 | pow.cpp safeguards removal | pow.cpp | PORT P1 | M | use release default |
| PRT-21 | pcom.cpp arrest queue removal | pcom.cpp | OPEN | M | API simplification |
| PRT-22 | pcom.cpp noise queue removal | pcom.cpp | OPEN | M | API simplification |
| PRT-23 | pcom.cpp AI ASSERT tripwires removal | pcom.cpp | DONE 2026-04-14 | T | убраны DARCI/CIV/COP asserts в 5 функциях + alert_my_gang |
| PRT-24 | pcom cop search radius 0x800→0x700 | pcom.cpp | PORT P1 | T | verify retail |
| PRT-25 | pcom bodyguard search 0x800→0x500 | pcom.cpp | PORT P1 | T | verify retail |
| PRT-26 | pcom PTIME→GAME_TURN revert | pcom.cpp | OPEN | M | determinism cluster |
| PRT-27 | pcom findcar gating simplified | pcom.cpp | PORT P1 | T | AI carjack frequency |
| PRT-28 | pcom navtokill home-zone branch drop | pcom.cpp | PORT P1 | T | logic |
| PRT-29 | pcom process_person lookat idle | pcom.cpp | PORT P1 | T | verify retail |
| PRT-30 | pcom SOUND_FIGHT bent test regression | pcom.cpp | OPEN | T | unclear if bug |
| PRT-31 | overlay viewport reset removal | overlay.cpp | SELF | T | наш OpenGL |
| PRT-32 | overlay POLY_flush_local_rot removal | overlay.cpp | SELF | T | наш фикс |
| PRT-33 | overlay grenade path un-gated | overlay.cpp | PORT P1 | T | balance |
| PRT-34 | overlay enemy health regression | overlay.cpp | OPEN | T | unclear |
| PRT-35 | overlay dead help_system block | overlay.cpp | SKIP | L | `#ifdef UNUSED` |
| PRT-36 | plat draw-mesh ASSERT removal | plat.cpp | PORT P2 | T | hardening |
| PRT-37 | PLAT_MAX_PLATS static 32 | plat.h | AUDIT | T | our value |
| PRT-38 | ribbon menu early-out removal | ribbon.cpp | OPEN | T | pair с PRT-14 |
| PRT-39 | barrel cone penalty re-enable | barrel.cpp | PORT P1 | T | verify |
| PRT-40 | dike dist shift conditional | dike.cpp | SKIP | T | dead guard |
| PRT-41 | spark colvect stub | spark.cpp | SKIP | T | dead |
| PRT-42 | ns cache_find_light dead | ns.cpp | SKIP | M | dead |
| PRT-43 | dirt tick type change | dirt.cpp | SKIP | T | cosmetic |
| PRT-44 | mist type_cycle BSS | mist.cpp | SKIP | T | cosmetic |
| PRT-45 | mist float→double | mist.cpp | SKIP | T | cosmetic |
| PRT-46 | pyro points delta double | pyro.cpp | SKIP | T | cosmetic |
| PRT-47 | ware MF_Fopen→fopen | ware.cpp | PORT P2 | T | API |
| PRT-48 | ware AENG_world_line debug | ware.cpp | SKIP | T | debug |
| PRT-49 | ware door frame `#if 0` | ware.cpp | SKIP | M | dead |
| PRT-50 | pcom add_gang_member off-by-one | pcom.cpp | DONE 2026-04-14 | T | bug fix (upto-1 → upto-2) |
| PRT-51 | pcom bodyguard shoot_time 4× removal | pcom.cpp | PORT P1 | T | balance |
| PRT-52 | pcom combo_display removal | pcom.cpp | SKIP | T | test HUD |
| PRT-53 | pcom timer_bored cast drop | pcom.cpp | SKIP | T | cosmetic |
| PRT-54 | pcom navtokill shoot stagger | pcom.cpp | OPEN | M | determinism |

## player_combat (CMB)

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| CMB-01 | PTIME → ComboHistory cascade | Person.cpp | OPEN | M | determinism cluster |
| CMB-02 | Vehicle mount flag split | Person.cpp | PORT P1 | T | verify retail |
| CMB-03 | PCOM_call_cop_to_arrest_me signature | Person.cpp | PORT P1 | T | API |
| CMB-04 | Hands-up civilians targeting removal | guns.cpp/Person.cpp | OPEN | T | verify |
| CMB-05 | Roper/tramp estate guard removal | Combat.cpp | PORT P1 | T | verify retail |
| CMB-06 | over_nogo/slippy guard removal (group) | Person.cpp | OPEN | M | guard cluster batch A |
| CMB-07 | Baseball-bat yomp anim cut | Person.cpp | PORT P1 | T | cut feature |
| CMB-08 | Hit wall anim re-enable | Person.cpp | PORT P1 | T | guard cluster |
| CMB-09 | Respawn/vanish timer tweaks | Person.cpp | PORT P1 | T | balance pack |
| CMB-10 | Grenade explosion tuning | grenade.cpp | PORT P1 | T | balance pack |
| CMB-11 | Damage value popup wiring | Combat.cpp/Person.cpp | PORT P1 | T | UI |
| CMB-12 | WaveParams positional sound | Person.cpp | SKIP | M | miniaudio |
| CMB-13 | check_limb_pos_on_ladder sig | Person.cpp | PORT P2 | T | cleanup |
| CMB-14 | Cable-fall damage + LOS flag | Person.cpp | PORT P1 | M | behavior |
| CMB-15 | Hit-from-behind roll 70→100 | Person.cpp | PORT P1 | T | balance pack |
| CMB-16 | find_best_grapple room text | Combat.cpp | PORT P1 | T | UI |
| CMB-17 | Crawl/prone anim collapse | Person.cpp | PORT P1 | T | logic |
| CMB-18 | Minor logic fixes group | Person.cpp/hook.cpp | PORT P1 | T | cleanup |
| CMB-V1 | Jump slope guards deleted | Person.cpp | OPEN | T | guard A |
| CMB-V2 | Pull-up NOGO guard deleted | Person.cpp | OPEN | T | guard A |
| CMB-V3 | Vault fence NOGO/wall safety | Person.cpp | OPEN | T | guard A |
| CMB-V4 | Gun-out run anim variant removed | Person.cpp | PORT P1 | T | anim |
| CMB-V5 | ko_recoil/recoil interrupt guards | Person.cpp | OPEN | M | guard A |
| CMB-V6 | fn_person_gun safety path | Person.cpp | INVERT-KEEP | T | pre-release safer |
| CMB-V7 | set_person_idle slippy guard | Person.cpp | OPEN | T | guard A |
| CMB-V8 | set_person_crawling fence slide | Person.cpp | OPEN | T | guard A |
| CMB-V9 | set_person_goto_xz default ASSERT | Person.cpp | PORT P2 | T | invariant |
| CMB-V10 | person_normal_move PAP_HIDDEN | Person.cpp | OPEN | M | guard B |
| CMB-V11 | general_process_player fight mode | Person.cpp | PORT P1 | T | guard A |
| CMB-V12 | find_arrestee scoring | Person.cpp | OPEN | T | verify retail |
| CMB-V13 | perform_arrest floating text | Person.cpp | PORT P1 | T | UI |
| CMB-V14 | fn_person_idle dead target | Person.cpp | PORT P1 | T | guard A |
| CMB-V15 | fn_person_search target keep | Person.cpp | PORT P1 | T | guard A |
| CMB-V16 | set_person_shoot target keep | Person.cpp | PORT P1 | T | guard A |
| CMB-V17 | set_person_croutch NON_INT_M | Person.cpp | PORT P1 | T | guard A |
| CMB-V18 | set_person_enter_vehicle text | Person.cpp | PORT P1 | T | UI |
| CMB-V19 | slippy_slope default case removal | Person.cpp | PORT P1 | T | invariant |
| CMB-V20 | fn_person_dead respawn civ branch | Person.cpp | PORT P1 | T | cleanup |
| CMB-V21 | fn_person_dying on_face clear | Person.cpp | PORT P1 | T | logic |
| CMB-V22 | fn_person_dying Timer1 increment | Person.cpp | PORT P1 | T | timing |
| CMB-V23 | fn_person_fighting think formula | Person.cpp | OPEN | T | determinism |
| CMB-V24 | person_holding_2handed bat cleanup | Person.cpp | PORT P2 | T | cosmetic |
| CMB-V25 | set_person_arrest Roper voice | Person.cpp | PORT P1 | T | audio |
| CMB-V26 | create_person softfail + PTIME seed | Person.cpp | PART | T | softfail yes, PTIME open |
| CMB-V27 | set_person_dead InsideRoom keep | Person.cpp | OPEN | T | verify |
| CMB-D1 | fn_person_moveing anim→yomp fixup | Person.cpp | OPEN | M | guard C |
| CMB-D2 | SUB_STATE_WALKING footstep deletion | Person.cpp | INVERT-KEEP | T | new/ regression |
| CMB-D3 | SKID_STOP velocity clear removal | Person.cpp | PORT P1 | T | guard A |
| CMB-D4 | Walking slope_ahead guard | Person.cpp | OPEN | T | guard B |
| CMB-D5 | Moveing slippy guards multi | Person.cpp | OPEN | T | guard B |
| CMB-D6 | Crawling switch uncomment | Person.cpp | SKIP | T | cosmetic |
| CMB-D7 | dead_tween global removal | Person.cpp | SKIP | T | dead |
| CMB-D8 | save_psx extern | Person.cpp | SKIP | T | save-load |
| CMB-D9 | FPM track ifdef | Person.cpp | SKIP | T | feature flag |
| CMB-E1 | Bat yomp removal extended | Person.cpp | PORT P1 | T | pair CMB-07 |
| CMB-E2 | **headbutt p_person copy-paste bug** | Person.cpp | INVERT-KEEP | T | new/ bug |
| CMB-E3 | person_death_slide sdist cleanup | Person.cpp | PORT P1 | T | cleanup |
| CMB-E4 | slippy_slope size literal | Person.cpp | SKIP | T | cosmetic |
| CMB-E5 | Arrest struggle tick formula | Person.cpp | OPEN | T | determinism |

## ai_npcs (AI)

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| AI-01 | Thing Timer1 + BuildingList union + CameraMan | Thing.h | AUDIT | T | должно быть у нас |
| AI-02 | **Follower fall damage immunity removal** | Darci.cpp | PORT P0 | T | empty branch |
| AI-03 | projectile_move NO_FLOOR + plunge stagger | Darci.cpp | PORT P1 | T | Roper floating fix |
| AI-04 | **Balrog PAP_FLAG_HIDDEN collision** | bat.cpp | DONE 2026-04-14 | T | весь блок удалён |
| AI-05 | process_things noise+arrest+PTIME removal | Thing.cpp | OPEN | M | связан с API simplification |
| AI-06 | THING_find_sphere hit_player removal | Thing.cpp | SKIP | T | feature cut |
| AI-07 | process_things NextLink move | Thing.cpp | INVERT-KEEP | T | pre safer |
| AI-08 | **SPECIAL_TREASURE dedup + 71↔81 swap** | Special.cpp | N/A 2026-04-14 | M | дубликата нет; ObjectId 71/94/81/39 уже matches release |
| AI-09 | Special drop pointer cleanup removal | Special.cpp | OPEN | T | UAF risk |
| AI-10 | Shotgun/AK47 scatter tick stagger | Special.cpp | PORT P1 | T | |
| AI-11 | SPECIAL text floats | Special.cpp | PORT P1 | T | UI |
| AI-12 | animal.h pigeon support | animal.h/cpp | AUDIT | M | save layout impact |
| AI-13 | MAV_find_building_entrance `#ifdef UNUSED` | mav.cpp | SKIP | T | dead |
| AI-V1 | calc_things_height return type | Darci.cpp | SKIP | T | API |
| AI-V2 | INPUT_TYPE_ALL literal | Thing.cpp | SKIP | T | cosmetic |
| AI-V3 | BAT_change_state floating label | bat.cpp | SKIP | T | dev debug |

## vehicles (VEH)

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| VEH-01 | Shadow/headlights SOFTWARE gate | Vehicle.cpp | SKIP | T | software |
| VEH-02 | **Car drawmesh crumple 0xff→0** | Vehicle.cpp | PORT P0 | T | crumple visible |
| VEH-03 | Analog steering revert to inc | Vehicle.cpp | PORT P1 | M | verify feel |
| VEH-04 | **Siren no-driver guard removal** | Vehicle.cpp | DONE (already applied) | T | 2026-04-14: наш код уже имеет `&& Driver` guard (matches release) |
| VEH-05 | Pedals signature cleanup | Vehicle.cpp | PORT P2 | T | cleanup |
| VEH-06 | Anim-prim collision `#ifdef` | Vehicle.cpp | SKIP | M | WIP feature |
| VEH-07 | Engine noise WaveParams partial | Vehicle.cpp | SKIP | M | dead/partial |
| VEH-08 | **Chopper MFX_QUEUED drop** | chopper.cpp | PORT P0 | T | clean loop |
| VEH-09 | Chopper landing double math | chopper.cpp | SKIP | T | accident |
| VEH-10 | **WMOVE_remove(class) bulk** | wmove.cpp | DONE 2026-04-14 | M | новая функция добавлена |
| VEH-11 | **WMOVE_relative_pos overflow fix** | wmove.cpp | DONE 2026-04-14 | L | shifts, split Y, removed clamps |
| VEH-12 | CAM_get_psx export | cam.h | SKIP | T | PSX |

## ui_frontend (UI) — heavily inverted

All inverted items need per-item audit (grep + retail behavior + replacement check) before decision.

### panel / font / menufont

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| UI-01 | panel depth-bodge/screensaver removal | panel.cpp | SELF | M | OpenGL не нужен |
| UI-02 | panel origin + health clamp + AK47 + wraparound | panel.cpp | AUDIT | M | inverted, есть ли у нас |
| UI-03 | panel karma/compass/press-button helpers | panel.cpp | PORT P1 | T | new HUD features |
| UI-04 | panel sw_hack shadow gating | panel.cpp | SKIP | T | software |
| UI-05 | font2d dynamic bitmap + bDontDraw | font2d.cpp | AUDIT | T | inverted |
| UI-06 | font2d 0xAC non-breaking space | font2d.cpp | AUDIT | T | inverted, French |
| UI-07 | font2d strikethrough caching | font2d.cpp | OPEN | T | verify |
| UI-08 | menufont GLIMMER/SHAKE/HALOED | menufont.cpp | PORT P1 | M | title screen effects |

### interfac

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| UI-09 | NOISE_TOLERANCE 8192→1024 | interfac.cpp | PORT P1 | T | correct tuning |
| UI-10 | force_walk flag | interfac.cpp | AUDIT | T | inverted |
| UI-11 | Camera-jump turn reduction | interfac.cpp | AUDIT | T | inverted |
| UI-12 | player_relative camera mode | interfac.cpp | AUDIT | T | inverted |
| UI-13 | should_i_jump helper | interfac.cpp | AUDIT | T | inverted |
| UI-14 | Stern Revenge semtex skip | interfac.cpp | AUDIT | T | inverted |
| UI-15 | menu-input API exposure | interfac.cpp | AUDIT | M | inverted |
| UI-16 | joypad polling BUTTON_IS_PRESSED | interfac.cpp | SKIP | T | SDL |
| UI-17 | Vehicle steering smoothing | interfac.cpp | AUDIT | T | inverted |
| UI-18 | EWAY subcondition parameter | interfac.cpp | AUDIT | T | inverted |

### gamemenu / attract

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| UI-19 | music blocking level-end wait | gamemenu.cpp | AUDIT | T | inverted |
| UI-20 | pause menu SFX cutoff | gamemenu.cpp | AUDIT | T | inverted |
| UI-21 | menu-selection wrap logic | gamemenu.cpp | AUDIT | T | inverted |
| UI-22 | Attract language-change reinit | Attract.cpp | AUDIT | M | inverted |
| UI-23 | Attract D3D state setup block | Attract.cpp | SKIP | T | D3D |
| UI-24 | Attract mucky_times table | Attract.cpp | PORT P1 | T | data |
| UI-25 | Attract cheat-punishment flag | Attract.cpp | AUDIT | T | inverted |
| UI-26 | Attract sound cleanup | Attract.cpp | AUDIT | T | inverted |

### widget / xlat / language

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| UI-27 | widget MemAlloc→malloc | widget.cpp | SKIP | T | API |
| UI-28 | widget unsigned cast INPUT_Char | widget.cpp | AUDIT | T | inverted |
| UI-29 | xlat_str asserts removal + TRACE | xlat_str.cpp | PORT P2 | T | production safe |
| UI-30 | lang_english.txt VMU cleanup | lang_english.txt | PORT P1 | T | data |

### frontend.cpp — большой блок

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| UI-31 | auto-play FMV timeout | frontend.cpp | AUDIT | T | inverted |
| UI-32 | language-aware speech directory | frontend.cpp | AUDIT | T | inverted |
| UI-33 | mission-brief @→\r | frontend.cpp | AUDIT | T | inverted |
| UI-34 | default title "Urban Chaos" + trim | frontend.cpp | AUDIT | T | inverted |
| UI-35 | **FRONTEND_MissionHierarchy comparator direction** | frontend.cpp | AUDIT | M | inverted, critical |
| UI-36 | Hierarchy district-ready gate | frontend.cpp | AUDIT | T | inverted |
| UI-37 | French breakout hide | frontend.cpp | AUDIT | T | inverted |
| UI-38 | find_savegames grey-out | frontend.cpp | AUDIT | T | inverted UX |
| UI-39 | save/load return values | frontend.cpp | AUDIT | T | inverted |
| UI-40 | FRONTEND_mode transition + stackpos reset | frontend.cpp | AUDIT | M | inverted |
| UI-41 | FRONTEND_recenter_menu helper | frontend.cpp | AUDIT | T | inverted |
| UI-42 | French big font scale | frontend.cpp | AUDIT | T | inverted |
| UI-43 | NTSC Y offset | frontend.cpp | SKIP | T | NTSC |
| UI-44 | Strikethrough parameterization | frontend.cpp | AUDIT | T | inverted |
| UI-45 | FRONTEND_shadowed_text helper | frontend.cpp | AUDIT | T | inverted |
| UI-46 | Kibble alpha override | frontend.cpp | AUDIT | T | inverted |
| UI-47 | Slider alpha 192 vs 128 | frontend.cpp | OPEN | T | verify retail |
| UI-48 | DrawMulti colour parameter | frontend.cpp | AUDIT | T | inverted |
| UI-49 | store_video_data int + init | frontend.cpp | AUDIT | T | inverted |
| UI-50 | FRONTEND_init music reset | frontend.cpp | AUDIT | M | inverted |
| UI-51 | FRONTEND_init kibble bFirstTime | frontend.cpp | AUDIT | T | inverted |
| UI-52 | FRONTEND_init mission_launch early-out | frontend.cpp | OPEN | T | verify |
| UI-53 | FRONTEND_init kibble halving | frontend.cpp | SKIP | T | perf |
| UI-54 | FRONTEND_sound wind bug | frontend.cpp | AUDIT | T | inverted |
| UI-55 | **whattoload[] 36-entry mission table** | frontend.cpp | AUDIT | L | inverted critical |
| UI-56 | MFX_QUICK_stop on mission launch | frontend.cpp | AUDIT | T | inverted |
| UI-57 | FRONTEND_display viewport setup | frontend.cpp | SKIP | T | D3D |
| UI-58 | OT_PADMOVE HUD preview | frontend.cpp | AUDIT | M | inverted, pairs UI-02 |
| UI-59 | FRONTEND_input multi Enter cycle | frontend.cpp | AUDIT | T | inverted |
| UI-60 | FRONTEND_input joypad rebind guard | frontend.cpp | AUDIT | T | inverted |
| UI-61 | FRONTEND_input pad-cancel in ESC | frontend.cpp | AUDIT | T | inverted |
| UI-62 | FRONTEND_input ESC commit + queue back | frontend.cpp | AUDIT | T | inverted |
| UI-63 | FRONTEND_input joypad grab pressed bit | frontend.cpp | SKIP | T | SDL |
| UI-64 | FRONTEND_input Left/Right order | frontend.cpp | AUDIT | T | inverted |
| UI-65 | FRONTEND_level_won cheat gate + kibble | frontend.cpp | AUDIT | T | inverted |
| UI-66 | FRONTEND_level_lost kibble reinit | frontend.cpp | AUDIT | T | inverted |
| UI-67 | screenfull init/cleanup | frontend.cpp | AUDIT | T | inverted |
| UI-68 | FRONTEND_scr_new_theme music pause | frontend.cpp | AUDIT | T | inverted |
| UI-69 | FRONTEND_scr_new_theme last=1 init | frontend.cpp | SKIP | T | minor |

## platform_host (PLT)

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| PLT-01 | DirectInput 8 DIManager | DIManager.cpp | SKIP | L | SDL |
| PLT-02 | Video/driver selection system | GDisplay/DDManager | SKIP | L | single GL path |
| PLT-03 | Bink filename macros rename | GDisplay.cpp | SKIP | T | cosmetic |
| PLT-04 | TGA_load_psx palette loader | Tga.cpp | SKIP | M | зависит от PLT-05 |
| PLT-05 | 8-bit palettized texture subsystem | GDisplay/D3DTexture/Tga | SKIP | L | OpenGL не использует |
| PLT-06 | Display::Flip depth-bodge removal | GDisplay.cpp | SKIP | T | OpenGL |
| PLT-07 | RunCutscene parameter simplification | GDisplay | SKIP | T | API |
| PLT-08 | Display MenuOn/Off Win32 menu bar | GDisplay | SKIP | T | SDL |
| PLT-09 | ChangeDriver/ChangeDevice | GDisplay | SKIP | M | single path |
| PLT-10 | init_best_found language env | GDisplay.cpp | AUDIT | T | language env key |
| PLT-11 | GHost debug-break `_asm int 3` | GHost.cpp | SKIP | T | own break |
| PLT-12 | MF_Fopen → fopen | Drive/FileClump/Tga | PORT P2 | T | cleanup |
| PLT-13 | CD LocalInstall default 1→0 | Drive.cpp | SKIP | T | not applicable |
| PLT-14 | GKeyboard autorepeat | GKeyboard.cpp | SKIP | T | SDL |
| PLT-15 | DDManager driver selection | DDManager.cpp | SKIP | T | single path |
| PLT-16 | A3DManager stub-out | A3DManager.h | SKIP | T | own audio |
| PLT-17 | QSManager Miles stub | QSManager.h | SKIP | T | own audio |
| PLT-18 | DDlib pragmas + includes | DDlib.h | SKIP | T | DX libs |
| PLT-19 | GDisplay header cleanups | GDisplay.h | SKIP | T | D3D |
| PLT-20 | D3DTexture header cleanups | D3DTexture.h | SKIP | T | D3D |
| PLT-21 | outro/Tga.h prefix normalization | outro/Tga.h | SKIP | T | outro |

## added_deleted (ADD)

| ID | Tag | File | Dec | Size | Notes |
|---|---|---|---|---|---|
| ADD-01 | sw.cpp / tom.cpp software rasterizer | sw.cpp/tom.cpp | SKIP | L | OpenGL only |
| ADD-02 | FFManager force feedback | FFManager.cpp/h | SKIP | T | own haptics path |
| ADD-03 | farfacet deletion | farfacet.cpp/h | AUDIT | L | **мы осознанно держим** |
| ADD-04 | superfacet deletion | superfacet.cpp/h | AUDIT | L | dead в нашей базе? |
| ADD-05 | fastprim deletion | fastprim.cpp/h | AUDIT | L | dead в нашей базе? |
| ADD-06 | flamengine deletion | flamengine.cpp/h | AUDIT | M | dead both sides |
| ADD-07 | build.cpp deletion (header orphan) | build.cpp/h | AUDIT | M | BUILD_draw call sites |
| ADD-08 | AsyncFile deletion | AsyncFile*.* | SKIP | T | dev scaffold |
| ADD-09 | DCLowLevel.h deletion | DCLowLevel.h | SKIP | T | DC |
| ADD-10 | BinkClient.cpp deletion | BinkClient.cpp | SKIP | T | dead stub |
| ADD-11 | net.cpp deletion | net.cpp | SKIP | T | network cut |
| ADD-12 | outro/os.cpp / wire.cpp | outro/os.cpp | SKIP | L | dev tool |
| ADD-13 | outline.h deletion | outline.h | SKIP | T | Stage-2 artifact |
| ADD-14 | america.h / demo.h feature flags | | SKIP | T | disabled |
| ADD-15 | noserver.h | noserver.h | SKIP | T | confirmed cut |
| ADD-16 | bike.h placeholder | bike.h | SKIP | T | empty |
| ADD-17 | Camera.h gameplay entity | Camera.h | AUDIT | T | verify presence |
| ADD-18 | font3d.h `#if 0` | font3d.h | SKIP | T | dead |
| ADD-19 | snd_type.h Miles selector | snd_type.h | SKIP | T | own |
| ADD-20 | resource.h MSVC rc | resource.h | SKIP | T | tooling |

## misc (MSC)

Пустой раздел — 0 находок. Все 17 файлов cosmetic (header guards, trailing newlines, bool→SLONG renames, debug TRACE).

---

## Summary counts by decision

- **PORT P0** (реальные баги): ~30 findings — см. plan.md блоки P0
- **PORT P1** (видимые улучшения): ~80
- **PORT P2** (cleanup, trivial): ~25
- **OPEN** (обсуждение при ревью): ~20
- **AUDIT** (проверить текущее состояние): ~90 (в основном UI inverted)
- **SKIP** (out-of-scope): ~140
- **SELF / INVERT-KEEP / DONE** (наш путь или уже сделано): ~20
- **Total:** ~405

---

## Как этот файл использовать при работе

1. При ревью плана пользователь утверждает **блоки** из `plan.md`, не отдельные ID.
2. Когда доходим до порта конкретного блока, беру список ID из `plan.md`, смотрю полные описания здесь и в исходных `release_changes/*.md`.
3. Для AUDIT пунктов — запускаю grep через subagent batches (не больше 2-3 одновременно), заполняю колонку статуса результатом.
4. После принятия решения по OPEN пункту — меняю Decision в этой таблице и ссылку в plan.md.
