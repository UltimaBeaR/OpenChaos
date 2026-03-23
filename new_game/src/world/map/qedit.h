#ifndef WORLD_MAP_QEDIT_H
#define WORLD_MAP_QEDIT_H

// QMAP wireframe editor: interactive viewer for the quad-tree collision map.
// Only used during development; not part of the shipping game loop.

// Run the QMAP editor main loop (opens display, processes input, renders until quit).
// uc_orig: QEDIT_loop (fallen/Headers/qedit.h)
void QEDIT_loop(void);

#endif // WORLD_MAP_QEDIT_H
