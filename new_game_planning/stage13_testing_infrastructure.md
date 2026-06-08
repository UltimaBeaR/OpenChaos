# Testing infrastructure (part of Stage 13)

Former Stage 6. Done BEFORE the architecture refactoring.

**Goal:** create a safety net for the architecture refactoring. The refactoring will touch all of the code — we need a way to make sure that nothing broke after porting to the new architecture.

**Principle:** we take a "snapshot" of the game's behavior BEFORE the refactoring, and after the refactoring we compare — discrepancies = bugs.

Details → [`testing.md`](testing.md)

**Criterion:** the testing infrastructure is implemented and covers the main subsystems before the architecture refactoring begins.
