# Additional game content (part of Stage 13)

Creating new maps, missions, and other content on top of the original game. Requires a working game editor.

---

## Preparation: the editor

- Restore the game editor — the code was cut out of new_game, so it will have to be pulled from the original (`original_game/`)
- Not necessarily a 1:1 copy of the original editor — possibly a different, more convenient one
- References: PieroZ (working version, recorded videos), [Darci's Shield](https://github.com/SirSwish/Darcis-Shield)

## Two approaches

Not mutually exclusive — both can be used together. The AI creates the foundation → a human refines it in the editor, or the other way around: a human makes the layout → the AI places the details.

### Manual creation
- Classic workflow: a human works in the editor by hand
- Separate system, its own UI, its own tools

### AI mission-creation system
- The AI (Claude Code) creates content via an MCP server, the user provides direction
- A fundamentally different workflow, built around the AI
- Details → [stage13_ai_level_design.md](stage13_ai_level_design.md)
