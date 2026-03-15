//
// Loads an environment from a file.
//

#ifndef _ENV_
#define _ENV_

//
// now we use standard Windows .INI files
//


void ENV_load(CBYTE* fname);


//
// retrieve values
//

CBYTE* ENV_get_value_string(CBYTE* name, CBYTE* section = "Game"); // returns NULL if not found - NOTE: string is in a static buffer
SLONG ENV_get_value_number(CBYTE* name, SLONG def, CBYTE* section = "Game"); // returns def if not found

//
// store values
//

void ENV_set_value_string(CBYTE* name, CBYTE* value, CBYTE* section = "Game");
void ENV_set_value_number(CBYTE* name, SLONG value, CBYTE* section = "Game");

#endif
