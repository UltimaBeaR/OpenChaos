// Player.h
// Guy Simmons, 2nd January 1998.

#ifndef FALLEN_HEADERS_PLAYER_H
#define FALLEN_HEADERS_PLAYER_H

//---------------------------------------------------------------

// claude-ai: MAX_PLAYERS=2 — поддержка split-screen. Мультиплеер по сети в новой версии НЕ переносится.
#define MAX_PLAYERS 2

// claude-ai: Типы игрока. DARCI(1) и ROPER(2) — играбельные персонажи. COP/THUG — неиграбельные.
#define PLAYER_NONE 0
#define PLAYER_DARCI 1
#define PLAYER_ROPER 2
#define PLAYER_COP 3
#define PLAYER_THUG 4

//---------------------------------------------------------------

// claude-ai: Структура Player — данные об игроке (ввод, здоровье, штрафы, инвентарь, ссылки на Thing)
typedef struct
{
    COMMON(PlayerType)

    // claude-ai: Input — bitmask текущего состояния кнопок (биты 0-17 = кнопки, биты 17-31 = аналоговые стики)
    // claude-ai: InputDone — маска уже обработанных нажатий (чтобы не обрабатывать дважды)
    ULONG Input;
    ULONG InputDone;
    UWORD PlayerID;
    UBYTE Stamina;
    UBYTE Constitution;

    // claude-ai: Pressed = новые нажатия (Input & ~LastInput), Released = отпущенные (LastInput & ~Input)
    ULONG LastInput; // The input last gameturn
    ULONG ThisInput; // The input this gameturn
    ULONG Pressed; // The keys pressed  this gameturn
    ULONG Released; // The keys released this gameturn
    ULONG DoneSomething; // Flag so you know when you've pressed left or done a left-punch.
    // claude-ai: LastReleased — GetTickCount() момента отпускания кнопки; DoubleClick — счётчик двойных нажатий (окно 200мс)
    SLONG LastReleased[16]; // The GetTickCount() of when each key was last released
    UBYTE DoubleClick[16]; // The double-click count for each key.

    UBYTE Strength;
    // claude-ai: RedMarks — штрафные очки (0-10). При достижении 10 — немедленный game over!
    UBYTE RedMarks;
    UBYTE TrafficViolations;
    UBYTE Danger; // How far from Danger is Darci? 0 => No danger, 1 = Max danger, 3 = min danger
    // temporarily marked out from the psx until it gets ported across
    UBYTE PopupFade; // Bringing the pop-up inventory in and out. Part of player cos of splitscreen mode
    SBYTE ItemFocus; // In the inventory, the item you're about to select when you let go
    UBYTE ItemCount; // Number of valid inventory items currently held
    UBYTE Skill;

    // claude-ai: CameraThing — Thing, за которым следует камера; PlayerPerson — Thing самого персонажа игрока
    struct Thing *CameraThing,
        *PlayerPerson;

} Player;

typedef Player* PlayerPtr;

//---------------------------------------------------------------

extern GenusFunctions player_functions[];

void init_players(void);
Thing* alloc_player(UBYTE type);
void free_player(Thing* player_thing);
Thing* create_player(UBYTE type, SLONG x, SLONG y, SLONG z, SLONG id);

//---------------------------------------------------------------

//
// Call when the player gains or looses a red mark.
//

void PLAYER_redmark(SLONG playerid, SLONG dredmarks);

#endif // FALLEN_HEADERS_PLAYER_H
