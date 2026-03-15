// Level.cpp
// Guy Simmons, 29th January 1998.
// claude-ai: ОБЗОР ФАЙЛА — Level.cpp
// claude-ai: Загрузка и инициализация уровня из бинарного .lev файла.
// claude-ai: ВНИМАНИЕ: это ранняя версия Level.cpp — в финальной игре загрузка уровней
// claude-ai: значительно сложнее (включает .pam геометрию, PAP heightmap и т.д.).
// claude-ai: Данный файл обрабатывает только "Things" слой уровня (объекты, точки появления,
// claude-ai: маршрутные точки, условия и команды).
// claude-ai:
// claude-ai: ФОРМАТ ФАЙЛА .lev:
// claude-ai:   Путь: "Levels\Level%03d.lev"  (например, Level001.lev)
// claude-ai:   Структура (бинарная, последовательная):
// claude-ai:     UBYTE  version (должен быть 0)
// claude-ai:     ULONG  thing_count    → N записей ThingDef (объекты уровня)
// claude-ai:     ULONG  waypoint_count → N записей WaypointDef (маршрутные точки AI)
// claude-ai:     ULONG  clist_count    → N записей ConditionListDef (условия скриптов)
// claude-ai:     ULONG  comlist_count  → N записей CommandListDef (команды скриптов)
// claude-ai:   Каждая запись начинается с UWORD Version (сейчас всегда 0).
// claude-ai:
// claude-ai: ПРОЦЕСС ЗАГРУЗКИ (load_level):
// claude-ai:   1. init_waypoints / init_clists / init_comlists — сброс пулов
// claude-ai:   2. Загрузка Things → создание персонажей, переключателей, спецпредметов
// claude-ai:   3. Загрузка Waypoints → создание маршрутных точек AI
// claude-ai:   4. Ремаппинг Waypoints (EdRef → runtime индексы)
// claude-ai:   5. Загрузка ConditionLists (условия триггеров)
// claude-ai:   6. Ремаппинг Conditions (EdRef → runtime индексы)
// claude-ai:   7. Загрузка CommandLists (команды AI)
// claude-ai:   8. Ремаппинг Commands
// claude-ai:   9. Ремаппинг всех Thing-ссылок (SwitchThing, Waypoint, ComList)
// claude-ai:
// claude-ai: ДРУГИЕ ЧАСТИ ЗАГРУЗКИ УРОВНЯ (НЕ в этом файле):
// claude-ai:   - .pam файл (геометрия карты, здания) — Building.cpp / Map.cpp
// claude-ai:   - PAP_Hi[128×128] heightmap — mav.cpp (MAV_init)
// claude-ai:   - Текстуры — Textures/ директория
// claude-ai:   - 3D меши — Meshes/ директория
// claude-ai:   - Миссионный скрипт (.ucm) — eway.cpp (EWAY система)
// claude-ai:   - Звуки — data/ директория
// claude-ai:
// claude-ai: РЕМАППИНГ EdRef → runtime:
// claude-ai:   EdThingRef, EdWaypointRef, EdConListRef, EdComListRef — редакторские индексы
// claude-ai:   thing_map[], waypoint_map[], conlist_map[], comlist_map[] — таблицы перевода
// claude-ai:   После загрузки всех объектов выполняется patching всех перекрёстных ссылок.
// claude-ai:
// claude-ai: PSX-специфичный код (FileOpen/FileRead макросы) — не переносить.

#include "Game.h"
#include "Command.h"
#include "statedef.h"

// claude-ai: PSX-специфичные макросы файлового ввода-вывода — не переносить.
// claude-ai: На PC/новой версии использовать стандартный fopen/fread или платформонезависимый VFS.

// claude-ai: Таблицы ремаппинга: editor-индекс → runtime-индекс.
// claude-ai: Заполняются во время загрузки; используются для патчинга перекрёстных ссылок.
// claude-ai: После загрузки уровня эти массивы можно считать временными (не нужны в runtime).
UWORD comlist_map[MAX_COMLISTS],
    conlist_map[MAX_CLISTS],
    waypoint_map[MAX_WAYPOINTS];
THING_INDEX thing_map[MAX_THINGS];

void store_player_pos(ThingDef* the_def);

//---------------------------------------------------------------

// claude-ai: load_thing_def() — читает одну запись ThingDef из .lev файла.
// claude-ai: ThingDef содержит: Version, Class (CLASS_*), Genus, X/Y/Z координаты,
// claude-ai:   CommandRef (ссылка на команду AI), Data[N] (класс-специфичные данные),
// claude-ai:   EdThingRef (editor-индекс, нужен для ремаппинга).
// claude-ai: По классу вызывает соответствующую create_* функцию:
// claude-ai:   CLASS_PERSON   → create_person()
// claude-ai:   CLASS_SWITCH   → create_switch()
// claude-ai:   CLASS_SPECIAL  → create_special()
// claude-ai:   CLASS_VEHICLE  → закомментировано (в этой версии транспорт не создаётся через .lev)
// claude-ai:   CLASS_PLAYER   → store_player_pos() (точка появления игрока)
// claude-ai:   CLASS_BUILDING → настройка уже существующего здания (locked, switch ссылки)
BOOL load_thing_def(MFFileHandle the_file)
{
    Thing* b_thing;
    ThingDef the_def;

    FileRead(the_file, &the_def.Version, sizeof(the_def.Version));
    if (the_def.Version == 0) {
        FileRead(the_file, &the_def.Class, sizeof(the_def.Class));
        FileRead(the_file, &the_def.Genus, sizeof(the_def.Genus));
        FileRead(the_file, &the_def.X, sizeof(the_def.X));
        FileRead(the_file, &the_def.Y, sizeof(the_def.Y));
        FileRead(the_file, &the_def.Z, sizeof(the_def.Z));
        FileRead(the_file, &the_def.CommandRef, sizeof(the_def.CommandRef));
        FileRead(the_file, &the_def.Data, sizeof(the_def.Data));
        FileRead(the_file, &the_def.EdThingRef, sizeof(the_def.EdThingRef));
    }

    switch (the_def.Class) {
    case CLASS_NONE:
        break;
    case CLASS_PLAYER:
        store_player_pos(&the_def);
        break;
    case CLASS_CAMERA:
        break;
    case CLASS_PROJECTILE:
        break;
    case CLASS_BUILDING:
        b_thing = TO_THING(building_list[the_def.Data[0]].ThingIndex);

        if (the_def.Data[1])
            b_thing->Flags |= FLAGS_LOCKED;
        b_thing->SwitchThing = the_def.Data[2];
        break;
    case CLASS_PERSON:
        thing_map[the_def.EdThingRef] = create_person(&the_def);
        break;
    case CLASS_FURNITURE:
        break;
    case CLASS_SWITCH:
        thing_map[the_def.EdThingRef] = create_switch(&the_def);
        break;
    case CLASS_VEHICLE:
        //	Mark	-	Create a vehicle here.

        // claude-ai: Создание транспорта из .lev файла — закомментировано в этой версии.
        // claude-ai: В финальной игре транспорт создаётся иначе (вероятно, через .pam или EWAY).
        // claude-ai: Этот код — незаконченная ранняя реализация; не переносить как есть.
        {
            SLONG prim;

            if (the_def.Genus == 1) {
                prim = 48; // PRIM_OBJ_CAR;
            } else if (the_def.Genus == 2) {
                prim = 48; // PRIM_OBJ_VAN;
            } else {
                prim = 48; // PRIM_OBJ_LION;
            }
            /*
                                            thing_map[the_def.EdThingRef] = VEH_create(
                                                                                                                    the_def.X << 8,
                                                                                                                    the_def.Y << 8,
                                                                                                                    the_def.Z << 8,
                                                                                                                    0,0,0,
                                                                                                                    1); //1 is van


                                            TO_THING(thing_map[the_def.EdThingRef])->Genus.Furniture->Command = the_def.Data[0];
            */
        }

        break;
    case CLASS_SPECIAL:
        thing_map[the_def.EdThingRef] = create_special(&the_def);
        break;
    }

    return TRUE;
}

//---------------------------------------------------------------

// claude-ai: load_waypoint_def() — читает маршрутную точку AI из .lev файла.
// claude-ai: WaypointDef: Version, Next/Prev (editor-индексы связного списка), X/Y/Z, EdWaypointRef.
// claude-ai: После загрузки всех waypoints — ремаппинг Next/Prev через waypoint_map[].
// claude-ai: Waypoints — связный список; используются AI-патрулями (COM_PATROL_WAYPOINT).
BOOL load_waypoint_def(MFFileHandle the_file)
{
    UWORD the_wp;
    WaypointDef the_def;

    FileRead(the_file, &the_def.Version, sizeof(the_def.Version));
    if (the_def.Version == 0) {
        FileRead(the_file, &the_def.Next, sizeof(the_def.Next));
        FileRead(the_file, &the_def.Prev, sizeof(the_def.Prev));
        FileRead(the_file, &the_def.X, sizeof(the_def.X));
        FileRead(the_file, &the_def.Y, sizeof(the_def.Y));
        FileRead(the_file, &the_def.Z, sizeof(the_def.Z));
        FileRead(the_file, &the_def.EdWaypointRef, sizeof(the_def.EdWaypointRef));
    }

    the_wp = alloc_waypoint();
    waypoint_map[the_def.EdWaypointRef] = the_wp;

    waypoints[the_wp].Next = the_def.Next;
    waypoints[the_wp].Prev = the_def.Prev;
    waypoints[the_wp].X = the_def.X;
    waypoints[the_wp].Y = the_def.Y;
    waypoints[the_wp].Z = the_def.Z;

    return TRUE;
}

//---------------------------------------------------------------

BOOL load_condition_def(MFFileHandle the_file, ConditionDef* the_def)
{
    FileRead(the_file, &the_def->Version, sizeof(the_def->Version));
    if (the_def->Version == 0) {
        FileRead(the_file, &the_def->Flags, sizeof(the_def->Flags));
        FileRead(the_file, &the_def->ConditionType, sizeof(the_def->ConditionType));
        FileRead(the_file, &the_def->GroupRef, sizeof(the_def->GroupRef));
        FileRead(the_file, &the_def->Data1, sizeof(the_def->Data1));
        FileRead(the_file, &the_def->Data2, sizeof(the_def->Data2));
        FileRead(the_file, &the_def->Data3, sizeof(the_def->Data3));
    }

    return TRUE;
}

//---------------------------------------------------------------

BOOL load_command_def(MFFileHandle the_file, CommandDef* the_def)
{
    FileRead(the_file, &the_def->Version, sizeof(the_def->Version));
    if (the_def->Version == 0) {
        FileRead(the_file, &the_def->Flags, sizeof(the_def->Flags));
        FileRead(the_file, &the_def->CommandType, sizeof(the_def->CommandType));
        FileRead(the_file, &the_def->GroupRef, sizeof(the_def->GroupRef));
        FileRead(the_file, &the_def->Data1, sizeof(the_def->Data1));
        FileRead(the_file, &the_def->Data2, sizeof(the_def->Data2));
        FileRead(the_file, &the_def->Data3, sizeof(the_def->Data3));
    }

    return TRUE;
}

//---------------------------------------------------------------

BOOL load_clist_def(MFFileHandle the_file)
{
    SLONG c0;
    Condition* the_condition;
    ConditionDef the_con_def;
    ConditionList* the_clist;
    ConditionListDef the_def;

    FileRead(the_file, &the_def.Version, sizeof(the_def.Version));
    if (the_def.Version == 0) {
        FileRead(the_file, the_def.ListName, sizeof(the_def.ListName));
        FileRead(the_file, &the_def.ConditionCount, sizeof(the_def.ConditionCount));
        FileRead(the_file, &the_def.EdConListRef, sizeof(the_def.EdConListRef));

        //	Create a condition list.
        the_clist = alloc_clist();
        conlist_map[the_def.EdConListRef] = CONLIST_NUMBER(the_clist);

        for (c0 = 0; c0 < the_def.ConditionCount; c0++) {
            load_condition_def(the_file, &the_con_def);

            //	Create the condition.
            the_condition = alloc_condition();
            add_condition(the_clist, the_condition);

            the_condition->Flags = the_con_def.Flags;
            the_condition->ConditionType = the_con_def.ConditionType;
            the_condition->GroupRef = the_con_def.GroupRef;
            the_condition->Data1 = the_con_def.Data1;
            the_condition->Data2 = the_con_def.Data2;
            the_condition->Data3 = the_con_def.Data3;
        }
    }

    return TRUE;
}

//---------------------------------------------------------------

BOOL load_comlist_def(MFFileHandle the_file)
{
    SLONG c0;
    Command* the_command;
    CommandDef the_com_def;
    CommandList* the_comlist;
    CommandListDef the_def;

    FileRead(the_file, &the_def.Version, sizeof(the_def.Version));
    if (the_def.Version == 0) {
        FileRead(the_file, the_def.ListName, sizeof(the_def.ListName));
        FileRead(the_file, &the_def.CommandCount, sizeof(the_def.CommandCount));
        FileRead(the_file, &the_def.EdComListRef, sizeof(the_def.EdComListRef));

        //	Create a command list.
        the_comlist = alloc_comlist();
        comlist_map[the_def.EdComListRef] = COMLIST_NUMBER(the_comlist);

        for (c0 = 0; c0 < the_def.CommandCount; c0++) {
            load_command_def(the_file, &the_com_def);

            //	Create the command.
            the_command = alloc_command();
            add_command(the_comlist, the_command);

            the_command->Flags = the_com_def.Flags;
            the_command->CommandType = the_com_def.CommandType;
            the_command->GroupRef = the_com_def.GroupRef;
            the_command->Data1 = the_com_def.Data1;
            the_command->Data2 = the_com_def.Data2;
            the_command->Data3 = the_com_def.Data3;
        }
    }

    return TRUE;
}

//---------------------------------------------------------------

// claude-ai: load_level() — главная функция загрузки уровня.
// claude-ai: Файл: "Levels\Level%03d.lev" (бинарный формат, см. описание выше).
// claude-ai: Последовательность: init → вещи → маршрутные точки → ремаппинг WP →
// claude-ai:   условия → ремаппинг условий → команды → ремаппинг команд → ремаппинг Thing-ссылок.
// claude-ai: Выделение ресурсов: alloc_waypoint(), alloc_clist(), alloc_comlist(), alloc_condition(),
// claude-ai:   alloc_command() — берут из статических пулов (не динамический heap).
// claude-ai: Освобождение ресурсов при выгрузке уровня: init_waypoints() / init_clists() / init_comlists()
// claude-ai:   сбрасывают пулы (вызываются в начале следующего load_level или при выходе).
BOOL load_level(ULONG level)
{
    CBYTE level_name[256];
    UBYTE version;
    ULONG c0,
        clist_count,
        thing_count,
        waypoint_count;
    Command* the_command;
    Condition* the_condition;
    MFFileHandle level_file;
    Thing* current_thing;

    init_waypoints();
    init_clists();
    init_comlists();

    ZeroMemory(comlist_map, sizeof(comlist_map));
    ZeroMemory(conlist_map, sizeof(conlist_map));
    ZeroMemory(waypoint_map, sizeof(waypoint_map));
    ZeroMemory(thing_map, sizeof(thing_map));

    // claude-ai: Путь к файлу уровня: "Levels\Level001.lev" и т.д.
    // claude-ai: Формат .lev — бинарный, специфичный для этого движка (не стандартный).
    // claude-ai: version == 0 — единственная поддерживаемая версия формата в этом коде.
    // claude-ai: При портировании: заменить FileOpen/FileRead на стандартный ввод-вывод или VFS.
    sprintf(level_name, "Levels\\Level%3.3d.lev", level);
    level_file = FileOpen(level_name);
    if (level_file != FILE_OPEN_ERROR) {
        FileRead(level_file, &version, sizeof(version));
        if (version == 0) {
            //
            //	Load Things.
            //
            FileRead(level_file, &thing_count, sizeof(thing_count));
            while (thing_count) {
                load_thing_def(level_file);
                thing_count--;
            }

            //
            //	Load Waypoints.
            //
            FileRead(level_file, &waypoint_count, sizeof(waypoint_count));
            while (waypoint_count) {
                load_waypoint_def(level_file);
                waypoint_count--;
            }
            //
            //	Remap the Waypoints.
            //
            // claude-ai: Патчинг Next/Prev ссылок waypoints: editor-индексы → runtime-индексы.
            // claude-ai: Waypoints образуют двусвязный список; ссылки нужно исправить после
            // claude-ai: загрузки всех точек, т.к. в файле хранятся EdWaypointRef (editor ID).
            for (c0 = 1; c0 < MAX_WAYPOINTS; c0++) {
                if (waypoints[c0].Used) {
                    waypoints[c0].Next = waypoint_map[waypoints[c0].Next];
                    waypoints[c0].Prev = waypoint_map[waypoints[c0].Prev];
                }
            }

            //
            //	Load Conditions.
            //
            FileRead(level_file, &clist_count, sizeof(clist_count));
            while (clist_count) {
                load_clist_def(level_file);
                clist_count--;
            }
            //
            //	Remap the Conditions.
            //
            for (c0 = 1; c0 < MAX_CONDITIONS; c0++) {
                if (conditions[c0].Used) {
                    the_condition = &conditions[c0];
                    switch (the_condition->ConditionType) {
                    case CON_NONE:
                        break;
                    case CON_THING_DEAD:
                        the_condition->Data1 = thing_map[the_condition->Data1];
                        break;
                    case CON_ALL_GROUP_DEAD:
                        break;
                    case CON_PERCENT_GROUP_DEAD:
                        break;
                    case CON_THING_NEAR_PLAYER:
                        the_condition->Data1 = thing_map[the_condition->Data1];
                        the_condition->Data2 = 65536;
                        break;
                    case CON_GROUP_NEAR_PLAYER:
                        break;
                    case CON_CLASS_NEAR_PLAYER:
                        break;
                    case CON_THING_NEAR_THING:
                        the_condition->Data1 = thing_map[the_condition->Data1];
                        the_condition->Data2 = thing_map[the_condition->Data2];
                        the_condition->Data3 = 65536;
                        break;
                    case CON_GROUP_NEAR_THING:
                        break;
                    case CON_CLASS_NEAR_THING:
                        break;
                    case CON_CLASS_COUNT:
                        break;
                    case CON_GROUP_COUNT:
                        break;
                    case CON_SWITCH_TRIGGERED:
                        the_condition->Data1 = thing_map[the_condition->Data1];
                        break;
                    case CON_TIME:
                        break;
                    case CON_CLIST_FULFILLED:
                        the_condition->Data1 = conlist_map[the_condition->Data1];
                        break;
                    }
                }
            }

            //
            //	Load Commands.
            //
            FileRead(level_file, &clist_count, sizeof(clist_count));
            while (clist_count) {
                load_comlist_def(level_file);
                clist_count--;
            }
            //
            //	Remap the Commands.
            //
            for (c0 = 1; c0 < MAX_COMMANDS; c0++) {
                if (commands[c0].Used) {
                    the_command = &commands[c0];
                    switch (the_command->CommandType) {
                    case COM_NONE:
                        break;
                    case COM_ATTACK_PLAYER:
                        break;
                    case COM_ATTACK_THING:
                    case COM_DEFEND_THING:
                    case COM_WAIT_FOR_TRIGGER:
                        the_command->Data1 = thing_map[the_command->Data1];
                        break;
                    case COM_ATTACK_GROUP:
                        break;
                    case COM_ATTACK_CLASS:
                        break;
                    case COM_DEFEND_PLAYER:
                        break;
                    case COM_DEFEND_GROUP:
                        break;
                    case COM_DEFEND_CLASS:
                        break;
                    case COM_PATROL_WAYPOINT:
                        //	I think a waypoint needs remapping here.
                        break;
                    case COM_START_TIMER:
                        break;
                    case COM_WAIT_FOR_CLIST:
                        the_command->Data1 = conlist_map[the_command->Data1];
                        break;
                    case COM_FOLLOW_PLAYER:
                        break;
                    }

                    switch (the_command->Data2) {
                    case COM_S_NONE:
                        break;
                    case COM_S_UNTIL_TRIGGER:
                    case COM_S_WHILE_TRIGGER:
                        the_command->Data3 = thing_map[the_command->Data3];
                        break;
                    case COM_S_UNTIL_CLIST:
                    case COM_S_WHILE_CLIST:
                        the_command->Data3 = conlist_map[the_command->Data3];
                        break;
                    }
                }
            }

            //
            //	Remap all the Thing references.
            //
            for (c0 = 1; c0 < MAX_THINGS; c0++) {
                current_thing = TO_THING(c0);
                switch (current_thing->Class) {
                case CLASS_NONE:
                    break;
                case CLASS_PLAYER:
                    break;
                case CLASS_CAMERA:
                    break;
                case CLASS_PROJECTILE:
                    break;
                case CLASS_BUILDING:
                    current_thing->SwitchThing = thing_map[current_thing->SwitchThing];
                    break;
                case CLASS_PERSON:
                    if (current_thing->Genus.Person->Command) {
                        current_thing->Genus.Person->ComList = &com_lists[comlist_map[current_thing->Genus.Person->Command]];
                    }
                    break;
                case CLASS_FURNITURE:
                    break;
                case CLASS_SWITCH:
                    switch (current_thing->Genus.Switch->SwitchType) {
                    case SWITCH_NONE:
                        break;
                    case SWITCH_PLAYER:
                        break;
                    case SWITCH_THING:
                        current_thing->Genus.Switch->Scanee = thing_map[current_thing->Genus.Switch->Scanee];
                        break;
                    case SWITCH_GROUP:
                        break;
                    case SWITCH_CLASS:
                        break;
                    }
                    break;
                case CLASS_VEHICLE:

                    // claude-ai: Патчинг команды транспорта: editor-индекс ComList → runtime Command.
                    // claude-ai: Если у машины есть COM_PATROL_WAYPOINT — ремапит и Waypoint.
                    // claude-ai: Машина с командой принудительно переводится в STATE_FDRIVING
                    // claude-ai: со всеми четырьмя колёсами на земле (FLAG_FURN_WHEEL*_GRIP).
                    if (current_thing->Genus.Furniture->Command) {
                        current_thing->Genus.Furniture->Command = COMMAND_NUMBER(com_lists[comlist_map[current_thing->Genus.Furniture->Command]].TheList);

                        if (current_thing->Genus.Furniture->Command) {
                            struct Command* com = TO_COMMAND(current_thing->Genus.Furniture->Command);

                            if (com->CommandType == COM_PATROL_WAYPOINT) {
                                current_thing->Genus.Furniture->Waypoint = waypoint_map[com->Data1];
                            }
                        }

                        //
                        // A car must be driving to execute commands... with all its wheels on
                        // the ground!
                        //

                        set_state_function(current_thing, STATE_FDRIVING);

                        current_thing->Genus.Furniture->Flags |= FLAG_FURN_DRIVING;
                        current_thing->Genus.Furniture->Flags |= FLAG_FURN_WHEEL1_GRIP;
                        current_thing->Genus.Furniture->Flags |= FLAG_FURN_WHEEL2_GRIP;
                        current_thing->Genus.Furniture->Flags |= FLAG_FURN_WHEEL3_GRIP;
                        current_thing->Genus.Furniture->Flags |= FLAG_FURN_WHEEL4_GRIP;
                    }

                    break;
                case CLASS_SPECIAL:
                    break;
                }
            }
        }
        FileClose(level_file);
    }
    return TRUE;
}

//---------------------------------------------------------------
