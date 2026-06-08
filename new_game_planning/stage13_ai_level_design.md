# AI mission-creation system (part of Stage 13)

A major feature. The idea: **the AI (Claude Code) creates custom levels and missions**, while the user acts as a creative director. The AI does ~95% of the work — it designs, builds, and adds detail. The user provides high-level direction and makes decisions.

This is NOT manual mission creation in an editor — it is a fundamentally different workflow, built around the AI.

---

## Preparation

- **Restore the game editor.** The editor code was cut out of new_game, so it will have to be pulled from the original (`original_game/`). Build it separately, make a working version. Not necessarily a 1:1 copy of the original editor — possibly a different, more convenient one.

## MCP server for the editor

- Wrap all of the editor's functionality in an MCP server. Claude Code interacts with the editor through MCP: it places objects, sets parameters, controls the map geometry, and so on.
- A special Claude Code working mode: connecting to the editor's MCP, with skills and documentation for level design.
- The AI works with the editor programmatically — not a single action requires a manual click in the UI.

## Knowledge base for level design

Before creating missions — build up a complete knowledge base so that the AI understands what to build and how:
- Analysis of all existing levels: structure, objects, effects, scripts
- Asset catalog: what the buildings, objects, characters, and vehicles look like (visual understanding)
- Visual reference: small preview images of each object/asset (low resolution — for fast and cheap perception by the AI)
- Object types, available effects, placement options
- Game design patterns: how missions are built, which mechanics combine, how difficulty is structured
- Documentation and skills for Claude Code covering all of the above

## Mission-creation workflow

An iterative process. The AI does all the work, the user provides direction:
1. **Story and concept** — the AI proposes story/scenario options for the mission, the user picks a direction
2. **Level layout** — the AI designs the layout, proposes options ("put a building here — will it look nice?"), the user approves/corrects
3. **Detailing** — the AI works through the details: enemy placement, event scripts, effects, lighting
4. **Showing the result** — the AI shows the intermediate result to the user (screenshots, description)
5. **Iteration** — the user gives feedback in broad strokes, the AI refines

Principle: all questionable decisions are discussed with the user. The user picks the high-level direction, the AI handles the details and implementation.

## References

- **PieroZ** — a working version of the original editor (recorded videos). Worth a look to understand the workflow.
- **Darci's Shield** — [`github.com/SirSwish/Darcis-Shield`](https://github.com/SirSwish/Darcis-Shield) — a separate level editor for Urban Chaos.
