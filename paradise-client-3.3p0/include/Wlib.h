/* Wlib.h
 *
 * Include file for the Windowing interface.
 *
 * Kevin P. Smith  6/11/89
 *
 * The deal is this:
 *   Call W_Initialize(), and then you may call any of the listed fuinctions.
 *   Also, externals you are allowed to pass or use include W_BigFont,
 *     W_RegularFont, W_UnderlineFont, W_HighlightFont, W_White, W_Black,
 *     W_Red, W_Green, W_Yellow, W_Cyan, W_Grey, W_Textwidth, and W_Textheight.
 */

#ifndef WLIB_H
#define WLIB_H

#include <X11/Xlib.h>
#include "copyright2.h"
#include "config.h"

/* image struct to hold all info about an image, be it a bitmap or 
   pixmap.  Replaces W_Icon.  [BDyess] */
typedef struct {
  /* public */
  unsigned int width, height, frames;	/* frames = nviews for ships */
  int xpm;			/* is it a Pixmap (> 1 plane) or not */
  char *filename;		/* filename without .xpm/.xbm extension*/
  int loaded;			/* 1 if loaded (for on-demand loading) */
  int alternate;		/* offset into images array for alternate
                                   image to use if this one can't be
				   loaded */
  int bad;			/* if set, loading failed */
  int compiled_in;		/* image is compiled into the binary */
  unsigned char *xbmdata;	/* ptr to compiled in xbm data.  Only valid */
                                /* if compiled_in is true. */
  char **xpmdata;		/* ptr to compiled in xpm data.  Only valid */
                                /* if compiled_in is true. */
  /* X Data structures (private) */
  Pixmap pixmap, clipmask;	/* clipmask only used if in xpm mode */
} W_Image;

/*typedef char *W_Window;*/
typedef char *W_Icon;
typedef char *W_Font;
typedef int W_Color;

typedef int (*W_Callback) ();
typedef char *W_Window;

extern W_Font W_BigFont, W_RegularFont, W_UnderlineFont, W_HighlightFont;
extern W_Color W_White, W_Black, W_Red, W_Green, W_Yellow, W_Cyan, W_Grey;
extern int W_Textwidth, W_Textheight;
extern int W_FastClear;

#define W_EV_EXPOSE	1
#define W_EV_KEY	2
#define W_EV_BUTTON	3
#define W_EV_KILL_WINDOW 4

#define W_LBUTTON	1
#define W_MBUTTON	2
#define W_RBUTTON	3

typedef struct event {
    int     type;
    W_Window Window;
    int     key;
    int     x, y;
} W_Event;

#define W_BoldFont W_HighlightFont

#define W_StringWidth(st,font) (strlen(st)*W_Textwidth)

#endif
