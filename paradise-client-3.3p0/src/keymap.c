/*
 * keymap.c
 * Bill Dyess, 10/20/93
 */

#include "copyright.h"

#include "config.h"
#include <stdio.h>
#include "str.h"

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"

#define control(x) (x)+128

int
doKeymap(W_Event *data)
{
    int     key = data->key;

    if (macroState) {		/* macro needed a destination or in macro
				   mode */
	if (key > 256 && data->type != W_EV_BUTTON) {	/* an alt-key will exit
							   the macro mode */
	    /*
	       but let mouse buttons do macros! button conversion is done by
	       calling function.
	    */
	    warning("             ");
	} else {
	    if (key > 256)
		data->key -= 256;
	    doMacro(data);
	    return -1;
	}
    }

    if (key < 256) {		/* not alt key */
	key = myship->s_keymap[key];

	if (macrotable[key] && (macrotable[key]->flags & MACSINGLE)) {
	    data->key = key;
	    doMacro(data);
	    return -1;
	}

    } else
	key = key - 256;
    return key;
}

unsigned char def_keymap[256];
unsigned char def_buttonmap[12];

void
buildShipKeymap(struct ship *shipp)
{
    char    keybuf[40], ckeybuf[40], buttonbuf[40], cbuttonbuf[40];
    struct stringlist *l;

    memcpy(shipp->s_keymap, def_keymap, 256);
    memcpy(shipp->s_buttonmap, def_buttonmap, 12);

    sprintf(keybuf, "keymap.%c%c", shipp->s_desig[0], shipp->s_desig[1]);
    sprintf(ckeybuf, "ckeymap.%c%c", shipp->s_desig[0], shipp->s_desig[1]);
    sprintf(buttonbuf, "buttonmap.%c%c", shipp->s_desig[0], shipp->s_desig[1]);
    sprintf(cbuttonbuf, "cbuttonmap.%c%c", shipp->s_desig[0], shipp->s_desig[1]);

    for (l = defaults; l; l = l->next) {
	if (!strcasecmp(keybuf, l->string))
	    keymapAdd(l->value, (char*)shipp->s_keymap);
	else if (!strcasecmp(ckeybuf, l->string))
	    ckeymapAdd(l->value, (char*)shipp->s_keymap);
	else if (!strcasecmp(buttonbuf, l->string))
	    buttonmapAdd(l->value, (char*)shipp->s_buttonmap);
	else if (!strcasecmp(cbuttonbuf, l->string))
	    cbuttonmapAdd(l->value, (char*)shipp->s_buttonmap);
    }
}

void
initkeymap(int type)
{
    int     i, j;
    struct stringlist *l;

    if (type < 0) {
	for (i = 0; i < 256; i++)
	    def_keymap[i] = i;

	for (l = defaults; l; l = l->next) {
	    if (!strcasecmp("keymap", l->string))
		keymapAdd(l->value, (char*)def_keymap);
	    else if (!strcasecmp("ckeymap", l->string))
		ckeymapAdd(l->value, (char*)def_keymap);
	    else if (!strcasecmp("buttonmap", l->string))
		buttonmapAdd(l->value, (char*)def_buttonmap);
	    else if (!strcasecmp("cbuttonmap", l->string))
		cbuttonmapAdd(l->value, (char*)def_buttonmap);
	}

	for (j = 0; j < nshiptypes; j++) {
	    buildShipKeymap(getship(j));
	}
    }
}

void
keymapAdd(char *str, char *kmap)
{
    if (str) {
	/* parse non-control char keymap */
	while (*str != '\0' && *(str + 1) != '\0') {
	    if (*str >= 32 && *str < 127) {
		kmap[(int) *str] = *(str + 1);
	    }
	    str += 2;
	}
    }
}

void
ckeymapAdd(char *cstr, char *kmap)
{
    unsigned char key[2];
    short   state = 0;

    if (cstr) {
	/*
	   control chars are allowed, so use ^char to mean control, and ^^ to
	   mean ^
	*/
	while (*cstr != '\0') {
	    if (*cstr == '^') {
		cstr++;
		if (*cstr == '^' || !*cstr)
		    key[state] = '^';
		else
		    key[state] = 128 + *cstr;
	    } else {
		key[state] = *cstr;
	    }
	    if (*cstr)
		cstr++;
	    if (state)
		kmap[key[0]] = key[1];
	    state = 1 - state;
	}
    }
}

void
buttonmapAdd(char *str, char *kmap)
{
    unsigned char button, ch;

    if (str) {
	while (*str != '\0' && *(str + 1) != '\0') {
	    if (*str < 'a')
		button = *str++ - '1';
	    else
		button = 9 + *str++ - 'a';
	    if (button > 11)
		fprintf(stderr, "%c ignored in buttonmap\n", *(str - 1));
	    else {
		ch = *str++;
		kmap[button] = ch;
	    }
	}
    }
}

void
cbuttonmapAdd(char *cstr, char *kmap)
{
    unsigned char button, ch;

    if (cstr) {
	while (*cstr != '\0' && *(cstr + 1) != '\0') {
	    /*
	       code for cbuttonmap, which allows buttons to be mapped to
	       control keys. [BDyess]
	    */
	    if (*cstr < 'a')
		button = *cstr++ - '1';
	    else
		button = 9 + *cstr++ - 'a';
	    if (button > 11)
		fprintf(stderr, "%c ignored in cbuttonmap\n", *(cstr - 1));
	    else {
		ch = *cstr++;
		if (ch == '^') {
		    ch = *cstr++;
		    if (ch != '^')
			ch += 128;
		}
		kmap[button] = ch;
	    }
	}
    }
}

#ifdef KEYMAP_DEBUG
void
dumpKeymap(void)
{
    int     i;

    for (i = 0; i < 256; i++) {
	printf("%3d %c : %3d %c\n",
	       i,
	       isprint(i) ? i : '_',
	       myship->s_keymap[i],
	       isprint(myship->s_keymap[i]) ? myship->s_keymap[i] : '_');
    }
}
#endif				/* KEYMAP_DEBUG */
