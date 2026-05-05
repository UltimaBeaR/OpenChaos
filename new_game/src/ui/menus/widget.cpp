#include "ui/menus/widget.h"
#include "ui/menus/widget_globals.h"
#include "engine/audio/mfx.h"
#include "assets/sound_id.h"
#include "engine/input/keyboard_globals.h"
#include "engine/input/keyboard.h"
#include "engine/input/input_frame.h"
#include "engine/graphics/pipeline/polypage.h" // PolyPage::UIModeScope
#include "game/input_actions.h"
#include "engine/graphics/graphics_engine/game_graphics_engine.h"
#include "engine/platform/sdl3_bridge.h"

// Sound-to-wave ID mapping for widget events. Uses S_MENU_START/S_MENU_END from sound_id.h.
// uc_orig: _WS_MOVE (fallen/Source/widget.cpp)
#define _WS_MOVE S_MENU_START
// uc_orig: _WS_FADEIN (fallen/Source/widget.cpp)
#define _WS_FADEIN S_MENU_START + 2
// uc_orig: _WS_FADEOUT (fallen/Source/widget.cpp)
#define _WS_FADEOUT S_MENU_START + 1
// uc_orig: _WS_OK (fallen/Source/widget.cpp)
#define _WS_OK S_MENU_END
// uc_orig: _WS_FAIL (fallen/Source/widget.cpp)
#define _WS_FAIL S_MENU_END - 1
// uc_orig: _WS_BLIP (fallen/Source/widget.cpp)
#define _WS_BLIP S_MENU_START + 3

// InkeyToAscii tables defined in Controls.cpp (not yet migrated).
// uc_orig: InkeyToAscii (fallen/Source/Controls.cpp)
extern UBYTE InkeyToAscii[];
// uc_orig: InkeyToAsciiShift (fallen/Source/Controls.cpp)
extern UBYTE InkeyToAsciiShift[];

// Plays a widget sound event by mapping WS_* codes to wave IDs.
// uc_orig: WIDGET_snd (fallen/Source/widget.cpp)
void WIDGET_snd(SLONG snd)
{
    switch (snd) {
    case WS_MOVE:
        snd = _WS_MOVE;
        break;
    case WS_FADEOUT:
        snd = _WS_FADEOUT;
        break;
    case WS_FADEIN:
        snd = _WS_FADEIN;
        break;
    case WS_OK:
        snd = _WS_OK;
        break;
    case WS_FAIL:
        snd = _WS_FAIL;
        break;
    case WS_BLIP:
        snd = _WS_BLIP;
        break;
    }
    MFX_play_ambient(0, snd, MFX_REPLACE);
}

// uc_orig: FORM_KeyProc (fallen/Source/widget.cpp)
//
// Internal bridge consumer for the gamepad-to-keyboard channel: the
// gamepad-to-keyboard bridge below synthesizes virtual key presses via
// input_frame_inject_key_press(KB_ENTER) etc., and FORM_KeyProc consumes
// them via input_key_press_pending + input_key_consume. Same pattern as
// hardware key consumers — input_frame is the single message bus.
static BOOL FORM_KeyProc(SLONG key)
{
    if (input_key_press_pending(key)) {
        input_key_consume(key);
        EatenKey = 1;
        return 1;
    }
    return 0;
}

// uc_orig: FORM_Process (fallen/Source/widget.cpp)
SLONG FORM_Process(Form* form)
{
    CBYTE key;
    Widget* lastfocus;
    static int lastx = 0, lasty = 0;
    static int input = 0, lastinput = 0;
    static int ticker = 0;

    if (!form->age)
        if (form->children)
            WIDGET_snd(WS_FADEIN);
        else
            WIDGET_snd(WS_FAIL);

    WidgetTick++;
    form->age++;

    const UBYTE last_key = input_last_key();
    if (last_key) {
        // ShiftFlag is the level-state Shift modifier mirrored from
        // input_frame's event-tracked state (KB_LSHIFT || KB_RSHIFT).
        key = ShiftFlag ? InkeyToAsciiShift[last_key] : InkeyToAscii[last_key];
        if (key == 8)
            key = 127;
        if (!key)
            switch (last_key) {
            case KB_UP:
                key = 11;
                break;
            case KB_RIGHT:
                key = 9;
                break;
            case KB_LEFT:
                key = 8;
                break;
            case KB_DOWN:
                key = 10;
                break;
            case KB_ESC:
                key = 27;
                break;
            case KB_ENTER:
                key = 13;
                break;
            case KB_PGUP:
                key = 1;
                break;
            case KB_PGDN:
                key = 2;
                break;
            case KB_HOME:
                key = 3;
                break;
            case KB_END:
                key = 4;
                break;
            case KB_DEL:
                key = 5;
                break;
            }
    } else
        key = 0;

    lastinput = input;

    input = get_hardware_input(INPUT_TYPE_JOY);
    if (input && (input != lastinput) && (ticker < 1)) {
        // Gamepad → keyboard bridge: synthesise key-press events into
        // input_frame so FORM_KeyProc (and any other pending-press consumer)
        // sees the gamepad press as if it were a keyboard press. Internal
        // closed channel — no leak to physical-keyboard text input because
        // input_frame_inject_key_press deliberately doesn't update LastKey.
        // Cross/A = confirm (JUMP maps to Cross in PS1 Config 0).
        if (input & INPUT_MASK_JUMP) {
            key = 13;
            input_frame_inject_key_press(KB_ENTER);
        }
        // Triangle/Y = cancel/back (KICK+CANCEL maps to Triangle).
        if (input & INPUT_MASK_CANCEL) {
            key = 27;
            input_frame_inject_key_press(KB_ESC);
        }
        if (input & INPUT_MASK_FORWARDS) {
            key = 11;
            input_frame_inject_key_press(KB_UP);
        }
        if (input & INPUT_MASK_BACKWARDS) {
            key = 10;
            input_frame_inject_key_press(KB_DOWN);
        }
        if (input & INPUT_MASK_LEFT) {
            key = 8;
            input_frame_inject_key_press(KB_LEFT);
        }
        if (input & INPUT_MASK_RIGHT) {
            key = 9;
            input_frame_inject_key_press(KB_RIGHT);
        }
        if (input & INPUT_MASK_START) {
            form->returncode = -69;
        }
        ticker = 10;
    }
    ticker--;

    if (key) {
        if (form->focus && form->focus->methods->Char && form->focus->methods->Char(form->focus, key))
            ;
        else {
            EatenKey = 0;
            lastfocus = form->focus;
            if (FORM_KeyProc(KB_UP)) {
                FORM_Focus(form, 0, -1);
                WIDGET_snd(WS_MOVE);
            }
            if (FORM_KeyProc(KB_DOWN)) {
                FORM_Focus(form, 0, 1);
                WIDGET_snd(WS_MOVE);
            }
            if (FORM_KeyProc(KB_HOME)) {
                FORM_Focus(form, form->children, 0);
                WIDGET_snd(WS_MOVE);
            }
            if (FORM_KeyProc(KB_END)) {
                FORM_Focus(form, form->children, -1);
                WIDGET_snd(WS_MOVE);
            }
            if (FORM_KeyProc(KB_ENTER))
                if (form->focus && form->focus->methods->Push)
                    form->focus->methods->Push(form->focus);
            if (FORM_KeyProc(KB_TAB) && form->focus) {
                if (form->focus->methods->Char)
                    form->focus->methods->Char(form->focus, 27);
                FORM_Focus(form, 0, ShiftFlag ? -1 : 1);
                WIDGET_snd(WS_MOVE);
            }
            if (lastfocus != form->focus)
                form->proc(form, 0, WFN_FOCUS);
            if (input_last_key() && form->proc && !EatenKey)
                form->proc(form, 0, WFN_CHAR);
        }
    } else {
        SLONG res;
        int ptx, pty;
        sdl3_get_global_mouse_pos(&ptx, &pty);
        if (!ge_is_fullscreen()) {
            int wx, wy;
            sdl3_window_get_position(&wx, &wy);
            ptx -= wx;
            pty -= wy;
        }
        if ((ptx != lastx) || (pty != lasty)) {
            lastx = ptx;
            lasty = pty;
            Widget* scan = FORM_GetWidgetFromPoint(form, TO_WIDGETPNT(lastx, lasty));
            if (scan && (scan != form->focus)) {
                if (form->focus && form->focus->methods->Char)
                    form->focus->methods->Char(form->focus, 27);
                FORM_Focus(form, scan, 0);
            }
        }
        if (input_mouse_button_held(0)) {
            Widget* scan = FORM_GetWidgetFromPoint(form, TO_WIDGETPNT(lastx, lasty));
            if (scan && scan->methods->Push)
                scan->methods->Push(scan);
            else if (scan && scan->methods->Char)
                scan->methods->Char(scan, 13);
        }
    }

    input_last_key_consume();

    return form->returncode;
}

// uc_orig: FORM_GetWidgetFromPoint (fallen/Source/widget.cpp)
Widget* FORM_GetWidgetFromPoint(Form* form, WidgetPoint pt)
{
    Widget* scan = form->children;
    while (scan) {
        if ((!(scan->state & WIDGET_STATE_DISABLED)) && scan->methods->HitTest && (scan->methods->HitTest(scan, pt.x, pt.y)))
            return scan;
        scan = scan->next;
    }
    return 0;
}

// uc_orig: FORM_Draw (fallen/Source/widget.cpp)
void FORM_Draw(Form* form)
{
    PolyPage::UIModeScope _ui_scope(ui_coords::UIAnchor::CENTER_CENTER);
    Widget* walk;

    walk = form->children;
    while (walk) {
        walk->methods->Draw(walk);
        walk = walk->next;
    }
    form->proc(form, 0, WFN_PAINT);
}

// uc_orig: FORM_Free (fallen/Source/widget.cpp)
void FORM_Free(Form* form)
{
    Widget* last;

    while (form->children) {
        last = form->children;
        form->children = form->children->next;
        last->form = 0;
        last->methods->Free(last);
    }
    MemFree(form);
}

// uc_orig: FORM_Focus (fallen/Source/widget.cpp)
Widget* FORM_Focus(Form* form, Widget* widget, SBYTE direction)
{
    if (form->focus)
        WIDGET_SetState(form->focus, 0, WIDGET_STATE_FOCUS);
    if (!widget)
        widget = form->focus;
    form->focus = widget;
    if ((!widget) || ((widget->state & (WIDGET_STATE_BLOCKFOCUS | WIDGET_STATE_DISABLED)) && !direction))
        return (form->focus = 0);
    if (direction) {
        if (direction == -1) {
            if (form->focus->prev)
                form->focus = form->focus->prev;
            else
                while (form->focus->next)
                    form->focus = form->focus->next;
        } else {
            if (form->focus->next)
                form->focus = form->focus->next;
            else
                while (form->focus->prev)
                    form->focus = form->focus->prev;
        }
        if (form->focus->state & (WIDGET_STATE_BLOCKFOCUS | WIDGET_STATE_DISABLED))
            return FORM_Focus(form, 0, direction);
    }
    if (form->focus)
        WIDGET_SetState(form->focus, WIDGET_STATE_FOCUS);
    return widget;
}
