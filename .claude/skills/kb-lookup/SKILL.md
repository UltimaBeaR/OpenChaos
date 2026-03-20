---
name: kb-lookup
description: >
  Navigator for the Urban Chaos original game knowledge base (48 files, ~450KB).
  Use this skill whenever you need information about the original game: how a subsystem works,
  what a function does, game mechanics, file formats, architecture, preprocessor flags,
  cut features. Trigger on: questions about the original Urban Chaos, "как работает X в оригинале",
  "что делает эта функция", "оригинальная механика", "как устроен", references to specific
  subsystems (physics, AI, rendering, navigation, combat, vehicles, etc.), or when you need
  to verify a claim about the original game before stating it as fact.
  IMPORTANT: Never guess about the original game — always use this skill to verify.
---

# Urban Chaos Knowledge Base Navigator

The knowledge base documents the original Urban Chaos (1999, MuckyFoot Productions) codebase.
It's located in `original_game_knowledge_base/` (detailed, 48 files) and
`original_game_knowledge_base_dense/` (compact summary, 3 files).

## Critical Rule

**Never make claims about the original game without verifying them in the KB.**
"Probably works like this", "typical for games of that era", "usually in such cases" — unacceptable.
Either the fact is in the KB, or say "I don't know, need to check."

## How to Navigate

### Quick answer about multiple subsystems → DENSE_SUMMARY

Read all parts (they may be split across files):
- `original_game_knowledge_base_dense/DENSE_SUMMARY.md`
- `original_game_knowledge_base_dense/DENSE_SUMMARY_2.md`
- `original_game_knowledge_base_dense/DENSE_SUMMARY_3.md`

~576 lines total. Covers all subsystems compactly with numbers, formulas, dependencies.
**Read ALL parts, not just the first one.**

### Deep question about one specific subsystem → Read the subsystem file directly

Use the index below to find the right file.

### Don't know which file → Check the index below, or read `original_game_knowledge_base/KB_INDEX.md`

---

## KB Index — What to Read for What

### General / Architecture
| Topic | File |
|-------|------|
| Architecture, folder structure, entry point, file map | `subsystems/../overview.md` |
| Analysis scope, what's included/excluded | `subsystems/../analysis_scope.md` |
| Cut/disabled features | `subsystems/../cut_features.md` |
| All #ifdef flags, classification | `subsystems/../preprocessor_flags.md` |

### Subsystems
| Topic | File |
|-------|------|
| Physics and collisions | `subsystems/physics.md` |
| Physics — water, gravity, HyperMatter, WMOVE | `subsystems/physics_details.md` |
| Game objects (Thing system) | `subsystems/game_objects.md` |
| Game objects — Barrel, Platform, Chopper | `subsystems/game_objects_details.md` |
| AI and NPC behavior (PCOM core) | `subsystems/ai.md` |
| AI — Person data structures | `subsystems/ai_structures.md` |
| AI — detailed behaviors (MIB/Bane/Snipe/Zone) | `subsystems/ai_behaviors.md` |
| NPC navigation (MAV/NAV) | `subsystems/navigation.md` |
| Characters and animations | `subsystems/characters.md` |
| Characters — types, CMatrix33, Darci physics | `subsystems/characters_details.md` |
| Controls and input (PC) | `subsystems/controls.md` |
| Controls and input (PSX) | `subsystems/psx_controls.md` |
| Vehicles | `subsystems/vehicles.md` |
| Combat system | `subsystems/combat.md` |
| Weapons and items | `subsystems/weapons_items.md` |
| Weapons — grenades, autoaim, DIRT banks | `subsystems/weapons_items_details.md` |
| Interaction (grab/ladder/cable/zipwire) | `subsystems/interaction_system.md` |
| Game world and map | `subsystems/world_map.md` |
| Buildings and interiors | `subsystems/buildings_interiors.md` |
| Buildings — id.cpp rendering, WARE warehouses | `subsystems/buildings_interiors_details.md` |
| Level loading | `subsystems/level_loading.md` |
| Mission system (EWAY runtime) | `subsystems/missions.md` |
| Missions — WPT translation, CRIME_RATE, binary .ucm | `subsystems/missions_implementation.md` |
| Player progress and saves | `subsystems/player_progress.md` |
| Game states (GS_*, pause, death) | `subsystems/game_states.md` |
| Player states (STATE_*, SUB_STATE_*) | `subsystems/player_states.md` |
| Camera | `subsystems/camera.md` |
| Rendering and lighting | `subsystems/rendering.md` |
| Rendering: Tom's Engine, meshes, morphing | `subsystems/rendering_mesh.md` |
| Rendering: NIGHT lighting, Crinkle | `subsystems/rendering_lighting.md` |
| Audio | `subsystems/audio.md` |
| UI and frontend (HUD, fonts, dialogs) | `subsystems/ui.md` |
| Frontend (menus, mission launch pipeline) | `subsystems/frontend.md` |
| Visual effects | `subsystems/effects.md` |
| Effects: explosions (POW), pyro (PYRO), debris (DIRT) | `subsystems/effects_pyro_pow_dirt.md` |
| Minor systems (water, tripwires, SM, balls, tracks, sparks) | `subsystems/minor_systems.md` |
| Math and utilities | `subsystems/math_utils.md` |
| WayWind (editor, out of scope) | `subsystems/waywind.md` |

### Resource Formats
| Format | File |
|--------|------|
| Overview + folder structure | `resource_formats/README.md` |
| Maps (.pam/.pi/.pp) | `resource_formats/map_format.md` |
| 3D models (.IMP/.MOJ) | `resource_formats/model_format.md` |
| Animations (in .IMP/.MOJ) | `resource_formats/animation_format.md` |
| Textures (.TGA/.TXC) | `resource_formats/texture_format.md` |
| Lighting (.lgt) | `resource_formats/lighting_format.md` |
| Audio (.WAV) | `resource_formats/audio_format.md` |
| Mission scripts (.sty) | `resource_formats/mission_script_format.md` |

All paths above are relative to `original_game_knowledge_base/`.

---

## Context Budget

The KB is ~450KB total — it does NOT fit in context.
- Reading 5+ KB files fills the context and causes compaction (data loss)
- For multi-subsystem questions: use DENSE_SUMMARY (576 lines) instead of individual files
- For single-subsystem deep dives: read only that one file
- Never load the whole KB "just in case"

## Important: Pre-Release Codebase

`original_game/` is a **pre-release version**, not the final game.
- Some features are commented out but work in the final release
- Some bugs in this version are fixed in the final
- "Commented out code" ≠ "cut feature"
- Reference: both release versions — Steam PC AND PS1
- When in doubt — ask the user (they played the PS1 final, can check PC via longplays)
