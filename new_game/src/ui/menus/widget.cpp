#include "ui/menus/widget.h"
#include "ui/menus/widget_globals.h"
#include "engine/audio/mfx.h"
#include "assets/sound_id.h"
#include "engine/input/keyboard_globals.h"
#include "engine/input/keyboard.h"
#include "engine/input/input_frame.h"
#include "engine/graphics/pipeline/polypage.h" // PolyPage::UIModeScope
#include "game/input_actions.h"
#include "game/action_map/act_menu.h" // ACT_MENU_*

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
// Form-level keyboard / gamepad press consumer. Returns 1 (and consumes) on
// either a sticky keyboard press_pending of kb_code OR — when gp_btn_idx is
// non-negative — a sticky gamepad press_pending of that button. Symmetric
// keyboard / gamepad channel — no synthesis / bridging needed.
static BOOL FORM_KeyProc(SLONG kb_code, SLONG gp_btn_idx = -1)
{
    const bool kb = input_key_press_pending(kb_code);
    const bool gp = (gp_btn_idx >= 0) && input_btn_press_pending(gp_btn_idx);
    if (kb || gp) {
        if (kb) input_key_consume(kb_code);
        if (gp) input_btn_consume(gp_btn_idx);
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
        // input_frame's event-tracked state (KKEY_LEFT_SHIFT || KKEY_RIGHT_SHIFT).
        key = ShiftFlag ? InkeyToAsciiShift[last_key] : InkeyToAscii[last_key];
        if (key == 8)
            key = 127;
        if (!key)
            switch (last_key) {
            case KKEY_UP:
                key = 11;
                break;
            case KKEY_RIGHT:
                key = 9;
                break;
            case KKEY_LEFT:
                key = 8;
                break;
            case KKEY_DOWN:
                key = 10;
                break;
            case KKEY_ESCAPE:
                key = 27;
                break;
            case KKEY_ENTER:
                key = 13;
                break;
            case KKEY_PAGE_UP:
                key = 1;
                break;
            case KKEY_PAGE_DOWN:
                key = 2;
                break;
            case KKEY_HOME:
                key = 3;
                break;
            case KKEY_END:
                key = 4;
                break;
            case KKEY_DELETE:
                key = 5;
                break;
            }
    } else
        key = 0;

    lastinput = input;

    input = get_hardware_input(INPUT_TYPE_JOY);
    if (input && (input != lastinput) && (ticker < 1)) {
        // Translate the gamepad-level input mask into form-ASCII keys (these
        // codes feed `methods->Char` below — they aren't keyboard scancodes,
        // they're widget-internal ASCII-ish codes 8..27). The form-ASCII keys
        // happen to match what KKEY_* would translate to in the InkeyToAscii
        // table above, so a keyboard arrow and a D-Pad press both deliver the
        // same form-ASCII to widget Char handlers.
        //
        // FORM_KeyProc below takes both a KKEY scancode and a GBTN index, so
        // form-level navigation reads the gamepad directly — no synthesis
        // through the input_frame message bus needed.
        if (input & INPUT_MASK_JUMP)      key = 13;  // confirm — Cross/A
        if (input & INPUT_MASK_CANCEL)    key = 27;  // cancel — Circle/B
        if (input & INPUT_MASK_FORWARDS)  key = 11;  // nav up — DPad / stick
        if (input & INPUT_MASK_BACKWARDS) key = 10;  // nav down
        if (input & INPUT_MASK_LEFT)      key = 8;   // nav left
        if (input & INPUT_MASK_RIGHT)     key = 9;   // nav right
        if (input & INPUT_MASK_START)     form->returncode = -69;
        ticker = 10;
    }
    ticker--;

    if (key) {
        if (form->focus && form->focus->methods->Char && form->focus->methods->Char(form->focus, key))
            ;
        else {
            EatenKey = 0;
            lastfocus = form->focus;
            if (FORM_KeyProc(ACT_MENU_NAV_UP_KKEY, ACT_MENU_NAV_UP_GBTN)) {
                FORM_Focus(form, 0, -1);
                WIDGET_snd(WS_MOVE);
            }
            if (FORM_KeyProc(ACT_MENU_NAV_DOWN_KKEY, ACT_MENU_NAV_DOWN_GBTN)) {
                FORM_Focus(form, 0, 1);
                WIDGET_snd(WS_MOVE);
            }
            if (FORM_KeyProc(ACT_MENU_PAGE_FIRST_KKEY)) {
                FORM_Focus(form, form->children, 0);
                WIDGET_snd(WS_MOVE);
            }
            if (FORM_KeyProc(ACT_MENU_PAGE_LAST_KKEY)) {
                FORM_Focus(form, form->children, -1);
                WIDGET_snd(WS_MOVE);
            }
            if (FORM_KeyProc(ACT_MENU_CONFIRM_KKEY_1, ACT_MENU_CONFIRM_GBTN))
                if (form->focus && form->focus->methods->Push)
                    form->focus->methods->Push(form->focus);
            if (FORM_KeyProc(ACT_MENU_FORM_CYCLE_FOCUS_KKEY) && form->focus) {
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
