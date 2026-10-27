# Этап 4.2 — Описания файлов

Описания всех файлов в `src/` (относительно `src/`). _globals файлы не описываются отдельно.

---

## main.cpp

| Файл | Описание |
|------|----------|
| `main.cpp` | Win32 WinMain entry point: calls HOST_run() |

---

## engine/core/

| Файл | Описание |
|------|----------|
| `engine/core/types.h` | Fundamental integer typedefs (UBYTE, SLONG, etc.), MFPoint, MFRect, UC_TRUE/UC_FALSE |
| `engine/core/macros.h` | General-purpose utility macros: min/max, WITHIN, SATURATE, SWAP, PI, UC_INFINITY |
| `engine/core/fixed_math.h` | 16.16 fixed-point arithmetic via inline x86 assembly: MUL64 and DIV64 |
| `engine/core/math.h/.cpp` | PSX angle trig API: SIN/COS tables, Arctan, Root, 2D segment intersection |
| `engine/core/vector.h` | 3D integer vector types: SVector and GameCoord (both SLONG X/Y/Z) |
| `engine/core/matrix.h/.cpp` | Float 3x3 rotation matrix API (MATRIX_calc, MATRIX_vector, MATRIX_skew) |
| `engine/core/fmatrix.h/.cpp` | Integer 3x3 matrix types and PSX fixed-point matrix operations |
| `engine/core/quaternion.h/.cpp` | CQuaternion class and FloatMatrix for SLERP-based rotation interpolation |
| `engine/core/memory.h/.cpp` | Custom heap allocator: Win32 HeapCreate/HeapAlloc (8 MB, zero-init, 4-byte aligned) |
| `engine/core/heap.h/.cpp` | Fixed-size 128 KB heap with free-list defragmentation |
| `engine/core/timer.h/.cpp` | High-resolution stopwatch via Win32 QueryPerformanceCounter |
| `engine/core/pq.h` | Generic binary min-heap (priority queue) template, used by A* in mav.cpp |

---

## engine/platform/

| Файл | Описание |
|------|----------|
| `engine/platform/uc_common.h` | Main platform abstraction header: Windows/DirectX includes, core types, host API |
| `engine/platform/host.h/.cpp` | Win32 application host: HOST_run, SetupHost/ResetHost, ShellPause threading |
| `engine/platform/wind_procs.h/.cpp` | Win32 window procedure: activation, resize/move, input handling |

---

## engine/graphics/geometry/

| Файл | Описание |
|------|----------|
| `engine/graphics/geometry/aa.h/.cpp` | Anti-aliased triangle rasterizer: coverage-weighted alpha mask into bitmap |
| `engine/graphics/geometry/cone.h/.cpp` | Cone geometry renderer: creates, clips, renders spotlight cones as triangle fans |
| `engine/graphics/geometry/facet.h/.cpp` | Exterior building wall/roof renderer: fast-path walls, doors, fences, walkable roofs |
| `engine/graphics/geometry/farfacet.h/.cpp` | LOD distant-building wall renderer: pre-built D3D vertex buffers, view-frustum culled |
| `engine/graphics/geometry/fastprim.h/.cpp` | Fast prim renderer: cached D3D vertex/index buffers per prim object |
| `engine/graphics/geometry/figure.h/.cpp` | D3D character mesh renderer: MS strip-optimizer, bone-skinned figure draw |
| `engine/graphics/geometry/mesh.h/.cpp` | Static prim mesh renderer: world-space draw with crumple/damage deformation |
| `engine/graphics/geometry/oval.h/.cpp` | Soft oval/square shadow projector: blob shadow quad on heightfield |
| `engine/graphics/geometry/shape.h/.cpp` | Primitive shape renderer: spheres, lines, tripwires, balloons via POLY pipeline |
| `engine/graphics/geometry/sky.h/.cpp` | Sky renderer: star generation/draw and animated cloud quad rendering |
| `engine/graphics/geometry/sprite.h/.cpp` | World-space billboard sprite renderer: square quads with UV region support |
| `engine/graphics/geometry/superfacet.h/.cpp` | Batched building-wall renderer: per-texture-page vertex buffer, single DrawIndexedPrimitive |

---

## engine/graphics/graphics_api/

| Файл | Описание |
|------|----------|
| `engine/graphics/graphics_api/dd_manager.h/.cpp` | DirectDraw/Direct3D driver enumeration and device-info structures |
| `engine/graphics/graphics_api/gd_display.h` | GDisplay class declaration: D3D/DD display, flags, resolution constants |
| `engine/graphics/graphics_api/display.cpp` | GDisplay class implementation: D3D device lifecycle, surfaces, viewport, mode changes |
| `engine/graphics/graphics_api/display_macros.h` | Convenience macros forwarding to the_display methods (BEGIN_SCENE, FLIP, etc.) |
| `engine/graphics/graphics_api/d3d_texture.h/.cpp` | D3D texture management: fast-load scratch buffer, pixel-format helpers |
| `engine/graphics/graphics_api/render_state.h/.cpp` | RenderState: full D3D render state with cached diff-only updates |
| `engine/graphics/graphics_api/vertex_buffer.h/.cpp` | D3D hardware vertex buffer pool: VB_Init/Term, lock/unlock/flush |
| `engine/graphics/graphics_api/work_screen.h/.cpp` | Work screen (2D overlay surface): lock/unlock, show, clear, coordinate mapping |

---

## engine/graphics/pipeline/

| Файл | Описание |
|------|----------|
| `engine/graphics/pipeline/aeng.h/.cpp` | Main scene rendering engine: world transform, scene draw entry point, crinkle shadow |
| `engine/graphics/pipeline/bucket.h/.cpp` | Z-sort bucket system: depth-sorted polygon submission pool |
| `engine/graphics/pipeline/draw2d.h/.cpp` | 2D screen-space drawing: filled boxes, triangles, textured quads for UI |
| `engine/graphics/pipeline/engine_types.h` | Core 3D math types for renderer: M31 float vector, transform matrices, engine state |
| `engine/graphics/pipeline/poly.h/.cpp` | Polygon attribute flags, POLY_Point struct, texture-page IDs, poly submission API |
| `engine/graphics/pipeline/poly_render.h/.cpp` | Render-state initialization for texture flags |
| `engine/graphics/pipeline/polypage.h/.cpp` | PolyPage class: per-texture-page vertex buffer + sort list, bucket-sort |
| `engine/graphics/pipeline/polypoint.h` | PolyPoint2D: D3DTLVERTEX wrapper for screen coords, UV, colour |
| `engine/graphics/pipeline/qeng.h/.cpp` | Minimal 3D engine for QMAP level-editor rendering (not used in shipped game) |

---

## engine/graphics/postprocess/

| Файл | Описание |
|------|----------|
| `engine/graphics/postprocess/bloom.h/.cpp` | Lens-flare / light bloom: 27-quad flare spread along screen axis, glow and beam |
| `engine/graphics/postprocess/wibble.h/.cpp` | Framebuffer wibble effect: horizontal row-shift distortion |

---

## engine/graphics/lighting/

| Файл | Описание |
|------|----------|
| `engine/graphics/lighting/cache.h` | Small fixed-size per-object lighting cache |
| `engine/graphics/lighting/crinkle.h/.cpp` | Surface bump-map approximation: dense triangle mesh from .sex/.txc files |
| `engine/graphics/lighting/ed_light.h` | Level-editor light struct (ED_Light): baked point lights at load time |
| `engine/graphics/lighting/gamut.h` | View frustum gamut: radial grid cells for AENG geometry culling |
| `engine/graphics/lighting/light.h` | Runtime light data: LIGHT_Save on-disk format, dynamic light pool |
| `engine/graphics/lighting/ngamut.h/.cpp` | Narrow-gamut visibility: 128-cell per-row xmin/xmax spans for occlusion |
| `engine/graphics/lighting/night.h/.cpp` | Per-object dynamic lighting: static point lights, per-vertex colour computation |
| `engine/graphics/lighting/shadow.h/.cpp` | Static directional shadow: 3-bit shadow values per map square, LOS ray cast |
| `engine/graphics/lighting/smap.h/.cpp` | Shadow map projection: person body silhouette bitmap onto world polygons |

---

## engine/graphics/text/

| Файл | Описание |
|------|----------|
| `engine/graphics/text/font.h/.cpp` | Legacy pixel-font system: 8x9 fixed-size font with shadow |
| `engine/graphics/text/font2d.h/.cpp` | 2D proportional font: scans atlas TGA for per-character UV/width |
| `engine/graphics/text/menufont.h/.cpp` | Stylised menu font: glow, sine-wave, halo, shake effects |
| `engine/graphics/text/text.h/.cpp` | Proportional bitmap text renderer: D3D texture-mapped quads |
| `engine/graphics/text/truetype.h/.cpp` | TrueType font renderer (optional): TextCommand cache, MBCS-aware |

---

## engine/io/

| Файл | Описание |
|------|----------|
| `engine/io/async_file.h/.cpp` | Async file loader: background worker thread, queue up to 16 files |
| `engine/io/env.h/.cpp` | INI config file reader/writer: get/set string and number by section |
| `engine/io/file.h/.cpp` | Low-level file I/O: MFFileHandle (Windows HANDLE wrapper), open/read/seek/close |

---

## engine/audio/

| Файл | Описание |
|------|----------|
| `engine/audio/mfx.h/.cpp` | 3D positional audio engine (OpenAL): channel IDs, wave playback, talk files |
| `engine/audio/music.h/.cpp` | Music mode system: DRIVING/FIGHTING modes, per-frame fade/transition |
| `engine/audio/sound.h/.cpp` | High-level sound API: ambient, weather, wind, world biome types, 3D positional playback |
| `engine/audio/soundenv.h/.cpp` | 3D audio environment geometry stubs (sewer quad detection disabled) |

---

## engine/input/

| Файл | Описание |
|------|----------|
| `engine/input/joystick.h/.cpp` | DirectInput joystick: enumerate/acquire/poll via IDirectInputDevice2 |
| `engine/input/keyboard.h/.cpp` | DirectInput keyboard: scancode constants, key-state read |
| `engine/input/mouse.h/.cpp` | Mouse input: RecenterMouse and mouse-state globals |

---

## engine/animation/

| Файл | Описание |
|------|----------|
| `engine/animation/anim_types.h` | Animation type definitions: keyframe structs, fight collision data, creature class limits |
| `engine/animation/morph.h/.cpp` | Vertex morph keyframes for pigeon mesh: loads .ASC files, interpolates poses |

---

## engine/effects/

| Файл | Описание |
|------|----------|
| `engine/effects/flamengine.h/.cpp` | 2D cellular-automaton fire simulation: 256x256 field, uploads to D3D texture for menu flame |
| `engine/effects/psystem.h/.cpp` | Particle pool (2048 particles): smoke, sparks, blood, fire; doubly-linked used/free lists |

---

## engine/physics/

| Файл | Описание |
|------|----------|
| `engine/physics/collide.h/.cpp` | Collision detection: linear barriers (COL_VECT), walkable surfaces, per-cell lists |
| `engine/physics/hm.h/.cpp` | Hypermatter spring-lattice soft-body physics: barrels/boxes as mass-point grids |
| `engine/physics/sm.h/.cpp` | Sphere-Matter jelly physics: elastic sphere-link simulation for deformable objects |

---

## engine/console/

| Файл | Описание |
|------|----------|
| `engine/console/console.h/.cpp` | On-screen console: timed message queue, positional messages |
| `engine/console/message.h/.cpp` | On-screen debug message overlay: MSG_add queue, MSG_draw render |

---

## engine/compression/

| Файл | Описание |
|------|----------|
| `engine/compression/compression.h/.cpp` | Video frame delta compression: calc/apply/undo delta between 64x64 tiles |
| `engine/compression/image_compression.h/.cpp` | S3-style block texture compression: 4x4 pixel blocks |

---

## game/

| Файл | Описание |
|------|----------|
| `game/startup.cpp` | Application entry point: reads config, calls game startup loop |
| `game/game.h/.cpp` | Top-level game API: game_startup/init/fini, map draw, global_load |
| `game/game_types.h` | Central game-state header: Game struct, GS_/GF_ constants, the_game |
| `game/game_tick.h/.cpp` | Per-frame game controller: inventory, music context, danger level, debug console |
| `game/input_actions.h/.cpp` | Player input: hardware constants, INPUT_MASK_*/ACTION_* dispatch |
| `game/network_state_globals.h/.cpp` | Stub network state: multiplayer removed, always single-player defaults |

---

## things/core/

| Файл | Описание |
|------|----------|
| `things/core/thing.h/.cpp` | Thing — base game object: alloc/free/process_things, per-class dispatch |
| `things/core/hierarchy.h/.cpp` | Hierarchy — skeletal bone tree (15 bones), body_part tables, world-offset |
| `things/core/player.h/.cpp` | Player — per-player input/stats, PLAYER_* type IDs, MAX_PLAYERS |
| `things/core/state.h/.cpp` | State — StateFunction/GenusFunctions dispatch, set_state_function |
| `things/core/statedef.h` | StateDef — STATE_* constants (INIT, NORMAL, FIGHTING, DEAD, etc.) |
| `things/core/drawtype.h/.cpp` | DrawType — DT_* render modes, DrawTween/DrawMesh pool alloc/free |
| `things/core/common.h` | COMMON() macro: shared header fields for genus structs |
| `things/core/interact.h/.cpp` | Interact — grab detection for edges/cables/ladders, bone attachment |
| `things/core/switch.h/.cpp` | Switch — trigger volumes (types, scan modes, flags), alloc/free/process |

---

## things/characters/

| Файл | Описание |
|------|----------|
| `things/characters/person_types.h` | Person struct, PERSON_* type IDs, ANIM_TYPE_*, FLAG_PERSON_* flags |
| `things/characters/person.h/.cpp` | Person — full API: animation, death, grab/fence, health, dispatch tables |
| `things/characters/anim_ids.h` | Animation index constants (CIV_M_*, COP_*, DARCI_*), body part indices |
| `things/characters/darci.h/.cpp` | Darci Mason player character: physics, movement, state init |
| `things/characters/cop.h/.cpp` | Cop — NPC state function declarations and handlers |
| `things/characters/roper.h/.cpp` | Roper — second playable character: init and normal-state handler |
| `things/characters/thug.h/.cpp` | Thug — NPC state handlers (init guard via ASSERT(0)) |
| `things/characters/snipe.h/.cpp` | Snipe — rifle scope mode: on/off, turn, process |

---

## things/vehicles/

| Файл | Описание |
|------|----------|
| `things/vehicles/vehicle.h/.cpp` | Vehicle — ground physics, VehInfo, collision, suspension, grip |
| `things/vehicles/chopper.h/.cpp` | Chopper — helicopter: flight AI state machine, physics |
| `things/vehicles/furn.h` | Furniture struct for moving world objects (doors, crates, cars) |

---

## things/items/

| Файл | Описание |
|------|----------|
| `things/items/barrel.h/.cpp` | Barrel — two-sphere physics, NORMAL/CONE/BURNING/BIN types |
| `things/items/balloon.h/.cpp` | Balloon — 4-point rope simulation, attachment to person bone |
| `things/items/grenade.h/.cpp` | Grenade — physics simulation, trajectory preview, explosion |
| `things/items/hook.h/.cpp` | Hook — grappling hook rope (256 points), spin/release/reel |
| `things/items/special.h/.cpp` | Special — pickup items: spawn, collect, drop, process |

---

## things/animals/

| Файл | Описание |
|------|----------|
| `things/animals/animal.h/.cpp` | Animal — Animal struct (canid species), stub dispatch |
| `things/animals/bat.h/.cpp` | Bat — flight AI, attack, particle effects, collision |

---

## map/

| Файл | Описание |
|------|----------|
| `map/pap.h/.cpp` | Two-tier heightfield map (128x128 hi / 32x32 lo cells): height/bounds query |
| `map/map.h/.cpp` | MapElement struct (128x128 world grid cell with colour/lighting), floor-flag constants |
| `map/supermap.h/.cpp` | Supermap: pre-computed spatial index of buildings, facets, walkables from .pam file |
| `map/road.h/.cpp` | Road node/edge graph: surface codes, junction constants, camber/curb |
| `map/ob.h/.cpp` | Static map objects (lamp posts, benches, hydrants): types, flags, rendering |
| `map/qmap.h/.cpp` | QMAP quick-map: block-level renderer (roads, cubes, prims) for editor/debug |
| `map/sewers.h/.cpp` | Sewer/cavern subsystem: underground map cells, floor/wall strip geometry |
| `map/map_constants.h` | Editor-side map constants: ELE_SHIFT, DepthStrip type |
| `map/level_pools.h` | Extern declarations for all level-load-time geometry pools |

---

## buildings/

| Файл | Описание |
|------|----------|
| `buildings/building_types.h` | Building data-model types: FBuilding/FStorey/FWall, pool limits |
| `buildings/building.h/.cpp` | Building geometry construction: reads FBuilding data, emits renderable prims |
| `buildings/build.h/.cpp` | Building renderer: per-building poly submission with per-vertex lighting |
| `buildings/build2.h/.cpp` | City spatial index: facet/walkable registration, ladder geometry, city rebuild |
| `buildings/prim_types.h` | Prim geometry types: PrimPoint/Face3/Face4/Object/Normal, pool limits |
| `buildings/prim.h/.cpp` | Prim (static world mesh): pool management, lighting, collision, normals |
| `buildings/stair.h/.cpp` | Stair placement algorithm: bounding-box edge analysis, stair-slot assignment |
| `buildings/outline.h/.cpp` | Grid outline utility: orthogonal polygon overlap test for building placement |
| `buildings/id.h` | Interior design: floor-plan generation (rooms, furniture, stairs) for storeys |
| `buildings/ware.h/.cpp` | Warehouse: large multi-entrance indoor areas with navigation data |

---

## navigation/

| Файл | Описание |
|------|----------|
| `navigation/walkable.h/.cpp` | Walkable surface query: find face under point, height-on-quad, point-in-quad |
| `navigation/wmove.h/.cpp` | Moving walkable faces: virtual quads on vehicles/platforms for standing characters |
| `navigation/inside2.h/.cpp` | Building interior navigation: storey/room grid, door/stair cell flags |
| `navigation/wand.h/.cpp` | Wander navigation: marks squares near roads for NPC roaming, spawn-point search |

---

## world_objects/

| Файл | Описание |
|------|----------|
| `world_objects/door.h/.cpp` | Door slot system: facet open/close, slide animation |
| `world_objects/plat.h/.cpp` | Moving platforms: waypoint-patrol for CLASS_PLAT Things |
| `world_objects/tripwire.h/.cpp` | Tripwire traps: collision detection, activation, counter management |
| `world_objects/puddle.h/.cpp` | Puddle system: rain puddles along building/road edges |
| `world_objects/dirt.h/.cpp` | Ambient debris: 1024-entry pool of leaves, cans, pigeons, blood, snow, sparks |

---

## effects/weather/

| Файл | Описание |
|------|----------|
| `effects/weather/fog.h/.cpp` | Fog sprite system: animated patches near camera, wind-gust support |
| `effects/weather/mist.h/.cpp` | Animated quad-patch mist/fog overlay: spring-damping point animation |
| `effects/weather/drip.h/.cpp` | Drip: expanding fading ripple circles on water/puddle surfaces |

---

## effects/combat/

| Файл | Описание |
|------|----------|
| `effects/combat/spark.h/.cpp` | Electric spark: branching lightning bolt between two points, glitter spawn |
| `effects/combat/glitter.h/.cpp` | Glitter: coloured line-segment spark clusters, z-slice iterable |
| `effects/combat/pow.h/.cpp` | Sprite-based explosion effect: POW_Pow with linked POW_Sprite particles |
| `effects/combat/pyro.h/.cpp` | Pyrotechnics: particle-based fire/explosions/smoke, 18 types, 64 slots |
| `effects/combat/glow.h/.cpp` | Boss glow overlay: rotating four-quad sprite for Guardian of Baalrog |

---

## effects/environment/

| Файл | Описание |
|------|----------|
| `effects/environment/fire.h/.cpp` | Pool-based fire effect: up to 8 fires, 256 shared flame nodes, size-decaying |
| `effects/environment/ribbon.h/.cpp` | Ribbon trail: circular-buffer of 3D points, textured triangle strips |
| `effects/environment/tracks.h/.cpp` | Tyre/footprint track decals: ring-buffer, surface-dependent |

---

## camera/

| Файл | Описание |
|------|----------|
| `camera/cam.h` | High-level camera API: mode/focus/zoom/shake |
| `camera/fc.h/.cpp` | Game-follow camera: FC_Cam, follows Darci/cops/vehicles, split-screen |

---

## ai/

| Файл | Описание |
|------|----------|
| `ai/mav_action.h` | MAV_Action — single navigation step result struct |
| `ai/mav.h/.cpp` | MAV — A* pathfinding on 128x128 map, build/update nav grid |
| `ai/pcom.h/.cpp` | PCOM — NPC AI system: timing, navigation, gang management, alerts |

---

## combat/

| Файл | Описание |
|------|----------|
| `combat/combat.h/.cpp` | Combat — melee/ranged hit resolution, damage, fight-tree combos |

---

## shooting/

| Файл | Описание |
|------|----------|
| `shooting/guns.h/.cpp` | Gun targeting system: auto-aim scoring and target selection |
| `shooting/projectile.h/.cpp` | Projectile — minimal struct, pool alloc/free |

---

## missions/

| Файл | Описание |
|------|----------|
| `missions/mission.h` | Mission data model: EventPoint, WPT_*/TT_*/OT_* constants |
| `missions/eway.h/.cpp` | EWAY scripting engine: waypoint triggers, player spawn, speech |
| `missions/playcuts.h/.cpp` | Cutscene playback: reads .cutscene files, drives characters/camera |
| `missions/memory.h/.cpp` | Level memory management and save/load: pool alloc, quick save/load (F5/F9) |
| `missions/save.h/.cpp` | Memory allocation table: MemTable, save_table[] pool descriptors |

---

## assets/formats/

| Файл | Описание |
|------|----------|
| `assets/formats/file_clump.h/.cpp` | FileClump archive: .gob/.ilf/.txc container format |
| `assets/formats/tga.h/.cpp` | TGA file loader/saver: load from file or FileClump, remapping |
| `assets/formats/anim.h/.cpp` | Character animation: loads .all files, builds global_anim_array |
| `assets/formats/anim_loader.h/.cpp` | Animation file loading: .VUE keyframes, .moj meshes, frame ordering |
| `assets/formats/anim_tmap.h/.cpp` | Animated texture map: cycles UV frames per texture, loads tmap.ani |
| `assets/formats/level_loader.h/.cpp` | Level resource loader: LoadGameThing, .tma style-table loading |
| `assets/formats/elev.h/.cpp` | Level loader: parses .ucm mission file, loads map/lighting/entities |
| `assets/formats/mapthing.h` | PSX map thing structs: MapThingPSX, MAP_THING_TYPE_* |

---

## assets/ (root)

| Файл | Описание |
|------|----------|
| `assets/texture.h/.cpp` | Direct3D texture page management: choose_set, special page indices |
| `assets/sound_id.h` | Sound ID enum (Waves): all filenames and grouped range constants |
| `assets/xlat_str.h/.cpp` | Localization strings: X_* string IDs, XCTL_* control placeholders |

---

## ui/frontend/

| Файл | Описание |
|------|----------|
| `ui/frontend/frontend.h/.cpp` | Main frontend/menu system: mission select, briefing, config screens |
| `ui/frontend/attract.h/.cpp` | Attract/frontend loop: game_attract_mode, loading screen |
| `ui/frontend/startscr.h/.cpp` | Start screen action codes (STARTS_*) and StartMenu structs |

---

## ui/hud/

| Файл | Описание |
|------|----------|
| `ui/hud/panel.h/.cpp` | HUD panel: health bars, gun sights, timers, inventory overlay |
| `ui/hud/overlay.h/.cpp` | In-game overlay: enemy tracking, gun sight, damage popups, beacons |
| `ui/hud/planmap.h/.cpp` | Plan-view (top-down) city map renderer and beacon blips |
| `ui/hud/eng_map.h/.cpp` | Full-screen minimap overlay: terrain tiles, people/vehicle dots |

---

## ui/menus/

| Файл | Описание |
|------|----------|
| `ui/menus/widget.h/.cpp` | GUI widget/form system: Widget/Form classes, callbacks |
| `ui/menus/gamemenu.h/.cpp` | In-game pause/won/lost menu state machine |
| `ui/menus/pause.h/.cpp` | Pause menu: overlay, Resume/Restart/Exit |

---

## outro/core/

| Файл | Описание |
|------|----------|
| `outro/core/outro_os.h/.cpp` | Outro platform abstraction: DirectDraw/D3D, input, textures, sound |
| `outro/core/outro_matrix.h/.cpp` | Outro 3x3 rotation matrix utilities |
| `outro/core/outro_font.h/.cpp` | Outro font renderer: glyph texture, formatted/italic/shimmer text |
| `outro/core/outro_cam.h/.cpp` | Outro camera: position/orientation, keyboard-driven update |
| `outro/core/outro_tga.h/.cpp` | TGA loader for outro (self-contained variant) |
| `outro/core/outro_imp.h/.cpp` | Outro mesh importer: IMP_Mesh/IMP_Mat structs |
| `outro/core/outro_mf.h/.cpp` | Outro mesh renderer: loads textures, transforms, draws IMP_Mesh |
| `outro/core/outro_key.h` | Keyboard scancodes for outro sequence |
| `outro/core/outro_always.h` | Base types and utility macros for outro mini-engine |

---

## outro/ (content)

| Файл | Описание |
|------|----------|
| `outro/outro_main.h/.cpp` | Outro/credits sequence entry point and main loop |
| `outro/outro_back.h/.cpp` | Outro sliding background panels: face images, crossfade |
| `outro/outro_credits.h/.cpp` | Outro scrolling credits text |
| `outro/outro_wire.h/.cpp` | Outro spinning wireframe/textured mesh display |
