#ifndef UI_MENUS_WIDGET_H
#define UI_MENUS_WIDGET_H

#include "engine/platform/uc_common.h"

// Widget state flags.

// uc_orig: WIDGET_STATE_FOCUS (fallen/Headers/widget.h)
#define WIDGET_STATE_FOCUS 1
// uc_orig: WIDGET_STATE_DISABLED (fallen/Headers/widget.h)
#define WIDGET_STATE_DISABLED 2
// uc_orig: WIDGET_STATE_ACTIVATED (fallen/Headers/widget.h)
#define WIDGET_STATE_ACTIVATED 4
// uc_orig: WIDGET_STATE_BLOCKFOCUS (fallen/Headers/widget.h)
#define WIDGET_STATE_BLOCKFOCUS 8
// uc_orig: WIDGET_STATE_ALIGNLEFT (fallen/Headers/widget.h)
#define WIDGET_STATE_ALIGNLEFT 16
// uc_orig: WIDGET_STATE_SHRINKTEXT (fallen/Headers/widget.h)
#define WIDGET_STATE_SHRINKTEXT 32

// Sound event codes passed to WIDGET_snd.
// uc_orig: WS_MOVE (fallen/Headers/widget.h)
#define WS_MOVE 1
// uc_orig: WS_FADEIN (fallen/Headers/widget.h)
#define WS_FADEIN 2
// uc_orig: WS_FADEOUT (fallen/Headers/widget.h)
#define WS_FADEOUT 3
// uc_orig: WS_OK (fallen/Headers/widget.h)
#define WS_OK 4
// uc_orig: WS_FAIL (fallen/Headers/widget.h)
#define WS_FAIL 5
// uc_orig: WS_BLIP (fallen/Headers/widget.h)
#define WS_BLIP 6

// Forward declarations.
// uc_orig: Widget (fallen/Headers/widget.h)
class Widget;
// uc_orig: Form (fallen/Headers/widget.h)
class Form;

// Callback typedefs for widget methods.
// uc_orig: WIDGET_Void (fallen/Headers/widget.h)
typedef void (*WIDGET_Void)(Widget* widget);
// uc_orig: WIDGET_Clik (fallen/Headers/widget.h)
typedef BOOL (*WIDGET_Clik)(Widget* widget, SLONG x, SLONG y);
// uc_orig: WIDGET_Char (fallen/Headers/widget.h)
typedef BOOL (*WIDGET_Char)(Widget* widget, CBYTE key);
// uc_orig: WIDGET_Data (fallen/Headers/widget.h)
typedef intptr_t (*WIDGET_Data)(Widget* widget, SLONG code, intptr_t data1, intptr_t data2);
// uc_orig: FORM_Proc (fallen/Headers/widget.h)
typedef BOOL (*FORM_Proc)(Form* form, Widget* widget, SLONG message);

// Virtual method table for a widget type.
// uc_orig: Methods (fallen/Headers/widget.h)
struct Methods {
    WIDGET_Void Init, Free, Draw, Push;
    WIDGET_Char Char;
    WIDGET_Data Data;
    WIDGET_Clik HitTest;
};

// 2D point used for widget layout calculations.
// uc_orig: WidgetPoint (fallen/Headers/widget.h)
struct WidgetPoint {
    SLONG x, y;
};

// A single UI control element in a doubly-linked list owned by a Form.
// uc_orig: Widget (fallen/Headers/widget.h)
class Widget {
public:
    SLONG x, y, ox, oy;   // parent-relative bounding box
    SLONG tag;             // opaque value for parent dialog proc
    SLONG state;           // WIDGET_STATE_* flags
    intptr_t data[5];      // widget-type-specific state (stores both ints and pointers)
    CBYTE* caption;        // text label
    Form* form;            // owning form
    Methods* methods;      // virtual method table
    Widget *prev, *next;   // doubly-linked list pointers
};

// A dialog container that owns a list of widgets and drives input routing.
// uc_orig: Form (fallen/Headers/widget.h)
class Form {
public:
    SLONG x, y, ox, oy;    // screen-space bounding box
    ULONG textcolour;       // RGBA text colour
    SLONG returncode;       // non-zero signals dialog completion
    SLONG age;              // ticks since creation (used for fade-in)
    SLONG inverse;          // invert rendering flag
    Widget* children;       // head of child widget list
    Widget* focus;          // currently focused widget
    FORM_Proc proc;         // dialog message handler
    CBYTE caption[32];      // form title string
};

// Form management.
// uc_orig: FORM_Create (fallen/Source/widget.cpp)
Form* FORM_Create(CBYTE* caption, FORM_Proc proc, SLONG x, SLONG y, SLONG ox, SLONG oy, ULONG textcolour);
// uc_orig: FORM_AddWidget (fallen/Source/widget.cpp)
Widget* FORM_AddWidget(Form* form, Widget* widget);
// uc_orig: FORM_DelWidget (fallen/Source/widget.cpp)
void FORM_DelWidget(Widget* widget);
// uc_orig: FORM_Process (fallen/Source/widget.cpp)
SLONG FORM_Process(Form* form);
// uc_orig: FORM_Draw (fallen/Source/widget.cpp)
void FORM_Draw(Form* form);
// uc_orig: FORM_Free (fallen/Source/widget.cpp)
void FORM_Free(Form* form);
// uc_orig: FORM_Focus (fallen/Source/widget.cpp)
Widget* FORM_Focus(Form* form, Widget* widget, SBYTE direction = 0);
// uc_orig: FORM_GetWidgetFromPoint (fallen/Source/widget.cpp)
Widget* FORM_GetWidgetFromPoint(Form* form, WidgetPoint pt);

// Widget factory and menu helper.
// uc_orig: WIDGET_menu (fallen/Source/widget.cpp)
void WIDGET_menu(Form* form, ...);
// uc_orig: WIDGET_Create (fallen/Source/widget.cpp)
Widget* WIDGET_Create(Methods* widget_class, SLONG x, SLONG y, SLONG ox, SLONG oy, CBYTE* caption);

// Plays a sound effect for a widget UI event.
// uc_orig: WIDGET_snd (fallen/Source/widget.cpp)
void WIDGET_snd(SLONG snd);

// Inline helpers for widget state manipulation.
// uc_orig: WIDGET_SetState (fallen/Headers/widget.h)
inline void WIDGET_SetState(Widget* widget, SLONG data, SLONG mask = 0)
{
    widget->state = (widget->state & ~mask) | data;
}
// uc_orig: WIDGET_ToggleState (fallen/Headers/widget.h)
inline void WIDGET_ToggleState(Widget* widget, SLONG data)
{
    widget->state ^= data;
}
// uc_orig: WIDGET_State (fallen/Headers/widget.h)
inline SLONG WIDGET_State(Widget* widget, SLONG mask = 0xFFFF)
{
    return (widget->state & mask);
}

// Inline coordinate utilities.
// uc_orig: TO_WIDGETPNT (fallen/Headers/widget.h)
inline WidgetPoint TO_WIDGETPNT(SLONG x, SLONG y)
{
    WidgetPoint pt = { x, y };
    return pt;
}
// uc_orig: FORM_To_Screen (fallen/Headers/widget.h)
inline WidgetPoint FORM_To_Screen(Form* form, WidgetPoint pt)
{
    return TO_WIDGETPNT(pt.x + form->x, pt.y + form->y);
}
// uc_orig: WIDGET_Centre (fallen/Headers/widget.h)
inline WidgetPoint WIDGET_Centre(Widget* widget)
{
    return FORM_To_Screen(widget->form, TO_WIDGETPNT((widget->x + widget->ox) >> 1, (widget->y + widget->oy) >> 1));
}

// Widget type method tables (defined in widget_globals.cpp).
// uc_orig: BUTTON_Methods (fallen/Source/widget.cpp)
extern Methods BUTTON_Methods;
// uc_orig: STATIC_Methods (fallen/Source/widget.cpp)
extern Methods STATIC_Methods;
// uc_orig: CHECK_Methods (fallen/Source/widget.cpp)
extern Methods CHECK_Methods;
// uc_orig: RADIO_Methods (fallen/Source/widget.cpp)
extern Methods RADIO_Methods;
// uc_orig: INPUT_Methods (fallen/Source/widget.cpp)
extern Methods INPUT_Methods;
// uc_orig: LISTS_Methods (fallen/Source/widget.cpp)
extern Methods LISTS_Methods;
// uc_orig: TEXTS_Methods (fallen/Source/widget.cpp)
extern Methods TEXTS_Methods;
// uc_orig: GLYPH_Methods (fallen/Source/widget.cpp)
extern Methods GLYPH_Methods;
// uc_orig: SHADE_Methods (fallen/Source/widget.cpp)
extern Methods SHADE_Methods;

// Widget method implementations — not declared in original widget.h, but
// public because they are referenced in the method tables (widget_globals.cpp).

// uc_orig: BUTTON_Free (fallen/Source/widget.cpp)
void BUTTON_Free(Widget* widget);
// uc_orig: BUTTON_Draw (fallen/Source/widget.cpp)
void BUTTON_Draw(Widget* widget);
// uc_orig: BUTTON_Push (fallen/Source/widget.cpp)
void BUTTON_Push(Widget* widget);
// uc_orig: BUTTON_HitTest (fallen/Source/widget.cpp)
BOOL BUTTON_HitTest(Widget* widget, SLONG x, SLONG y);
// uc_orig: STATIC_Init (fallen/Source/widget.cpp)
void STATIC_Init(Widget* widget);
// uc_orig: CHECK_Draw (fallen/Source/widget.cpp)
void CHECK_Draw(Widget* widget);
// uc_orig: CHECK_Push (fallen/Source/widget.cpp)
void CHECK_Push(Widget* widget);
// uc_orig: RADIO_Push (fallen/Source/widget.cpp)
void RADIO_Push(Widget* widget);
// uc_orig: INPUT_Init (fallen/Source/widget.cpp)
void INPUT_Init(Widget* widget);
// uc_orig: INPUT_Free (fallen/Source/widget.cpp)
void INPUT_Free(Widget* widget);
// uc_orig: INPUT_Draw (fallen/Source/widget.cpp)
void INPUT_Draw(Widget* widget);
// uc_orig: INPUT_Char (fallen/Source/widget.cpp)
BOOL INPUT_Char(Widget* widget, CBYTE key);
// uc_orig: INPUT_Data (fallen/Source/widget.cpp)
intptr_t INPUT_Data(Widget* widget, SLONG code, intptr_t data1, intptr_t data2);
// uc_orig: LISTS_Free (fallen/Source/widget.cpp)
void LISTS_Free(Widget* widget);
// uc_orig: LISTS_Draw (fallen/Source/widget.cpp)
void LISTS_Draw(Widget* widget);
// uc_orig: LISTS_Char (fallen/Source/widget.cpp)
BOOL LISTS_Char(Widget* widget, CBYTE key);
// uc_orig: LISTS_Data (fallen/Source/widget.cpp)
intptr_t LISTS_Data(Widget* widget, SLONG code, intptr_t data1, intptr_t data2);
// uc_orig: LISTS_Push (fallen/Source/widget.cpp)
void LISTS_Push(Widget* widget);
// uc_orig: TEXTS_Init (fallen/Source/widget.cpp)
void TEXTS_Init(Widget* widget);
// uc_orig: TEXTS_Free (fallen/Source/widget.cpp)
void TEXTS_Free(Widget* widget);
// uc_orig: TEXTS_Draw (fallen/Source/widget.cpp)
void TEXTS_Draw(Widget* widget);
// uc_orig: TEXTS_Char (fallen/Source/widget.cpp)
BOOL TEXTS_Char(Widget* widget, CBYTE key);
// uc_orig: TEXTS_Data (fallen/Source/widget.cpp)
intptr_t TEXTS_Data(Widget* widget, SLONG code, intptr_t data1, intptr_t data2);
// uc_orig: GLYPH_Draw (fallen/Source/widget.cpp)
void GLYPH_Draw(Widget* widget);
// uc_orig: SHADE_Draw (fallen/Source/widget.cpp)
void SHADE_Draw(Widget* widget);

// Widget notification / message codes sent to dialog procs.
// uc_orig: WBN_PUSH (fallen/Headers/widget.h)
#define WBN_PUSH 1
// uc_orig: WIN_ENTER (fallen/Headers/widget.h)
#define WIN_ENTER 1
// uc_orig: WLN_ENTER (fallen/Headers/widget.h)
#define WLN_ENTER 1
// uc_orig: WFN_CHAR (fallen/Headers/widget.h)
#define WFN_CHAR 1
// uc_orig: WFN_FOCUS (fallen/Headers/widget.h)
#define WFN_FOCUS 2
// uc_orig: WFN_PAINT (fallen/Headers/widget.h)
#define WFN_PAINT 3
// uc_orig: WLM_ADDSTRING (fallen/Headers/widget.h)
#define WLM_ADDSTRING 1
// uc_orig: WIM_SETSTRING (fallen/Headers/widget.h)
#define WIM_SETSTRING 1
// uc_orig: WIM_SETMODE (fallen/Headers/widget.h)
#define WIM_SETMODE 2
// uc_orig: WTM_ADDSTRING (fallen/Headers/widget.h)
#define WTM_ADDSTRING 1
// uc_orig: WTM_ADDBLOCK (fallen/Headers/widget.h)
#define WTM_ADDBLOCK 2

#endif // UI_MENUS_WIDGET_H
