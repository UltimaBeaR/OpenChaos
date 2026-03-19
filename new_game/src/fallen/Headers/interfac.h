#ifndef FALLEN_HEADERS_INTERFAC_H
#define FALLEN_HEADERS_INTERFAC_H

//
// Defines
//

// claude-ai: Номера кнопок (биты в bitmask Player::Input). Всего 18 кнопок (0-17).
// claude-ai: Старшие биты (17-31) зарезервированы под аналоговые стики — макросы GET_JOYX/GET_JOYY в interfac.cpp.
#define INPUT_FORWARDS 0
#define INPUT_BACKWARDS 1
#define INPUT_LEFT 2
#define INPUT_RIGHT 3
#define INPUT_START 4
#define INPUT_CANCEL 5
#define INPUT_PUNCH 6
#define INPUT_KICK 7
#define INPUT_ACTION 8
#define INPUT_JUMP 9
#define INPUT_CAMERA 10
#define INPUT_CAM_LEFT 11
#define INPUT_CAM_RIGHT 12
#define INPUT_CAM_BEHIND 13
#define INPUT_MOVE 14
#define INPUT_SELECT 15
#define INPUT_STEP_LEFT 16
#define INPUT_STEP_RIGHT 17

// claude-ai: Bitmask версии констант. INPUT_MASK_ALL_BUTTONS = 0x3ffff охватывает только биты 0-17 (цифровые кнопки).
#define INPUT_MASK_FORWARDS (1 << INPUT_FORWARDS)
#define INPUT_MASK_BACKWARDS (1 << INPUT_BACKWARDS)
#define INPUT_MASK_LEFT (1 << INPUT_LEFT)
#define INPUT_MASK_START (1 << INPUT_START)
// Used for menus
#define INPUT_MASK_CANCEL (1 << INPUT_CANCEL)
#define INPUT_MASK_RIGHT (1 << INPUT_RIGHT)
#define INPUT_MASK_PUNCH (1 << INPUT_PUNCH)
// Use for menus
#define INPUT_MASK_DOMENU (INPUT_MASK_PUNCH)
#define INPUT_MASK_KICK (1 << INPUT_KICK)
#define INPUT_MASK_ACTION (1 << INPUT_ACTION)
#define INPUT_MASK_JUMP (1 << INPUT_JUMP)
// #define	INPUT_MASK_MODE_CHANGE	(1 << INPUT_MODE_CHANGE)
#define INPUT_MASK_CAMERA (1 << INPUT_CAMERA)
#define INPUT_MASK_CAM_LEFT (1 << INPUT_CAM_LEFT)
#define INPUT_MASK_CAM_RIGHT (1 << INPUT_CAM_RIGHT)
#define INPUT_MASK_CAM_BEHIND (1 << INPUT_CAM_BEHIND)
#define INPUT_MASK_MOVE (1 << INPUT_MOVE)
#define INPUT_MASK_SELECT (1 << INPUT_SELECT)
#define INPUT_MASK_STEP_LEFT (1 << INPUT_STEP_LEFT)
#define INPUT_MASK_STEP_RIGHT (1 << INPUT_STEP_RIGHT)
// This is hardwired - the analog values go in the top 14 bits.
#define INPUT_MASK_ALL_BUTTONS (0x3ffff)

/*
#define INPUT_CAR_ACCELERATE	(INPUT_MASK_FORWARDS | INPUT_MASK_MOVE | INPUT_MASK_PUNCH)
#define INPUT_CAR_DECELERATE	(INPUT_MASK_BACKWARDS | INPUT_MASK_KICK)
#define INPUT_CAR_GOFASTER		(INPUT_MASK_ACTION)
#define INPUT_CAR_SIREN			(INPUT_MASK_JUMP)
*/
#define INPUT_CAR_ACCELERATE (INPUT_MASK_FORWARDS | INPUT_MASK_MOVE | INPUT_MASK_PUNCH)
#define INPUT_CAR_DECELERATE (INPUT_MASK_BACKWARDS | INPUT_MASK_KICK)
#define INPUT_CAR_GOFASTER (INPUT_CAR_ACCELERATE | INPUT_CAR_DECELERATE)
#define INPUT_CAR_SIREN (INPUT_MASK_JUMP)

#define INPUT_MASKM_CAM_TYPE (INPUT_MASK_CAM_LEFT | INPUT_MASK_CAM_RIGHT)
#define INPUT_MASKM_CAM_SHIFT (INPUT_CAM_LEFT)
#define INPUT_MASKM_CAM1 (0)
#define INPUT_MASKM_CAM2 (INPUT_MASK_CAM_LEFT)
#define INPUT_MASKM_CAM3 (INPUT_MASK_CAM_RIGHT)
#define INPUT_MASKM_CAM4 (INPUT_MASK_CAM_LEFT | INPUT_MASK_CAM_RIGHT)

#define INPUT_MASK_DIR (INPUT_MASK_FORWARDS | INPUT_MASK_BACKWARDS | INPUT_MASK_LEFT | INPUT_MASK_RIGHT)

//
// Structs
//

//
// Data
//

//
// Functions
//

// claude-ai: apply_button_input — обработка нажатий: бой и действия персонажа (пешком)
// claude-ai: process_hardware_level_input_for_player — главная функция ввода: читает клавиатуру/геймпад, заполняет Player::Input
extern void apply_button_input(struct Thing* p_thing, SLONG input);
extern void process_hardware_level_input_for_player(Thing* p_thing);
extern void init_user_interface(void);
extern SLONG continue_action(Thing* p_person);
extern SLONG continue_moveing(Thing* p_person);
extern SLONG continue_firing(Thing* p_person);
extern SLONG person_get_in_car(Thing* p_person); // Returns TRUE if it finds a car and set the person's InCar field
extern SLONG person_get_in_specific_car(Thing* p_person, Thing* p_vehicle);

// Numbers to feed as "type" to get_hardware_input().
#define INPUT_TYPE_KEY (1 << 0)
#define INPUT_TYPE_JOY (1 << 1)

// Only sets buttons that have gone down since the last poll. This is useful for
// menus, etc.
#define INPUT_TYPE_GONEDOWN (1 << 6)

#define INPUT_TYPE_ALL (INPUT_TYPE_KEY | INPUT_TYPE_JOY)

extern ULONG get_hardware_input(UWORD type);
// Type can only be INPUT_TYPE_GONEDOWN
extern ULONG get_last_input(UWORD type);
// Allow the last input state to autorepeat, despite GONEDOWN.
extern void allow_input_autorepeat(void);

extern UBYTE joypad_button_use[16];
extern UBYTE keybrd_button_use[16];

// claude-ai: Индексы кнопок геймпада в массиве joypad_button_use[16]. Маппинг физических кнопок на логические INPUT_*.
// claude-ai: Пример: joypad_button_use[JOYPAD_BUTTON_KICK] = номер физической кнопки DirectInput.
#define JOYPAD_BUTTON_KICK 0
#define JOYPAD_BUTTON_PUNCH 1
#define JOYPAD_BUTTON_JUMP 2
#define JOYPAD_BUTTON_ACTION 3
#define JOYPAD_BUTTON_MOVE 4
#define JOYPAD_BUTTON_START 5
#define JOYPAD_BUTTON_SELECT 6
#define JOYPAD_BUTTON_CAMERA 7
#define JOYPAD_BUTTON_CAM_LEFT 8
#define JOYPAD_BUTTON_CAM_RIGHT 9
#define JOYPAD_BUTTON_1STPERSON 10

#define KEYBRD_BUTTON_LEFT 11
#define KEYBRD_BUTTON_RIGHT 12
#define KEYBRD_BUTTON_FORWARDS 13
#define KEYBRD_BUTTON_BACK 14

#endif // FALLEN_HEADERS_INTERFAC_H
