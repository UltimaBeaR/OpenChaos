# DualSense — reference

Knowledge about the DualSense controller (protocol, hardware facts) and about our own library
[`libDualsense/`](../../../libDualsense/) (its usage docs: `libDualsense/README.md`, `API.md`, `EXAMPLES.md`).

- [`dualsense_protocol_reference.md`](dualsense_protocol_reference.md) — USB/BT transport, input/output report
  layout, trigger effect modes.
- [`dualsense_adaptive_trigger_facts.md`](dualsense_adaptive_trigger_facts.md) — measured hardware facts about the
  adaptive triggers (click rate limit, feedback offsets, state nybble, cross-platform validation).
- [`dualsense_hardware_quirks.md`](dualsense_hardware_quirks.md) — firmware/hardware behaviors that look like bugs but
  can't be fixed from the host.
- [`lib_scope.md`](lib_scope.md) — what belongs in libDualsense vs. in the consumer (game, debug tester).

How the game uses haptics and triggers — `new_game/src/engine/input/weapon_feel.cpp`, the bridge —
`new_game/src/engine/platform/ds_bridge*.cpp`.

History (plan of our own lib, review against daidr/dualsense-tester, PR notes for the former vendored lib, how to
download the reference libraries) — `archive/new_game_devlog/dualsense_libs_reference/`.
