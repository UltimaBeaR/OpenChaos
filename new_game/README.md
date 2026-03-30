# OpenChaos — new game

Modernized version of Urban Chaos (1999, MuckyFoot Productions).

Based on the original source code by Mike Diskett: https://github.com/dizzy2003/MuckyFoot-UrbanChaos

For prerequisites and first-time setup see [SETUP.md](SETUP.md). For development notes see [DEVELOPMENT.md](DEVELOPMENT.md).

---

## Quick start

1. Copy your Urban Chaos game files (everything except `.exe`/`.dll`) into `original_game_resources/` at the repo root
2. Run:
```bash
make configure-opengl   # configure (once, or after CMakeLists.txt changes)
make copy-resources     # copy game assets into build folders (once)
make r                  # build + run Release
```

Prerequisites and detailed instructions: [SETUP.md](SETUP.md).

---

## Source structure (`src/`)

```
src/
├── main.cpp                — Entry point (main → HOST_run)
│
├── engine/                 — Game engine (UC-agnostic, reusable)
│   ├── core/               — Types, math, memory, heap, timer
│   ├── platform/           — uc_common.h (platform umbrella), host, SDL3 bridge
│   ├── graphics/
│   │   ├── graphics_engine/ — Swappable graphics backend abstraction
│   │   │   ├── game_graphics_engine.h  — ge_* contract (API-agnostic)
│   │   │   ├── outro_graphics_engine.h — oge_* contract (outro)
│   │   │   ├── backend_opengl/         — OpenGL 4.1 Core Profile (cross-platform)
│   │   │   ├── backend_directx6/       — D3D6/DDraw (Windows-only legacy)
│   │   │   └── backend_stub/           — No-op stub (compile-only verification)
│   │   ├── pipeline/       — Render pipeline: aeng, bucket, poly, polypage, draw2d, qeng
│   │   ├── geometry/       — Mesh renderers: mesh, facet, superfacet, farfacet, fastprim, figure, cone, oval, shape, sprite, sky, aa
│   │   ├── lighting/       — Gamut, night (per-vertex), shadow, smap, crinkle
│   │   ├── postprocess/    — Bloom (lens flare), wibble (screen distortion)
│   │   └── text/           — Font, font2d, menufont, truetype, text renderer
│   ├── console/            — Debug console + on-screen messages
│   ├── io/                 — File I/O: file, env (INI config)
│   ├── audio/              — MFX (3D positional audio), music, sound, soundenv
│   ├── input/              — Keyboard, mouse, gamepad (SDL3)
│   ├── animation/          — Morph (vertex keyframes), anim_types
│   ├── effects/            — Particle system (psystem), flame engine (flamengine)
│   ├── physics/            — Collision detection (collide), soft-body (hm, sm)
│   └── compression/        — Delta compression, S3 block compression
│
├── game/                   — Game loop and startup
│   ├── startup.cpp         — MF_main: config, init, launch game()
│   ├── game.*              — Main game loop: attract → mission → gameplay
│   ├── game_types.h        — Game struct, GS_/GF_ constants, accessor macros
│   ├── game_tick.*          — Per-frame dispatcher: inventory, danger level, debug console
│   ├── input_actions.*     — Hardware input → game actions (INPUT_PUNCH, ACTION_*, etc.)
│   └── network_state_globals.* — Stub multiplayer state (always single-player)
│
├── things/                 — Thing system (all game objects)
│   ├── core/               — Thing alloc/free, state dispatch, drawtype, hierarchy, player, switch
│   ├── characters/         — Person, Darci, Cop, Roper, Thug, Snipe, anim_ids
│   ├── vehicles/           — Vehicle, Chopper, Furn
│   ├── items/              — Barrel, Balloon, Grenade, Guns, Hook, Projectile, Special
│   └── animals/            — Animal, Bat
│
├── map/                    — Heightfield map and spatial data
│   ├── pap.*               — Two-tier heightfield (128x128 hi-res / 32x32 lo-res)
│   ├── map.*               — MapElement grid (lighting per cell)
│   ├── supermap.*          — Spatial index: DBuildings, DFacets, walkables
│   ├── road.*              — Road node/edge graph
│   ├── ob.*                — Static map objects (lamps, benches, hydrants)
│   ├── qmap.*              — Debug/editor block renderer
│   ├── sewers.*            — Sewer/cavern underground layer
│   ├── map_constants.h     — Map cell size constants (ELE_SIZE, EDIT_MAP_WIDTH)
│   └── level_pools.h      — Extern declarations for geometry pools
│
├── buildings/              — Building geometry and construction
│   ├── building.*          — FBuilding/FStorey/FWall data model
│   ├── build.*             — Building renderer
│   ├── build2.*            — City spatial index construction
│   ├── prim.*              — Static world mesh: vertex/face pools, collision, normals
│   ├── stair.*             — Staircase placement algorithm
│   ├── outline.*           — Grid polygon overlap test
│   ├── id.h                — Interior design: floor-plan generation
│   └── ware.*              — Warehouse: large indoor areas with navigation
│
├── navigation/             — Movement and pathfinding surfaces
│   ├── walkable.*          — Walkable surface queries, height interpolation
│   ├── wmove.*             — Moving walkable faces (vehicles/platforms)
│   ├── inside2.*           — Building interior navigation (rooms, doors, stairs)
│   └── wand.*              — Wander zones for NPC roaming, spawn points
│
├── world_objects/          — Interactive and ambient world objects
│   ├── door.*              — Doors: open/close, slide animation
│   ├── plat.*              — Moving platforms: waypoint patrol
│   ├── tripwire.*          — Wire traps: collision, activation
│   ├── puddle.*            — Rain puddles along building/road edges
│   └── dirt.*              — Ambient debris: leaves, cans, pigeons (pick up & throw)
│
├── effects/                — UC visual effects
│   ├── weather/            — Fog sprites, mist quad-patches, drip ripples
│   ├── combat/             — Sparks, glitter, pow explosions, pyro fire/explosions, boss glow
│   └── environment/        — Fire pools, ribbon trails, tyre/foot tracks
│
├── camera/                 — Game camera
│   ├── cam.*               — High-level camera API: mode, focus, zoom, shake
│   └── fc.*                — Follow camera: tracks Darci/cops/vehicles
│
├── ai/                     — NPC behavior
│   ├── mav.*               — A* pathfinding on 128x128 nav grid
│   └── pcom.*              — NPC AI: timing, navigation, gang management, alerts
│
├── combat/                 — Melee combat system (all characters, player included)
│   └── combat.*            — Hit resolution, fight-tree combos, blocking
│
├── shooting/               — Ranged combat system
│   ├── guns.*              — Auto-aim scoring, target selection
│   └── projectile.*        — Projectile pool, alloc/free
│
├── missions/               — Mission system
│   ├── eway.*              — EWAY scripting engine (42 conditions, 52 actions)
│   ├── mission.h           — EventPoint struct, waypoint/trigger type constants
│   ├── playcuts.*          — Cutscene playback: .cutscene keyframe tracks
│   ├── memory.*            — Level memory management, quick save/load
│   └── save.*              — Memory allocation table (MemTable, save_table[])
│
├── assets/                 — UC resource data
│   ├── formats/            — Format parsers
│   │   ├── file_clump.*    — FileClump archives (.gob/.ilf/.txc)
│   │   ├── tga.*           — TGA image loader/saver
│   │   ├── anim.*          — Animation system (.all files)
│   │   ├── anim_loader.*   — Animation file loading (.VUE, .moj)
│   │   ├── anim_tmap.*     — Animated UV texture maps (tmap.ani)
│   │   ├── level_loader.*  — Level resource loading (.tma style-tables)
│   │   ├── elev.*          — Level loader: parse .ucm mission files
│   │   └── mapthing.h      — Map thing structs for level file parsing
│   ├── texture.*           — Texture page management (via ge_* abstraction)
│   ├── sound_id.*          — Sound filename enum (Waves)
│   └── xlat_str.*          — Localization string system
│
├── ui/                     — User interface
│   ├── frontend/           — Main menu: mission select, briefing, attract mode, start screen data
│   ├── hud/                — Panel (health, inventory), overlay (tracking, popups), planmap, eng_map
│   └── menus/              — Widget/Form GUI framework, gamemenu (pause/won/lost), pause overlay
│
└── outro/                  — End credits sequence (self-contained mini-engine)
    ├── core/               — Credits engine: renderer, camera, font, matrix, TGA, mesh import
    └── (root)              — Credits content: outro_main, backgrounds, scrolling text, spinning meshes
```

