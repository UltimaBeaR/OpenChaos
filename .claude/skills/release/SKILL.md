---
name: release
description: >
  GitHub release management for OpenChaos — guides user through building,
  packaging, and publishing releases, and generates release notes with the
  established format. Use this skill whenever the user asks to create/prepare
  a release, write release notes, generate a release description, or mentions
  publishing to GitHub releases. Trigger on: "релиз", "release", "release notes",
  "описание релиза", "напиши описание", "подготовь релиз", "сделай релиз",
  "write release notes", "generate release description".
---

# Release Management

This skill covers two workflows: guiding the user through the release process, and generating release notes.

## Part 1: Release process guide

When the user asks to create or prepare a release, walk them through these steps. You do NOT execute build/git commands yourself (per project rules — user builds manually). Just tell the user what to do.

### Steps to communicate to the user:

1. **Commit and tag.** Make sure all changes are committed and the release tag `v<X.Y.Z>` is created.

2. **Build on each platform.** On every supported platform (currently Windows x64, macOS ARM64, Linux x64):
   ```
   make configure
   make build-release
   make release-package VERSION=<X.Y.Z>
   ```
   This creates a zip in `release/dist/`.

3. **Test each package.** Extract the zip into an Urban Chaos game folder (where the `data` folder is) and verify the game launches and is playable.

4. **Collect all platform zips** into one folder for easy access.

5. **Create the GitHub release.**
   - Go to GitHub -> Releases -> "Draft a new release"
   - Select the existing tag `v<X.Y.Z>`
   - **Release title** field: `v<X.Y.Z> — Short description` (e.g. `v0.2.0 — Rendering fixes`)
   - **Release notes** body: paste from the generated release notes (see Part 2)
   - Attach all platform zip files as release assets
   - If version is below 1.0.0 — check **"Set as a pre-release"**
   - Publish

6. **Tell the user:** "When you're ready to write the release description, ask me — I know the format and will generate it."

---

## Part 2: Writing release notes

When the user asks to write or generate release notes:

### Step 1: Gather changes

Run `git log vPREVIOUS..vCURRENT --oneline` to get all commits between the previous and current release tags. If unsure which is the previous tag, run `git tag --sort=-v:refname` to list tags.

### Step 2: Analyze and categorize

- If a commit message is unclear or too vague, look at the actual diff (`git show <hash>`) to understand what was changed
- Also check documentation diffs in commits — docs often describe what was done in more detail than the commit message itself
- Skip docs-only commits ("docs update", "small docs update", etc.)
- Group remaining commits into categories based on content (see template below)
- Each change gets a clickable commit hash link

### Step 3: Generate notes using this template

This template is approximate — section contents may change depending on the current project state (e.g. supported platforms, known issues, alpha vs stable). For concrete examples see https://github.com/UltimaBeaR/OpenChaos/releases

```markdown
## OpenChaos vX.Y.Z — Short description

Fan modernization of Urban Chaos (1999). Requires original game files (Steam/GOG).

### What's new

**Category name:**
* Change description ([`abcd1234`](https://github.com/UltimaBeaR/OpenChaos/commit/abcd1234))

### Current state

This is an early alpha. The game launches and is partially playable, but expect:
* Crashes (especially on Windows)
* Visual glitches
* Fixed window resolution (no resize yet)
* No launcher or settings UI

### How to run

1. Download the zip for your platform
2. Extract into your Urban Chaos game folder (where the `data` folder is)
3. Run `OpenChaos.exe` (Windows), `OpenChaos.command` (macOS), or `OpenChaos.sh` (Linux)

### Requirements

* Original Urban Chaos game files (Steam or GOG version)
* Windows 10+ (x64), macOS 12+ (Apple Silicon), or Linux x86_64 (Steam Deck compatible)
```

### Formatting rules

These rules are derived from v0.1.0 and v0.2.0 release notes and must be followed exactly:

- **Title:** `## OpenChaos vX.Y.Z — Short description` — the same text without `## ` goes into GitHub's "Release title" field
- **Subtitle:** `Fan modernization of Urban Chaos (1999). Requires original game files (Steam/GOG).` — always present, unchanged
- **Commit hashes:** 8-char short hashes, always clickable links: `[`hash`](https://github.com/UltimaBeaR/OpenChaos/commit/hash)`
- **Multiple commits for one fix:** list all: `([`hash1`](...), [`hash2`](...))`
- **Categories** under "What's new" are **bold** with colon: `**Gameplay:**`, `**Rendering bug fixes:**`, `**Internal:**` etc. Choose categories based on actual content. **Internal** always goes last.
- **Bullet points:** use `*` not `-`
- **Russian documents:** if linking to documents written in Russian, add `(document is in Russian)` to the description
- **Boilerplate sections:** "Current state", "How to run", "Requirements" are copied as-is. Update them only when the project state actually changes (e.g. crashes fixed, new platforms added, resize support added).
- **Output file:** write the result into `release_notes_vX.Y.Z.md` in the project root for the user to copy, then delete afterwards

---

## Part 3: Handling user corrections

If the user asks to change something in the release notes during the current session:

1. Make the requested change
2. **Then ask:** "This change affects the release notes format. Should I update the release skill to reflect this for future releases?"
3. If yes — update this SKILL.md accordingly
