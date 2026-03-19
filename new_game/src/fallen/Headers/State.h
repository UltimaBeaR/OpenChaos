// State.h
// Guy Simmons, 4th January 1998.

#ifndef FALLEN_HEADERS_STATE_H
#define FALLEN_HEADERS_STATE_H

struct Thing;
//---------------------------------------------------------------

typedef struct
{
    UBYTE State;
    void (*StateFn)(Thing*);
} StateFunction;

//---------------------------------------------------------------

typedef struct
{
    UBYTE Genus;
    StateFunction* StateFunctions;
} GenusFunctions;

//---------------------------------------------------------------

extern void set_state_function(Thing* t_thing, UBYTE state);
extern void set_generic_person_state_function(Thing* t_thing, UBYTE state);
extern void set_generic_person_just_function(Thing* t_thing, UBYTE state);

//---------------------------------------------------------------

#endif // FALLEN_HEADERS_STATE_H
