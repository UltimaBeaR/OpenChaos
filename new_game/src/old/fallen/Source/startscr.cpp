

// most of this file is obsolete
// just a few torn scraps remain...
// claude-ai: В игровой (non-EDITOR) сборке весь файл кроме первой строки внутри #ifdef EDITOR.
// claude-ai: Реальный игровой код: только глобальная переменная CBYTE STARTSCR_mission[_MAX_PATH].
// claude-ai: STARTSCR_mission = мост frontend → загрузка уровня:
// claude-ai:   - Устанавливается в frontend.cpp::FRONTEND_loop() при FE_START (mission selected).
// claude-ai:   - Читается в elev.cpp::ELEV_load() через extern; вызывает ELEV_load_name().
// claude-ai:   - После загрузки *STARTSCR_mission=0 (сбрасывается).
// claude-ai: EDITOR блок (#ifdef EDITOR): старый mission selection UI (pre-release, дек 1998).
// claude-ai:   Содержит LoadMissionFilename(), LoadNextMissionFilename() — для старого briefing flow.

#include "MFStdLib.h"

CBYTE STARTSCR_mission[_MAX_PATH] = { 0 };
