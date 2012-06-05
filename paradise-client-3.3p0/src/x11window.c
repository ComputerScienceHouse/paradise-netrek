/* x11window.c
 *
 * Kevin P. Smith  6/11/89
 * Much modified by Jerry Frain and Joe Young
 * Many many modifications by Bill Dyess
 * Full-color rewriting done by Bill Dyess
 */

/* define ABORT_ON_ERROR to get a core when X errors occur.  X Buffering is
   also turned off (making display very slow) so the program will stop when
   an error occurs and not later. [BDyess] */
/* #define ABORT_ON_ERROR */

#include "config.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xmu/CurUtil.h>
#include <X11/cursorfont.h>
#include "str.h"

#ifdef HAVE_XPM_H
#include <xpm.h>
#else
#ifdef HAVE_X11_XPM_H
#include <X11/xpm.h>
#endif
#endif

#include "Wlib.h"
#include "defs.h"
#include "struct.h"
#include "data.h"
#include "proto.h"
#include "images.h"

#define INVALID_POSITION	-10000	/* gotta be a big negative */
/* XFIX speedup */
#define MAXCACHE	128

#define MAXPLANES	8
int	nplanes=3;
int	xpmORplanes = 8;
unsigned long	base;
unsigned long planes[MAXPLANES];
XColor	allocated_colors[1<<MAXPLANES];
int	nallocated_colors = -1;

/*
#define NORMAL_FONT	"-*-fixed-medium-r-*-*-10-*-*-*-*-*-*-*"
#define BOLD_FONT	"-*-fixed-medium-r-*-*-10-*-*-*-*-*-*-*"
#define ITALIC_FONT	"-*-fixed-medium-r-*-*-10-*-*-*-*-*-*-*"
*/

#define NORMAL_FONT	"6x10"
#define BOLD_FONT	"6x10"
#define BOLD_FONT2	"-*-clean-bold-r-normal--10-100-75-75-c-60-*"
#define ITALIC_FONT	"6x10"
#define ITALIC_FONT2	"-*-clean-bold-r-normal--10-100-75-75-c-60-*"
#define BIG_FONT	"-adobe-helvetica-bold-r-normal--34-*-*-*-*-*-*-*"
#define IND_FONT        "-*-clean-bold-r-normal--10-100-75-75-c-60-*"
/*#define BOLD_FONT2	"-schumacher-clean-bold-r-normal--10-100-75-75-c-60-iso8859-1"
#define ITALIC_FONT2	"-misc-fixed-medium-i-normal--10-100-75-75-c-60-iso8859-1"
#define BIG_FONT	"-adobe-helvetica-bold-r-normal--34-*-*-*-*-*-*-*"
*/

static char *_nfonts[] = {
    NORMAL_FONT,
    "-*-clean-medium-r-normal--10-100-75-75-c-60-*",
    "fixed",
    NULL,
};
static char *_bfonts[] = {
    BOLD_FONT,
    "-*-clean-bold-r-normal--10-100-75-75-c-60-*",
    "fixed",
    NULL,
};
static char *_ifonts[] = {
    ITALIC_FONT,
    "-*-clean-bold-r-normal--10-100-75-75-c-60-*",
    "fixed",
    NULL,
};
static char *_bgfonts[] = {
    BIG_FONT,
    "-*-lucidatypewriter-*-*-*-*-40-*-*-*-*-*-*-*",
    "fixed",
    NULL,
};

#define FONTS 4
#define BITGC 4
#define BITMASKGC 5
#define BITMASKNOCLIPGC 6
#define NONFONTS 3		/* 3 non-font GC's [BDyess] */

#define NONE		-1
#define WHITE   	0
#define BLACK   	1
#define RED     	2
#define GREEN   	3
#define YELLOW  	4
#define CYAN    	5
#define DARK_GREY	6
#define LIGHT_GREY 	7

static int zero = 0;
static int one = 1;
static int two = 2;
static int three = 3;

GC maskGC; 		/* JJJ */ /* for Cookie mode [BDyess] */
GC stippleGC;		/* GC for stippling [BDyess] */
GC borderGC; 		/* GC used for drawing the borders [BDyess] */

int     controlkey = 0;
#define	BillsScrewyAltMask	(Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask)
int     altkey = 0;
int     W_FastClear = 0;
int     buttonDown = 0;
Display *W_Display;
Window  W_Root;
Colormap W_Colormap;
int     W_Screen;
Visual *W_Visual;
W_Font  W_BigFont = (W_Font) & zero, W_RegularFont = (W_Font) & one;
W_Font  W_HighlightFont = (W_Font) & two, W_UnderlineFont = (W_Font) & three;
W_Color W_White = WHITE, W_Black = BLACK, W_Red = RED, W_Green = GREEN;
W_Color W_Yellow = YELLOW, W_Cyan = CYAN, W_Grey = LIGHT_GREY;
int     W_Textwidth, W_Textheight;

int     W_in_message = 0;	/* jfy -- for Jerry's warp message hack */

extern W_Window baseWin;
static XClassHint class_hint = {
    "netrek", "Netrek",
};

static XWMHints wm_hint = {
    InputHint | StateHint,
    True,
    NormalState,
    None,
    None,
    0, 0,
    None,
    None,
};

static XSizeHints wm_size_hint;

static W_Event W_myevent;
static int W_isEvent = 0;
static Atom wm_delete_window_atom;

struct fontInfo {
    XFontStruct *fontstruct;
    int     baseline;
};

struct colors {
    char   *name;
    GC      contexts[FONTS + NONFONTS];
    Pixmap  pixmap;
    unsigned long pixelValue;
};

#define WIN_GRAPH	1
#define WIN_TEXT	2
#define WIN_MENU	3
#define WIN_SCROLL	4
#define WIN_BORDER	5		/* border windows [BDyess] */
#define WIN_SCROLLBAR   6		/* scrollbar window [BDyess] */

struct window {
    Window  window;
    W_Window borderwin;
    int     border;		/* true if this is a border [BDyess] */
    W_Color border_color;
    Drawable drawable;
    Pixmap  buffer;
    int     isbuffered;
    int     type;
    char   *data;
    int     mapped;
    unsigned int width, height;
    char   *name;
    Cursor  cursor;
    int     insensitive;
    /* for scrollbars [BDyess] */
    W_Window scrollbarwin;
    int    isscrollbar;		/* true if this is a scrollbar window.
                                   Data is stored in the parent window[BDyess]*/
    int    top;			/* most recent addition to scrollback [BDyess]*/
    int    bottom;		/* last remembered scrollback line [BDyess]*/
    int    currenttop;		/* current bottom of window [BDyess] */
    struct _scrolldata {
      W_Color color;
      char   *string;
      int    len;
    } *scrolldata;
    int    maxlines;
};

struct stringList {
    char   *string;
    W_Color color;
    struct stringList *next;
};

struct menuItem {
    char   *string;
    W_Color color;
    W_Font  font;
    int     x;		/* number of characters offset from left side [BDyess]*/
};

struct colors colortable[] = {
    {"white"},
    {"black"},
    {"red"},
    {"green"},
    {"yellow"},
    {"cyan"},
    {"#767676"},	/* dark grey */
    {"#d8d8d8"}		/* light grey */
};

struct windowlist {
    struct window *window;
    struct windowlist *next;
};

#define HASHSIZE 29
#define hash(x) (((int) (x)) % HASHSIZE)

struct windowlist *hashtable[HASHSIZE];
struct fontInfo fonts[FONTS];

struct window myroot;

#define NCOLORS (sizeof(colortable)/sizeof(colortable[0]))
#define W_Void2Window(win) ((win) ? (struct window *) (win) : &myroot)
#define W_Window2Void(window) ((W_Window) (window))
#define fontNum(font) (*((int *) font))
#define TILESIDE 16

static unsigned char gray[] = {
    0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
    0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
    0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55,
    0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55
};

static unsigned char striped[] = {
    0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
    0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f,
    0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00,
    0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0
};

static unsigned char solid[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/* Prototypes */
#ifdef ABORT_ON_ERROR
static int _myerror P((Display *d , XErrorEvent *e ));
#endif
static void GetFonts P((void));
static XFontStruct *find_font P((char *oldf, char **fnts));
static void GetColors P((void));
static void FlushClearAreaCache P((W_Window window));
static int W_SpNextEvent P((W_Event * wevent));
static void FlushLineCache P((Window win, int color));
static void FlushPointCache P((Window win, int color));
static struct window *findWindow P((Window window));
static void addToHash P((struct window * win));
static void AddToScrolling P((struct window * win, W_Color color, char *str, int len));
static void redrawScrolling P((struct window * win));
static void resizeScrolling P((struct window * win, int width, int height));
static void redrawMenu P((struct window * win));
static void redrawMenuItem P((struct window * win, int n));
static void changeMenuItem P((struct window * win, int x, int n, W_Color color, char *str, int len, W_Font font));
/*static void W_SetTransientForHint P((W_Window w , W_Window pw ));*/
static void checkGeometry P((char *name, int *x, int *y, int *width, int *height));
static void checkParent P((char *name, W_Window * parent));
static void checkCursor P((char *name, char *cursname, Cursor * cursor));
static void findMouse P((int *x, int *y));
static void deleteWindow P((struct window * window));
void W_Flush P((void));
static void redrawBorder P((struct window *win));
static void redrawReversedBorder P((struct window *win));

#ifdef DEBUG
#define ABORT_ON_ERROR
#endif /*DEBUG*/

/* X debugging */
#ifdef ABORT_ON_ERROR
static int
_myerror(Display *d, XErrorEvent *e)
{
    abort();
    return 1;
}
#endif		/* ABORT_ON_ERROR */

void
W_Initialize(char *str)
{
    int     i;
    char    *s;

#ifdef DEBUG
    printf("Initializing...\n");
#endif
    for (i = 0; i < HASHSIZE; i++) {
	hashtable[i] = NULL;
    }
    if ((W_Display = XOpenDisplay(str)) == NULL) {
	fprintf(stderr, "Cannot open display \"%s\"\n", str ? str : "(null)");
	EXIT(1);
    }
#ifdef ABORT_ON_ERROR
    /* tmp */
    XSynchronize(W_Display, True);
    XSetErrorHandler(_myerror);
#endif

    W_Root = DefaultRootWindow(W_Display);
    W_Visual = DefaultVisual(W_Display, DefaultScreen(W_Display));
    W_Screen = DefaultScreen(W_Display);
    W_Colormap = DefaultColormap(W_Display, W_Screen);
    myroot.window = W_Root;
    myroot.type = WIN_GRAPH;
    s = strdup(stringDefault("shipBitmapPath", SHIP_BITMAP_PATH));
    s = expandFilename(s);
    imagedir = (char*)malloc(strlen(s) + 100);
    strcpy(imagedir,s);
    free(s);
    imagedirend = imagedir + strlen(imagedir);
    if(verbose_image_loading) printf("imagedir = %s\n",imagedir);
    GetFonts();
    GetColors();
    wm_delete_window_atom = XInternAtom(W_Display, "WM_DELETE_WINDOW", 1);
}

static void
GetFonts(void)
{
    Font    regular, italic, bold, big;
    int     i;
    XGCValues values;
    XFontStruct *fontinfo;
    char   *fontname;
    W_Image *image;

    fontname = stringDefault("font",NULL);
    if (fontname == NULL)
	fontname = NORMAL_FONT;
    fontinfo = XLoadQueryFont(W_Display, fontname);
    if (fontinfo == NULL) {
	fontinfo = find_font(fontname, _nfonts);
    }
    if (fontinfo == NULL) {
	printf("netrek: Can't find any fonts!\n");
	EXIT(1);
    }
    regular = fontinfo->fid;
    W_Textwidth = fontinfo->max_bounds.width;
    W_Textheight = fontinfo->max_bounds.descent + fontinfo->max_bounds.ascent;
    fonts[1].baseline = fontinfo->max_bounds.ascent;
    fonts[1].fontstruct = fontinfo;

    fontname = stringDefault("boldfont",NULL);
    if (fontname == NULL) {
	if (DisplayCells(W_Display, W_Screen) <= 4)
	    fontname = BOLD_FONT2;
	else
	    fontname = BOLD_FONT;
    }
    fontinfo = XLoadQueryFont(W_Display, fontname);
    if (fontinfo == NULL) {
	fontinfo = find_font(fontname, _bfonts);
    }
    if (fontinfo == NULL) {
	bold = regular;
	fonts[2].baseline = fonts[1].baseline;
	fonts[2].fontstruct = fonts[1].fontstruct;
    } else {
	bold = fontinfo->fid;
	fonts[2].baseline = fontinfo->max_bounds.ascent;
	fonts[2].fontstruct = fontinfo;
	if (fontinfo->max_bounds.width > W_Textwidth)
	    W_Textwidth = fontinfo->max_bounds.width;
	if (fontinfo->max_bounds.descent + fontinfo->max_bounds.ascent > W_Textheight)
	    W_Textheight = fontinfo->max_bounds.descent + fontinfo->max_bounds.ascent;
    }

    fontname = stringDefault("italicfont",NULL);
    if (fontname == NULL) {
	if (DisplayCells(W_Display, W_Screen) <= 4)
	    fontname = ITALIC_FONT2;
	else
	    fontname = ITALIC_FONT;
    }
    fontinfo = XLoadQueryFont(W_Display, fontname);
    if (fontinfo == NULL) {
	fontinfo = find_font(fontname, _ifonts);
    }
    if (fontinfo == NULL) {
	italic = regular;
	fonts[3].baseline = fonts[1].baseline;
	fonts[3].fontstruct = fonts[1].fontstruct;
    } else {
	italic = fontinfo->fid;
	fonts[3].baseline = fontinfo->max_bounds.ascent;
	fonts[3].fontstruct = fontinfo;
	if (fontinfo->max_bounds.width > W_Textwidth)
	    W_Textwidth = fontinfo->max_bounds.width;
	if (fontinfo->max_bounds.descent + fontinfo->max_bounds.ascent > W_Textheight)
	    W_Textheight = fontinfo->max_bounds.descent + fontinfo->max_bounds.ascent;
    }

    fontname = stringDefault("bigfont",NULL);
    if (fontname == NULL)
	fontname = BIG_FONT;
    fontinfo = XLoadQueryFont(W_Display, fontname);
    if (fontinfo == NULL) {
	fontinfo = find_font(fontname, _bgfonts);
    }
    if (fontinfo == NULL) {
	big = regular;
	fonts[0].baseline = fonts[1].baseline;
	fonts[0].fontstruct = fonts[1].fontstruct;
    } else {
	big = fontinfo->fid;
	fonts[0].baseline = fontinfo->max_bounds.ascent;
	fonts[0].fontstruct = fontinfo;
    }
    for (i = 0; i < NCOLORS; i++) {
	values.font = big;
	colortable[i].contexts[0] = XCreateGC(W_Display, W_Root, GCFont, &values);
	XSetGraphicsExposures(W_Display, colortable[i].contexts[0], False);
	values.font = regular;
	colortable[i].contexts[1] = XCreateGC(W_Display, W_Root, GCFont, &values);
	XSetGraphicsExposures(W_Display, colortable[i].contexts[1], False);
	values.font = bold;
	colortable[i].contexts[2] = XCreateGC(W_Display, W_Root, GCFont, &values);
	XSetGraphicsExposures(W_Display, colortable[i].contexts[2], False);
	values.font = italic;
	colortable[i].contexts[3] = XCreateGC(W_Display, W_Root, GCFont, &values);
	XSetGraphicsExposures(W_Display, colortable[i].contexts[3], False);
	{
	    static char dl[] = {1, 4};
	    XSetLineAttributes(W_Display, colortable[i].contexts[3],
			       0, LineOnOffDash, CapButt, JoinMiter);
	    XSetDashes(W_Display, colortable[i].contexts[3], 0, dl, 2);
	}
	values.function = GXor;
	colortable[i].contexts[BITGC] = XCreateGC(W_Display, W_Root, GCFunction,
	                                          &values);
	XSetGraphicsExposures(W_Display, colortable[i].contexts[BITGC], False);
	colortable[i].contexts[BITMASKGC] = XCreateGC(W_Display, W_Root, 0, 
	                                              &values);
	XSetGraphicsExposures(W_Display, colortable[i].contexts[BITMASKGC], 
			      False);
	colortable[i].contexts[BITMASKNOCLIPGC] = XCreateGC(W_Display, 
	                                                    W_Root, 0, &values);
	XSetGraphicsExposures(W_Display, 
	                      colortable[i].contexts[BITMASKNOCLIPGC], False);
    }
    values.fill_style = FillSolid;
    values.line_style = LineSolid;
    borderGC = XCreateGC(W_Display, W_Root, GCFillStyle|GCLineStyle, &values);

    /* stipple for scrollbars [BDyess] */
    image = getImage(I_STIPPLE);
    values.stipple = image->pixmap;
    values.fill_style = FillStippled;
    stippleGC = XCreateGC(W_Display, W_Root, GCStipple|GCFillStyle, &values);

    values.function = GXand;
    maskGC = XCreateGC(W_Display, W_Root,
		GCFunction , &values);
}

static XFontStruct *
find_font(char *oldf, char **fnts)
{
    XFontStruct *fi;
    char  **f;
    fprintf(stderr, "netrek: Can't find font %s.  Trying others...\n",
	    oldf);
    for (f = fnts; *f; f++) {
	if (strcmp(*f, oldf) != 0) {
	    if ((fi = XLoadQueryFont(W_Display, *f)))
		return fi;
	}
    }
    printf("Error - can't find any font!\n");
    return NULL;
}

static unsigned short extrared[8] = {0x00, 0x20, 0x40, 0x60, 0x80, 0xa0, 0xb0, 0xc0};
static unsigned short extragreen[8] = {0x40, 0x60, 0x80, 0xa0, 0xb0, 0xc0, 0x00, 0x20};
static unsigned short extrablue[8] = {0x80, 0xa0, 0xb0, 0xc0, 0x00, 0x20, 0x40, 0x60};

int
W_Mono(void)
{
    return forceMono;
}

static void
GetColors(void)
{
    int     i, j;
    XColor  foo;
    int     white, black;
/*    unsigned long pixel;
    unsigned long planes[3];*/
    char    defaultstring[100];
    char   *defs;
    unsigned long extracolors[8];
    XColor  colordef;
    extern int forceMono;
    forceMono = booleanDefault("forcemono", forceMono);

    if ((DisplayCells(W_Display, W_Screen) <= 4) || forceMono) {
	forceMono = 1;
	white = WhitePixel(W_Display, W_Screen);
	black = BlackPixel(W_Display, W_Screen);
	for (i = 0; i < NCOLORS; i++) {
	    if (i != W_Black) {
		colortable[i].pixelValue = white;
	    } else {
		colortable[i].pixelValue = black;
	    }
	    if (i == W_Red) {
		colortable[i].pixmap = XCreatePixmapFromBitmapData
		    (W_Display,
		     W_Root, (char *) striped, TILESIDE, TILESIDE,
		     (unsigned)white, (unsigned)black,
		     (unsigned)DefaultDepth(W_Display, W_Screen));
	    } else if (i == W_Yellow) {
		colortable[i].pixmap = XCreatePixmapFromBitmapData
		    (W_Display,
		     W_Root, (char *) gray, TILESIDE, TILESIDE,
		     (unsigned)white, (unsigned)black,
		     (unsigned)DefaultDepth(W_Display, W_Screen));
	    } else {
		colortable[i].pixmap = XCreatePixmapFromBitmapData
		    (W_Display,
		     W_Root, (char *) solid, TILESIDE, TILESIDE,
		     (unsigned)colortable[i].pixelValue,
		     (unsigned)colortable[i].pixelValue,
		     (unsigned)DefaultDepth(W_Display, W_Screen));
	    }

	    /*
	       We assume white is 0 or 1, and black is 0 or 1. We adjust
	       graphics function based upon who is who.
	    */
	    if (white == 0) {	/* Black is 1 */
		XSetFunction(W_Display, colortable[i].contexts[BITGC], GXand);
	    }
	}
    } else if (DefaultVisual(W_Display, W_Screen)->class == TrueColor) {
/* Stuff added by sheldon@iastate.edu 5/28/93
 * This is supposed to detect a TrueColor display, and then do a lookup of
 * the colors in default colormap, instead of creating new colormap
 */
	for (i = 0; i < NCOLORS; i++) {
	    sprintf(defaultstring, "color.%s", colortable[i].name);

	    defs = stringDefault(defaultstring,NULL);
	    if (defs == NULL)
		defs = colortable[i].name;
	    XParseColor(W_Display, W_Colormap, defs, &foo);
	    XAllocColor(W_Display, W_Colormap, &foo);
	    colortable[i].pixelValue = foo.pixel;
	    colortable[i].pixmap = XCreatePixmapFromBitmapData(W_Display,
	    W_Root, (char *) solid, TILESIDE, TILESIDE, (unsigned)foo.pixel, 
	    (unsigned)foo.pixel, (unsigned)DefaultDepth(W_Display, W_Screen));
	}
    } else {
	xpmORplanes = DefaultDepth(W_Display,W_Screen);
        nplanes = (xpm && useOR) ? xpmORplanes : 3;
	if (!XAllocColorCells(W_Display, W_Colormap, False, planes, nplanes,
			      &base, 1)) {
	    /* couldn't allocate (nplanes) planes, make a new colormap */
	    printf("Couldn't allocate %d planes for private use; allocating a private colormap.\n",nplanes);
	    W_Colormap = XCreateColormap(W_Display, W_Root, W_Visual, AllocNone);
	    if (!XAllocColorCells(W_Display, W_Colormap, False, planes, 3/*nplanes*/,
				  &base, 1)) {
		fprintf(stderr, "Cannot create new colormap\n");
		EXIT(1);
	    }
	    /*
	       and fill it with at least 8 more colors so when mouse is
	       inside netrek windows, use might be able to see his other
	       windows
	    */
	    if (XAllocColorCells(W_Display, W_Colormap, False, NULL, 0,
				 extracolors, 8)) {
		colordef.flags = DoRed | DoGreen | DoBlue;
		for (i = 0; i < 8; i++) {
		    colordef.pixel = extracolors[i];
		    colordef.red = extrared[i] << 8;
		    colordef.green = extragreen[i] << 8;
		    colordef.blue = extrablue[i] << 8;
		    XStoreColor(W_Display, W_Colormap, &colordef);
		}
	    }
	}
	nallocated_colors = NCOLORS + 1;
	for (i = 0; i < NCOLORS; i++) {
	    /*
	       strcpy(defaultstring, "color.%s", colortable[i].name);
	    */
	    sprintf(defaultstring, "color.%s", colortable[i].name);

	    defs = stringDefault(defaultstring,colortable[i].name);
	    XParseColor(W_Display, W_Colormap, defs, &foo);
	    /*
	       Black must be the color with all the planes off. That is the
	       only restriction I concerned myself with in the following case
	       statement.
	    */
	    switch (i) {
	    case WHITE:
		foo.pixel = base | planes[0] | planes[1] | planes[2];
		memcpy(&allocated_colors[7], &foo, sizeof(foo));
		break;
	    case BLACK:
		foo.pixel = base;
		memcpy(&allocated_colors[0], &foo, sizeof(foo));
		break;
	    case RED:
		foo.pixel = base | planes[0];
		memcpy(&allocated_colors[1], &foo, sizeof(foo));
		break;
	    case CYAN:
		foo.pixel = base |             planes[1];
		memcpy(&allocated_colors[2], &foo, sizeof(foo));
		break;
	    case YELLOW:
		foo.pixel = base |                         planes[2];
		memcpy(&allocated_colors[4], &foo, sizeof(foo));
		break;
	    case DARK_GREY:
		foo.pixel = base | planes[0] | planes[1];
		memcpy(&allocated_colors[3], &foo, sizeof(foo));
		break;
	    case GREEN:
		foo.pixel = base |             planes[1] | planes[2];
		memcpy(&allocated_colors[6], &foo, sizeof(foo));
		break;
	    case LIGHT_GREY:
	        foo.pixel = base | planes[0] |             planes[2];
		memcpy(&allocated_colors[5], &foo, sizeof(foo));
                break;
	    }
	    XStoreColor(W_Display, W_Colormap, &foo);
	    colortable[i].pixelValue = foo.pixel;
	    colortable[i].pixmap = XCreatePixmapFromBitmapData(W_Display,
	    W_Root, (char *) solid, TILESIDE, TILESIDE, foo.pixel, foo.pixel,
		(unsigned) DefaultDepth(W_Display, W_Screen));
	}
    }
    for (i = 0; i < NCOLORS; i++) {
	for (j = 0; j < FONTS + NONFONTS; j++) {
	    XSetForeground(W_Display, colortable[i].contexts[j],
			   colortable[i].pixelValue);
	    XSetBackground(W_Display, colortable[i].contexts[j],
			   colortable[W_Black].pixelValue);
	}
    }
    /* set FG/BG for border GC [BDyess] */
    XSetBackground(W_Display, borderGC, colortable[DARK_GREY].pixelValue);
    XSetForeground(W_Display, borderGC, colortable[LIGHT_GREY].pixelValue);

    XSetBackground(W_Display, maskGC, 0xff); /* necessary? */
    XSetForeground(W_Display, maskGC, 0);
}
  
/* clears the buffer [BDyess] */
void
W_ClearBuffer(W_Window window)
{
  struct window * win = W_Void2Window(window);

  if(!win->isbuffered) return;
  XFillRectangle(W_Display, win->drawable, colortable[backColor].contexts[0],
		 0, 0, win->width, win->height);
}

int
W_IsBuffered(W_Window window)
{
  return W_Void2Window(window)->isbuffered;
}

/* turns on buffering, reduces flicker [BDyess] */
/* on is a flag: 1 turns on buffering, 0 turns it off */
void
W_Buffer(W_Window window, int on)
{
  struct window * win = W_Void2Window(window);

  if(on) {	/* turn buffering on [BDyess] */
    win->isbuffered = 1;
    if(win->buffer == 0) {  /* create a new pixmap for the buffer [BDyess] */
      win->buffer = XCreatePixmap(W_Display, W_Root, win->width,
				win->height, 
				(unsigned)DefaultDepth(W_Display, W_Screen));
    }
    win->drawable = win->buffer;
    /* clear the buffer to start with (can contain garbage) [BDyess] */
    W_ClearBuffer(window);
  } else { /* turn it off [BDyess] */
    win->drawable = win->window;
    win->isbuffered = 0;
  }
}

/* draws the buffer onto the screen [BDyess] */
void
W_DisplayBuffer(W_Window window)
{
  struct window * win = W_Void2Window(window);

  if(!win->isbuffered) return;
  XCopyArea(W_Display, win->buffer, win->window, 
            colortable[W_Black].contexts[0], 0, 0, win->width, 
	    win->height, 0, 0);
}

void
W_RenameWindow(W_Window window, char *str)
{
    XStoreName(W_Display, ((struct window *) window)->window, str);
}

static  W_Window
w_MakeWindow(char *name, int x, int y, int width, int height, 
             W_Window parent, char *cursname, int border, W_Color color, 
             int wsort)
{
    int     gx, gy;
    struct window *neww;
    Window  wparent;
    W_Window borderwin = 0;
    Cursor  cursor;
    XSetWindowAttributes attrs;
    unsigned int pwidth, pheight;	/* pixel width and height */

    if(wsort != WIN_BORDER && wsort != WIN_SCROLLBAR) {
      checkGeometry(name, &gx, &gy, &width, &height);
      if (gx != INVALID_POSITION)
	  x = gx;
      if (gy != INVALID_POSITION)
	  y = gy;

      checkParent(name, &parent);

      if (wsort == WIN_TEXT || wsort == WIN_SCROLL || wsort == WIN_MENU) {
	  pwidth = width * W_Textwidth + WIN_EDGE * 2;
	  if (wsort == WIN_MENU)
	      pheight = height * (W_Textheight + MENU_PAD * 2 + MENU_BAR) - MENU_BAR;
	  else
	      pheight = height * W_Textheight + MENU_PAD * 2;
      } else {
	  pwidth = width;
	  pheight = height;
      }

      /* if this needs a border, create one with another call to this 
	 function [BDyess] */
      if (border) {
	parent = w_MakeWindow(name, x, y, pwidth+border*2, pheight+border*2, 
			      parent, cursname, border, color, WIN_BORDER);
	borderwin = parent;
	/* update the parameters to reflect the size of the surrounding burder
	   [BDyess] */
	x = border;
	y = border;
      }
      attrs.background_pixel = colortable[W_Black].pixelValue;
    } else if(wsort == WIN_SCROLLBAR) {
      pwidth = width;
      pheight = height;
      attrs.background_pixel = colortable[W_Black].pixelValue;
    } else { /* it is a border, set the background [BDyess] */
      pwidth = width;
      pheight = height;
      attrs.background_pixel = colortable[DARK_GREY].pixelValue;
    }

    wparent = W_Void2Window(parent)->window;

    checkCursor(name, cursname, &cursor);
    attrs.cursor = cursor;

    switch (wsort) {
    case WIN_TEXT:
    case WIN_MENU:
	attrs.event_mask = KeyPressMask | ButtonPressMask | ExposureMask | ButtonReleaseMask;
	attrs.do_not_propagate_mask = ExposureMask | KeyPressMask | ButtonPressMask;
	break;
    case WIN_GRAPH:
	attrs.event_mask = KeyPressMask | ButtonPressMask | ExposureMask | LeaveWindowMask | ButtonReleaseMask | ButtonMotionMask;
	attrs.do_not_propagate_mask = ExposureMask;
	break;
    case WIN_SCROLL:
	attrs.event_mask = ResizeRedirectMask | ExposureMask | KeyPressMask | ButtonReleaseMask | ButtonPressMask;
	attrs.do_not_propagate_mask = ResizeRedirectMask | ExposureMask;
	break;
    case WIN_SCROLLBAR:
        attrs.event_mask = ExposureMask;
	attrs.do_not_propagate_mask = ExposureMask;
	break;
    case WIN_BORDER:
        attrs.event_mask = ExposureMask;
	attrs.do_not_propagate_mask = ExposureMask;
	break;
    default:
	fprintf(stderr, "x11window.c: w_MakeWindow: unknown wsort %d\n", wsort);
    }

    if (strcmp(name, "netrek_icon") == 0)	/* icon should not select for
						   input */
	attrs.event_mask = ExposureMask;
    if (strcmp(name, "wait_icon") == 0)	/* same here [BDyess] */
	attrs.event_mask = ExposureMask;

    if (strcmp(name, "info") == 0)	/* make info window passthru [BDyess] */
	attrs.event_mask = ExposureMask;

    neww = newWindow
	(XCreateWindow(W_Display, wparent, x, y, pwidth, pheight, (unsigned) 0,
		       CopyFromParent, InputOutput, CopyFromParent,
		       (unsigned)(CWBackPixel | CWEventMask |
		       (cursor ? CWCursor : 0)),
		       &attrs),
	 wsort);

    neww->cursor = cursor;
    /* keep track of each windows border so they can be mapped and unmapped
       together. [BDyess] */
    neww->borderwin = borderwin;
    neww->border = border;
    neww->border_color = NONE;

    {
	char   *s;

	/* let the local and map window sizes be dynamic [BDyess] */
	if(0 == strcmp("local",name)) {
	  winside = pwidth;
	} else if (0 == strcmp("map",name)) {
	  mapside = pwidth;
	}
	if (0 == strcmp(name, "wait"))
	    s = serverName;
	else if (0 == strcmp(name, "Motd"))
	    s = "Motd - [f] forward, [b] back, [tab] sysdefs, [space] unmap";
	else if (0 == strcmp(name, "netrek")) {
	    if (!title) {
		char    buf[80];
		sprintf(buf, "Netrek  @  %s", serverName);
		s = buf;
	    } else {
		/* but title on command line will override */
		/* from -h arg */
		s = title;
	    }
	    /* netrek window now acts as a border, so needs a dark grey
	       background [BDyess] */
	    XSetWindowBackground(W_Display, neww->window, 
	                         colortable[DARK_GREY].pixelValue);
	} else
	    s = name;

	XStoreName(W_Display, neww->window, s);
    }

    wm_size_hint.width = wm_size_hint.min_width =
	wm_size_hint.max_width = wm_size_hint.base_width = pwidth;
    wm_size_hint.min_height = wm_size_hint.height =
	wm_size_hint.max_height = wm_size_hint.base_height = pheight;
    wm_size_hint.flags = USSize | PMinSize | PMaxSize | PBaseSize;
    if (gx > INVALID_POSITION || gy > INVALID_POSITION) {
	wm_size_hint.flags |= USPosition;
	wm_size_hint.x = x;
	wm_size_hint.y = y;
    }
    XSetWMNormalHints(W_Display, neww->window, &wm_size_hint);

    class_hint.res_name = name;
    class_hint.res_class = "Netrek";
    XSetClassHint(W_Display, neww->window, &class_hint);

    XSetWMHints(W_Display, neww->window, &wm_hint);

    if (wparent == W_Root) {
      if (((baseWin != NULL &&
	    strcmp(name, "wait") != 0)
	   || wsort == WIN_MENU) &&
	  strcmp(name, "MetaServer List") != 0 &&
	  strcmp(name, "Motd") != 0) {
	  XSetTransientForHint(W_Display, neww->window,
			       W_Void2Window(baseWin)->window);
      }
      /* allow the WM_DELETE_WINDOW protocol to work [BDyess] */
      if(wm_delete_window_atom != None) {
        if(0 == XSetWMProtocols(W_Display, neww->window, &wm_delete_window_atom,
	                        1)){
	  printf("Odd, XSetWMProtocols failed for window \"%s\".  Your window manager will be unable to delete this window.\n",name);
	}
      }
    }
    neww->name = strdup(name);
    neww->width = width;
    neww->height = height;
    switch (wsort) {
      case WIN_MENU: 
        {
	  int     i;
	  struct menuItem *items;
	  items = (struct menuItem *) malloc(height * sizeof(struct menuItem));
	  for (i = 0; i < height; i++) {
	      items[i].string = NULL;
	      items[i].color = W_White;
	      items[i].font = W_RegularFont;
	      items[i].x = 0;
	  }
	  neww->data = (char *) items;
	}
        break;
      case WIN_SCROLL:		/* create a scrollbar [BDyess] */
        neww->data = 0;
	neww->scrollbarwin = w_MakeWindow(name, pwidth-W_Textwidth, 0, 
	                                  W_Textwidth, pheight,
                                          W_Window2Void(neww),
				          cursname, 0, color, WIN_SCROLLBAR);
	neww->top = neww->bottom = neww->currenttop = 0;
	neww->maxlines = checkScrollSaveLines(name);
	if(neww->maxlines < neww->height) neww->maxlines = neww->height;
	neww->scrolldata = (struct _scrolldata *)
	                   malloc(sizeof(struct _scrolldata) * neww->maxlines);
	if(neww->scrolldata == 0) {
	  perror("Error allocating scrollback storage");
	  exit(errno);
	}
	break;
      default:
	neww->data = 0;
	break;
    }

    if (wparent != W_Root && wsort != WIN_BORDER)
	if (checkMapped(name))
	    W_MapWindow(W_Window2Void(neww));

    /* turn on buffering if name.buffered: on [BDyess] */
    if(wsort != WIN_BORDER && wsort != WIN_SCROLLBAR) {
      if(checkBuffered(name)) {
	W_Buffer(W_Window2Void(neww), 1);
      }
    }

#ifdef DEBUG
    printf("New graphics window %d, child of %d\n", neww, parent);
#endif

    XSetWindowColormap(W_Display, neww->window, W_Colormap);

    return (W_Window2Void(neww));
}

W_Window
W_MakeWindow(char *name, int x, int y, int width, int height, 
             W_Window parent, char *cursname, int border, W_Color color)
{
    return w_MakeWindow(name, x, y, width, height, parent,
			cursname, border, color, WIN_GRAPH);
}

void
W_ChangeBorder(W_Window window, int color)
{
#ifdef DEBUG
    printf("Changing border of %d\n", window);
#endif

    struct window *border = W_Void2Window(W_Void2Window(window)->borderwin);

    border->border_color = color;
    redrawBorder(border);
}

void
W_MapWindow(W_Window window)
{
    struct window *win;

#ifdef DEBUG
    printf("Mapping %d\n", window);
#endif
    win = W_Void2Window(window);
    if (win->mapped)
	return;
    if(win->type != WIN_BORDER && win->borderwin) {
      W_MapWindow(win->borderwin);
    }
    win->mapped = 1;
    XMapRaised(W_Display, win->window);
    if(win->type == WIN_SCROLL && win->scrollbarwin) {
      W_MapWindow(win->scrollbarwin);
    }
}

void
W_UnmapWindow(W_Window window)
{
    struct window *win;

#ifdef DEBUG
    printf("UnMapping %d\n", window);
#endif
    win = W_Void2Window(window);
    if (win->mapped == 0)
	return;
    win->mapped = 0;
    XUnmapWindow(W_Display, win->window);
    if(win->type != WIN_BORDER && win->borderwin) {
      W_UnmapWindow(win->borderwin);
    }
}

int
W_IsMapped(W_Window window)
{
    struct window *win;

    win = W_Void2Window(window);
    if (win == NULL)
	return (0);
    return (win->mapped);
}

void
W_FillArea(W_Window window, int x, int y, 
           unsigned int width, unsigned int height, W_Color color)
{
    struct window *win;

#ifdef DEBUG
    printf("Clearing (%d %d) x (%d %d) with %d on %d\n", x, y, width, height,
	   color, window);
#endif
    win = W_Void2Window(window);
    switch (win->type) {
    case WIN_GRAPH:
	XFillRectangle(W_Display, win->drawable, colortable[color].contexts[0],
		       x, y, width, height);
	break;
    default:
	XFillRectangle(W_Display, win->drawable, colortable[color].contexts[0],
		    WIN_EDGE + x * W_Textwidth, MENU_PAD + y * W_Textheight,
		       width * W_Textwidth, height * W_Textheight);
    }
}

/* XFIX */

static XRectangle _rcache[MAXCACHE];
static int _rcache_index;

static void
FlushClearAreaCache(W_Window window)
{
    struct window *win = W_Void2Window(window);
    XFillRectangles(W_Display, win->drawable, colortable[backColor].contexts[0],
		    _rcache, _rcache_index);
    _rcache_index = 0;
}

/* local window only */
void
W_CacheClearArea(W_Window window, int x, int y, int width, int height)
{
    register XRectangle *r;

    if (_rcache_index == MAXCACHE)
	FlushClearAreaCache(window);

    r = &_rcache[_rcache_index++];
    r->x = (short) x;
    r->y = (short) y;
    r->width = (unsigned short) width;
    r->height = (unsigned short) height;
}

void
W_FlushClearAreaCache(W_Window window)
{
    if (_rcache_index)
	FlushClearAreaCache(window);
}

/* XFIX: clears now instead of filling. */
/* not any more.  Can't clear a drawable. [BDyess] */
void
W_ClearArea(W_Window window, int x, int y,
            unsigned int width, unsigned int height)
{
    struct window *win;

#ifdef DEBUG
    printf("Clearing (%d %d) x (%d %d) with %d on %d\n", x, y, width, height,
	   color, window);
#endif
    win = W_Void2Window(window);
    switch (win->type) {
    case WIN_GRAPH:
	/* XFIX: changed */
	XFillRectangle(W_Display, win->drawable, 
	    colortable[backColor].contexts[0], x, y, width, height);
	break;
    default:
	/* XFIX: changed */
	XFillRectangle(W_Display, win->drawable, 
	   colortable[backColor].contexts[0], WIN_EDGE + x * W_Textwidth,
	   MENU_PAD + y * W_Textheight, width * W_Textwidth, 
	   W_Textheight * height);
	break;
    }
}

void
W_ClearWindow(W_Window window)
{
    struct window * win = W_Void2Window(window);

#ifdef DEBUG
    printf("Clearing %d\n", window);
#endif
    if(win->isbuffered) {
      W_ClearBuffer(window);
    } else {
      XClearWindow(W_Display, win->window);
    }
}

void
W_GetEvent(W_Event *wevent)
{
/* blocks until an event is received [BDyess] */
    XEvent  event;

    if (W_isEvent) {
	*wevent = W_myevent;
	W_isEvent = 0;
	return;
    }
    XNextEvent(W_Display, &event);
    XPutBackEvent(W_Display, &event);
    W_SpNextEvent(wevent);
}

int
W_EventsPending(void)
{
    if (W_isEvent)
	return (1);
    while (XPending(W_Display) || buttonDown) {
	if (W_SpNextEvent(&W_myevent)) {
	    W_isEvent = 1;
	    return (1);
	}
    }
    return (0);
}

void
W_NextEvent(W_Event *wevent)
{
    if (W_isEvent) {
	*wevent = W_myevent;
	W_isEvent = 0;
    } else {
      while (W_SpNextEvent(wevent) == 0);
    }
}

static int
W_SpNextEvent(W_Event *wevent)
{
    XEvent  event;
    XKeyEvent *key;
    XButtonEvent *button;
    XExposeEvent *expose;
    XResizeRequestEvent *resize;
    char    ch;
    struct window *win;
    static W_Event buttonEvent;
    static int delaytime, cupd = -1;

#ifdef DEBUG
    printf("Getting an event...\n");
#endif
    key = (XKeyEvent *) & event;
    button = (XButtonEvent *) & event;
    expose = (XExposeEvent *) & event;
    resize = (XResizeRequestEvent *) & event;
    for (;;) {
	if (XPending(W_Display))
	    XNextEvent(W_Display, &event);
	else if (buttonDown) {
	    if (continuousMouse && allowContinuousMouse) {
		if (cupd != udcounter) {
		    cupd = udcounter;
		    if (delaytime == 0) {
			bcopy(&buttonEvent, wevent, sizeof(W_Event));
			delaytime = clickDelay;
		    } else {
			delaytime--;
			wevent->type = -1;
		    }
		} else
		    wevent->type = -1;
		exitInputLoop = 1;
	    } else {
		wevent->type = -1;
		buttonDown = 0;
	    }
	    return (1);
	}
	else
	    return (0);
	/*
	   printf("read an event %d\n", event.type);
	*/
	win = findWindow(key->window);
	if (win == NULL)
	    return (0);
	if ((event.type == KeyPress || event.type == ButtonPress) &&
	    win->type == WIN_MENU) {
	    if (key->y % (W_Textheight + MENU_PAD * 2 + MENU_BAR) >=
		W_Textheight + MENU_PAD * 2)
		return (0);
	    key->y = key->y / (W_Textheight + MENU_PAD * 2 + MENU_BAR);
	}
	switch ((int) event.type) {
	case LeaveNotify:	/* for message window -- jfy */
	    if (win == (struct window *) messagew) {
		W_in_message = 0;
	    }
	    return (0);
	case KeyPress:
	    if (key->state & ControlMask) {
		controlkey = 1;
		key->state &= ~ControlMask;
	    } else
		controlkey = 0;
	    if (key->state & BillsScrewyAltMask) {
		altkey = 1;
		key->state &= ~BillsScrewyAltMask;
	    } else
		altkey = 0;
	    if (XLookupString(key, &ch, 1, NULL, NULL) > 0) {
		wevent->type = W_EV_KEY;
		wevent->Window = W_Window2Void(win);
		wevent->x = key->x;
		wevent->y = key->y;
		if (controlkey)
		    wevent->key = (int) ch + 128;
		else if (altkey)
		    wevent->key = (int) ch + 256;
		else
		    wevent->key = ch;
		return (1);
	    }
	    return (0);
	case ButtonPress:
	    /* do the actual scrolling here [BDyess] */
	    if(win->type == WIN_SCROLL) {	/* scroll window [BDyess] */
	      switch (button->button & 0xf) {
	      case Button1:			/* scroll back [BDyess] */
		  if(win->currenttop > win->bottom + win->height) {
		    win->currenttop -= win->height;
		    if(win->currenttop - win->bottom < win->height) 
		      win->currenttop = win->bottom + win->height;
		  }
		  redrawScrolling(win);
		  break;
	      case Button2:			/* scroll to end [BDyess] */
		  win->currenttop = win->top;
		  redrawScrolling(win);
		  break;
	      case Button3:			/* scroll forward [BDyess] */
	          win->currenttop += win->height;
		  if(win->currenttop > win->top)
		    win->currenttop = win->top;
		  redrawScrolling(win);
		  break;
	      }
	      return 0;
	    }
	    wevent->type = W_EV_BUTTON;
	    wevent->Window = W_Window2Void(win);
	    wevent->x = button->x;
	    wevent->y = button->y;
	    switch (button->button & 0xf) {
	    case Button3:
		wevent->key = W_RBUTTON;
		break;
	    case Button1:
		wevent->key = W_LBUTTON;
		break;
	    case Button2:
		wevent->key = W_MBUTTON;
		break;
	    }
	    if (key->state & ControlMask)
		wevent->key += 6;
	    if (key->state & ShiftMask)
		wevent->key += 3;
	    if (key->state & BillsScrewyAltMask)
		wevent->key += 12;	/* alt */
	    if (continuousMouse && allowContinuousMouse &&
		(wevent->Window == w || wevent->Window == mapw) &&
	    /*
	       buttonRepeatMask allows only certain buttons to repeat
	       [BDyess]
	    */
		(1 << (wevent->key) & buttonRepeatMask)) {
		buttonDown = 1;
		exitInputLoop = 1;
		delaytime = clickDelay;
		bcopy(wevent, &buttonEvent, sizeof(W_Event));
	    }
	    return (1);
	case ButtonRelease:
	    /* bcopy(&buttonEvent,wevent,sizeof(W_Event)); */
	    wevent->type = -1;
	    buttonDown = 0;
	    return (1);
	case MotionNotify:
	    /*
	       the !buttonDown ensures that if you press a button and then
	       press another, release just the second, and then move the
	       mouse that nothing happens.
	    */
	    if (!(continuousMouse && allowContinuousMouse) || !buttonDown) {
		wevent->type = -1;
		return (1);
	    }
	    wevent->type = W_EV_BUTTON;
	    wevent->Window = W_Window2Void(win);
	    wevent->x = button->x;
	    wevent->y = button->y;
	    wevent->key = buttonEvent.key;
	    bcopy(wevent, &buttonEvent, sizeof(W_Event));
	    if (cupd == udcounter)
		wevent->type = -1;
	    else
		cupd = udcounter;

	    return (1);
	case Expose:
	    if (expose->count != 0)
		return (0);
	    if (win->type == WIN_SCROLL) {
		redrawScrolling(win);
		return (0);
	    }
	    if (win->type == WIN_MENU) {
		redrawMenu(win);
		return (0);
	    }
	    if (win->type == WIN_BORDER) {
	        redrawBorder(win);
		return (0);
	    }
	    if (win == W_Void2Window(baseWin)) {
	        /* main window has a reverse border [BDyess] */
		redrawReversedBorder(win);
	    }
	    wevent->type = W_EV_EXPOSE;
	    wevent->Window = W_Window2Void(win);
	    return (1);
	case ResizeRequest:
	    resizeScrolling(win, resize->width, resize->height);
	    break;
	case ClientMessage:
	    /* WM_DELETE_WINDOW support [BDyess] */
	    if(event.xclient.data.l[0] == wm_delete_window_atom) {
	      wevent->type = W_EV_KILL_WINDOW;
	      wevent->Window = W_Window2Void(win);
	      return 1;
	    } else {
	      return 0;
	    }
	default:
	    return (0);
	}
    }
}

void
W_MakeLine(W_Window window, int X0, int Y0, int X1, int Y1, W_Color color)
{
    Window  win;

#ifdef DEBUG
    printf("Line on %d\n", window);
#endif
    win = W_Void2Window(window)->drawable;
    XDrawLine(W_Display, win, colortable[color].contexts[0], X0, Y0, X1, Y1);
}

void
W_DrawPoint(W_Window window, int x, int y, W_Color color)
{
    Window  win;

#ifdef DEBUG
    printf("Point on %d\n", window);
#endif
    win = W_Void2Window(window)->drawable;
    XDrawPoint(W_Display, win, colortable[color].contexts[0], x, y);
}

/* XFIX */

static XSegment _lcache[NCOLORS][MAXCACHE];
static int _lcache_index[NCOLORS];

static void
FlushLineCache(Window win, int color)
{
    XDrawSegments(W_Display, win, colortable[color].contexts[0],
		  _lcache[color], _lcache_index[color]);
    _lcache_index[color] = 0;
}

/* for local window only */
void
W_CacheLine(W_Window window, int X0, int Y0, int X1, int Y1, int color)
{
    Window  win = W_Void2Window(window)->drawable;
    register XSegment *s;

    if (_lcache_index[color] == MAXCACHE)
	FlushLineCache(win, color);

    s = &_lcache[color][_lcache_index[color]++];
    s->x1 = (short) X0;
    s->y1 = (short) Y0;
    s->x2 = (short) X1;
    s->y2 = (short) Y1;
}

void
W_FlushLineCaches(W_Window window)
{
    Window  win = W_Void2Window(window)->drawable;
    register int i;
    for (i = 0; i < NCOLORS; i++) {
	if (_lcache_index[i])
	    FlushLineCache(win, i);
    }
}

static XPoint _pcache[NCOLORS][MAXCACHE];
static int _pcache_index[NCOLORS];

static void
FlushPointCache(Window win, int color)
{
    XDrawPoints(W_Display, win, colortable[color].contexts[0],
		_pcache[color], _pcache_index[color], CoordModeOrigin);
    _pcache_index[color] = 0;
}

void
W_CachePoint(W_Window window, int x, int y, int color)
{
    Window  win = W_Void2Window(window)->drawable;
    register XPoint *p;

    if (_pcache_index[color] == MAXCACHE)
	FlushPointCache(win, color);

    p = &_pcache[color][_pcache_index[color]++];
    p->x = (short) x;
    p->y = (short) y;
}

void
W_FlushPointCaches(W_Window window)
{
    Window  win = W_Void2Window(window)->drawable;
    register int i;
    for (i = 0; i < NCOLORS; i++) {
	if (_pcache_index[i])
	    FlushPointCache(win, i);
    }
}

void
W_MakeTractLine(W_Window window, int X0, int Y0, int X1, int Y1, W_Color color)
{
    Window  win;

#ifdef DEBUG
    printf("Line on %d\n", window);
#endif
    win = W_Void2Window(window)->drawable;
    XDrawLine(W_Display, win, colortable[color].contexts[3], X0, Y0, X1, Y1);
}

void
W_DrawSectorHighlight(W_Window window, int x, int y, int width, int h, 
                      W_Color color)
{
    Window  win;
    XRectangle r[1];

    r[0].x = (short) x + 2;
    r[0].y = (short) y + 2;
    r[0].width = (unsigned short) width - 4;
    r[0].height = (unsigned short) h - 4;

    win = W_Void2Window(window)->drawable;
    XDrawRectangles(W_Display, win, colortable[color].contexts[3],
		    r, 1);
}

void
W_WriteAnyTriangle(W_Window window, int X1, int Y1, int X2, int Y2, 
                   int X3, int Y3, W_Color color)
{
    struct window *win = W_Void2Window(window);
    XPoint  points[3];

    points[0].x = X1;
    points[0].y = Y1;
    points[1].x = X2;
    points[1].y = Y2;
    points[2].x = X3;
    points[2].y = Y3;

    XFillPolygon(W_Display, win->drawable, colortable[color].contexts[0],
		 points, 3, Convex, CoordModeOrigin);
}

void
W_WriteTriangle(W_Window window, int x, int y, int s, int t, W_Color color)
{
    struct window *win = W_Void2Window(window);
    XPoint  points[3];

    if (t == 0) {
	points[0].x = x;
	points[0].y = y;
	points[1].x = x + s;
	points[1].y = y - s;
	points[2].x = x - s;
	points[2].y = y - s;
    } else {
	points[0].x = x;
	points[0].y = y;
	points[1].x = x + s;
	points[1].y = y + s;
	points[2].x = x - s;
	points[2].y = y + s;
    }


    XFillPolygon(W_Display, win->drawable, colortable[color].contexts[0],
		 points, 3, Convex, CoordModeOrigin);
}

void
W_WriteText(W_Window window, int x, int y, W_Color color, char *str, 
            int len, W_Font font)
{
    struct window *win;
    int     addr;

    if (!font)
	font = W_RegularFont;
#ifdef DEBUG
    printf("Text for %d @ (%d, %d) in %d: [%s]\n", window, x, y, color, str);
#endif
    win = W_Void2Window(window);
    switch (win->type) {
    case WIN_GRAPH:
	addr = fonts[fontNum(font)].baseline;
	XDrawImageString(W_Display, win->drawable,
	  colortable[color].contexts[fontNum(font)], x, y + addr, str, len);
	break;
    case WIN_SCROLL:
        if(win->currenttop == win->top) {
	  if (y<0) {
	    XCopyArea(W_Display, win->drawable, win->drawable,
		      colortable[W_White].contexts[0], WIN_EDGE, MENU_PAD,
		      win->width * W_Textwidth, (win->height - 1) * W_Textheight,
		      WIN_EDGE, MENU_PAD+W_Textheight);
	    XClearArea(W_Display, win->window,
		       WIN_EDGE, MENU_PAD,
		       W_Textwidth * win->width,(unsigned) W_Textheight, False);
	    XDrawImageString(W_Display, win->drawable,
			     colortable[color].contexts[1],
			     WIN_EDGE, MENU_PAD + fonts[1].baseline,
			     str, len);
	  } else {
	    XCopyArea(W_Display, win->drawable, win->drawable,
		      colortable[W_White].contexts[0], WIN_EDGE, MENU_PAD + 
		      W_Textheight, win->width * W_Textwidth, (win->height - 1)
		      * W_Textheight, WIN_EDGE, MENU_PAD);
	    XClearArea(W_Display, win->window,
		       WIN_EDGE,(int)(MENU_PAD+W_Textheight*(win->height-1)),
		       W_Textwidth * win->width, (unsigned)W_Textheight, False);
	    XDrawImageString(W_Display, win->drawable,
			     colortable[color].contexts[1],
			     WIN_EDGE, 
			     (int)(MENU_PAD + W_Textheight * (win->height-1)
				      + fonts[1].baseline),
			     str, len);
	  }
	}
	AddToScrolling(win, color, str, len);
	break;
    case WIN_MENU:
	changeMenuItem(win, x, y, color, str, len, font);
	break;
    default:
	addr = fonts[fontNum(font)].baseline;
	XDrawImageString(W_Display, win->drawable,
			 colortable[color].contexts[fontNum(font)],
	     x * W_Textwidth + WIN_EDGE, MENU_PAD + y * W_Textheight + addr,
			 str, len);
	break;
    }
}

void
W_MaskText(W_Window window, int x, int y, W_Color color, char *str,
           int len, W_Font font)
{
    struct window *win;
    int     addr;

    addr = fonts[fontNum(font)].baseline;
#ifdef DEBUG
    printf("TextMask for %d @ (%d, %d) in %d: [%s]\n", window, x, y, color, str);
#endif
    win = W_Void2Window(window);
    XDrawString(W_Display, win->drawable,
	  colortable[color].contexts[fontNum(font)], x, y + addr, str, len);
}

/* same as W_MaskText above, except draws directly to the destination window
   and not its drawable.  This bypasses buffering, if any. [BDyess] */
void
W_DirectMaskText(W_Window window, int x, int y, W_Color color, char *str,
                 int len, W_Font font)
{
    struct window *win;
    int     addr;

    addr = fonts[fontNum(font)].baseline;
#ifdef DEBUG
    printf("TextMask for %d @ (%d, %d) in %d: [%s]\n", window, x, y, color, str);
#endif
    win = W_Void2Window(window);
    XDrawString(W_Display, win->window,
	  colortable[color].contexts[fontNum(font)], x, y + addr, str, len);
}

void
W_FreeImage(W_Image *image)
{
  if(image->loaded) {
    XFreePixmap(W_Display, image->pixmap);
    if(image->clipmask && image->xpm) XFreePixmap(W_Display, image->clipmask);
    image->pixmap = image->clipmask = 0;
  }
  if(image > getImage(I_LAST) || 
     image < getImage(I_FIRST)) { /* dynamically allocated */
    free(image);
  }
  image->loaded = 0;
  image->xpm = 0;
  image->frames = 0;
}

W_Image *
W_BitmapToImage(unsigned int width, unsigned int height, char *bits)
{
  W_Image *image = (W_Image*)malloc(sizeof(W_Image));

  image->clipmask = XCreateBitmapFromData(W_Display, W_Root,
                                              bits, width, height);
  image->pixmap = image->clipmask;
  image->frames = 1;
  if(height % width == 0) {  /* probably has multiple frames [BDyess] */
    image->frames = height / width;
    height /= image->frames;
  }
  image->width = width;
  image->height = height;
  image->xpm = 0;
  image->loaded = 1;
  image->bad = 0;
  image->filename = "";
  return image;
}

int
checkloaded(W_Image *image)
{
  if(!(image)->loaded) {
    /* not loaded, autoload [BDyess] */
    if(W_LoadImage(image)) {
      strcpy(imagedirend,(image)->filename);
      fprintf(stderr,"Something is very wrong.  I can't find any image to load for %s, and all the alternates fail too.  I'm too bummed to continue.\n", imagedir);
      exit(1);
    }
  }
  if((image)->bad) return 1;	/* don't draw bad images [BDyess] */
  return 0;
}

/*
   Takes an array of image pointers, NULL terminated, and combines them
   to produce a new image.

   All images are written centered, and the combined image is just large 
   enough to hold the largest passed image.  If one image has more frames
   than the others, the largest frame size is used and the smaller frame
   images are replicated over the entire sequence. [BDyess] 
*/
W_Image *
W_CreateCombinedImage(W_Image **imagelist, W_Color color)
{
  int width = 1, height = 1, frames = 1, i, centerx, centery;
  W_Image **list;
  W_Image *image;
  struct window fakewin;
  int tmpOR;

  for(list = imagelist; *list; list++) {
    if(!checkloaded(*list)) {
      if((*list)->width > width) width = (*list)->width;
      if((*list)->height > height) height = (*list)->height;
      if((*list)->frames > frames) frames = (*list)->frames;
    }
  }
  centerx = width / 2;
  centery = height / 2;
  image = (W_Image*)malloc(sizeof(W_Image));
  image->loaded = 1;
  image->xpm = 1;
  image->bad = 0;
  image->width = width;
  image->height = height;
  image->frames = frames;
  image->filename = "combined image";
  image->pixmap = XCreatePixmap(W_Display, W_Root, width, height * frames,
                                (unsigned)DefaultDepth(W_Display, W_Screen));
  image->clipmask = None;
  /* clear our new image */
  XFillRectangle(W_Display, image->pixmap, colortable[backColor].contexts[0], 
                 0, 0, width, height * frames);
  tmpOR = useOR;	/* don't want to OR when using W_DrawImage [BDyess] */
  useOR = 0;
  for(list = imagelist; *list; list++) {
    for(i=0;i < frames; i++) {
      /* copy the frames, one at a time */
      /* use a fake window for code reuse */
      fakewin.drawable = image->pixmap;
      W_DrawImage(W_Window2Void(&fakewin), centerx - (*list)->width / 2,
                            i*height + centery - (*list)->height / 2,
			    i,
			    *list,
			    color);
    }
  }
  useOR = tmpOR;
/*  printf("created composite image: width %d, height %d, frames %d.\n",
         width,height,frames);*/
  return image;
}

/* modification - even if there IS an image loading error I don't want
   to see it.  The load-all-images attempts to load every image even
   if it would never be a valid image.  We'll let the developers take
   care of insuring that there is a complete image set available by
   default; end users wishing to customize their display can do so on
   their own with the help of -v. */
int
W_LoadExternalImage(W_Image *image)
{
  XpmAttributes attributes;
  int ErrorStatus;
  char *warn = NULL, *error = NULL;
  char buf[256], *t, *pi;
  W_Image *imagetmp;

  if(xpm) {
    attributes.valuemask = XpmCloseness|XpmReturnExtensions|XpmColormap;
    attributes.extensions = NULL;
    attributes.colormap = W_Colormap;
    /* take colors that are close [BDyess] */
    attributes.closeness = 40000;

    pi = imagedir;
    do
    {
      t = strchr(pi, ':');
      strncpy(buf, pi, (t ? t - pi : strlen(pi)));
      sprintf(buf + (t ? t - pi : strlen(pi)), "%s.xpm", image->filename);
      ErrorStatus = XpmReadFileToPixmap(W_Display, W_Root, buf,
                                        &image->pixmap, &image->clipmask,
					&attributes);
      pi = (t ? t+1 : t);
    } while((ErrorStatus == XpmOpenFailed) && pi);

    switch (ErrorStatus) {
      case XpmColorError:
	  warn = "Could not parse or alloc requested color";
	  break;
      case XpmOpenFailed:
	  error = "Cannot open file";
	  break;
      case XpmFileInvalid:
	  error = "Invalid XPM file";
	  break;
      case XpmNoMemory:
	  error = "Not enough memory";
	  break;
      case XpmColorFailed:
	  error = "Failed to parse or alloc some color";
	  break;
    }
  } else { /* fool the rest of the code into thinking that it is xpm mode
              but loading the xpm failed */
    error = "";
    ErrorStatus = XpmOpenFailed;
  }

  if (error) {
    if(ErrorStatus != XpmOpenFailed && verbose_image_loading) {
      printf("Error reading in %s: %s.\n", image->filename+1, error);
    }
    /* 
     * ok, can't find or load the xpm, try the xbm [BDyess] 
     */
    
    pi = imagedir;
    do
    {
      char *t, buf[256];

      t = strchr(pi, ':');
      strncpy(buf, pi, (t ? t - pi : strlen(pi)));
      sprintf(buf + (t ? t - pi : strlen(pi)), "%s.xbm", image->filename);
      ErrorStatus = XReadBitmapFile(W_Display, W_Root, buf,
                                    &image->width, &image->height,
				    &image->clipmask, NULL, NULL);
      pi = (t ? t+1 : t);
    } while((ErrorStatus != BitmapSuccess) && pi);

    if( ErrorStatus != BitmapSuccess) {
      if(verbose_image_loading)
        printf("Bitmap read failed for %s (%d).\n" , 
	       image->filename+1, getImageNum(image));
/* NEW STUFF */
      /*abort();*/
      /* GAH don't try the alternate before the built-in one if it exists! */
      if(image->compiled_in)
        return(W_LoadInternalImage(image));
      /* uh oh, no good image files.  Lets try the alternate [BDyess] */
      if(image->alternate == -1) { /* bad news, no alternate [BDyess] */
	return 1;		/* let the calling function handle it [BDyess]*/
      }
      if(image->alternate == I_DEFAULT && verbose_image_loading) {
        printf("Using compiled in default image...\n");
      } else {
	/*printf("Trying to load alternate (%d)...\n",image->alternate);*/
      }
      image->bad = 1;		/* marks image as bad.  Stops infinite
      				   recursion if alternate = current [BDyess] */
      /* getImage will load the image if it's not already loaded [BDyess] */
      /* this is recursive [BDyess] */
      imagetmp = getImage(image->alternate);
      /* copy the alternate to the current image */
      /* note that since the image data is stored on the X server and
	 the image structure just keeps a id for it, this doesn't
	 make a duplicate copy of the image itself [BDyess] */
      memcpy(image,imagetmp,sizeof(W_Image));
      return 0;		/* return success [BDyess] */
    }
    if(verbose_image_loading) printf("xbm\n");
    /* bitmap loaded successfully */
    image->loaded = 1;
    image->pixmap = image->clipmask;
    if(image->frames == 0) {
    /* if an image doesn't have a number of frames, then guess how many there
       are by assuming that a frame is square.  This is easily overriden by
       setting frames = 1. [BDyess] */
      if((image->height % image->width) == 0) {   /* even multiple */
        image->frames = image->height / image->width;
      } else {
        image->frames = 1;
      }
    }
    image->height /= image->frames;
    image->xpm = 0;
    return 0;
  }
  if(verbose_image_loading) printf("xpm\n");
  if (warn && verbose_image_loading)
      printf("Warning reading in %s: %s.\n", buf, warn);
  /* ok, xpm loaded successfully.  Lets store the data and get outta here
     [BDyess] */
  image->loaded = 1;
  image->xpm = 1;
  image->width = attributes.width;
  /* check for an extension that changes the number of frames [BDyess] */
  if(attributes.extensions) {
    int i;

    for (i=0; i<attributes.nextensions; i++) {
	if (0==strcmp("num_views", attributes.extensions[i].name))
	    break;
	if (0==strcmp("nviews", attributes.extensions[i].name))
	    break;
    }
    if (i<attributes.nextensions) {
      image->frames = atoi(attributes.extensions[i].lines[0]);
    }
  }
  if(image->frames == 0) {
  /* if an image doesn't have a number of frames, then guess how many there
     are by assuming that a frame is square.  This is easily overriden by
     setting frames = 1. [BDyess] */
    if((attributes.height % attributes.width) == 0) {   /* even multiple */
      image->frames = attributes.height / attributes.width;
    } else {
      image->frames = 1;
    }
  }
  image->height = attributes.height / image->frames;
  return 0;
} 

/* loads a compiled_in image [BDyess] */
int
W_LoadInternalImage(W_Image *image)
{
  XpmAttributes attributes;
  int ErrorStatus;
  W_Image *imagetmp;

  if(xpm && image->xpmdata) {
    attributes.valuemask = XpmCloseness|XpmReturnExtensions|XpmColormap;
    attributes.extensions = NULL;
    attributes.colormap = W_Colormap;
    /* take colors that are close [BDyess] */
    attributes.closeness = 40000;

    sprintf(imagedirend,"%s.xpm",image->filename);
    ErrorStatus = XpmCreatePixmapFromData(W_Display, W_Root, image->xpmdata,
			    &image->pixmap, &image->clipmask, &attributes);
    if(verbose_image_loading) printf("xpm\n");
    image->xpm = 1;
    image->height = attributes.height;
    image->width = attributes.width;
    /* check for an extension that changes the number of frames [BDyess] */
    if(attributes.extensions) {
      int i;

      for (i=0; i<attributes.nextensions; i++) {
	  if (0==strcmp("num_views", attributes.extensions[i].name))
	      break;
	  if (0==strcmp("nviews", attributes.extensions[i].name))
	      break;
      }
      if (i<attributes.nextensions) {
	image->frames = atoi(attributes.extensions[i].lines[0]);
      }
    }
  } else if(image->xbmdata) {
    image->clipmask = XCreateBitmapFromData(W_Display, W_Root, 
		      (char *) image->xbmdata, image->width, image->height);
    if(verbose_image_loading) printf("xbm\n");
    image->pixmap = image->clipmask;
    image->xpm = 0;
  } else {
    /* No image data.  Lets try the alternate [BDyess] */
    if(image->alternate == -1) { /* bad news, no alternate [BDyess] */
      fprintf(stderr,"Error: bad image data for %s.\n",image->filename);
      return 1;		/* let the calling function handle it [BDyess]*/
    }
    if(image->alternate == I_DEFAULT) {
      printf("Using compiled in default image in place of %s...\n",
	     image->filename);
    }
    image->bad = 1;		/* marks image as bad.  Stops infinite
				 recursion if alternate = current [BDyess] */
    /* getImage will load the image if it's not already loaded [BDyess] */
    /* this is recursive [BDyess] */
    imagetmp = getImage(image->alternate);
    /* copy the alternate to the current image */
    /* note that since the image data is stored on the X server and
       the image structure just keeps a id for it, this doesn't
       make a duplicate copy of the image itself [BDyess] */
    memcpy(image,imagetmp,sizeof(W_Image));
    return 0;		/* return success [BDyess] */
  }
  /* image loaded successfully [BDyess] */
  if(image->frames == 0) {
  /* if an image doesn't have a number of frames, then guess how many there
     are by assuming that a frame is square.  This is easily overriden by
     setting frames = 1. [BDyess] */
    if((image->height % image->width) == 0) {   /* even multiple */
      image->frames = image->height / image->width;
    } else {
      image->frames = 1;
    }
  }
  image->height /= image->frames;
  image->loaded = 1;
  return 0;
}

int
W_LoadImage(W_Image *image)
{
  int ret;
  char buf[BUFSIZ];

  if(image->loaded) return 0;
  if(image->bad) {
    fprintf(stderr,"Error, tried to load bad image (%s).\n",image->filename);
    abort();
    return 1;
  }

  if(!image->compiled_in && verbose_image_loading) {
    sprintf(buf,"%s %s...",image->compiled_in ? "Storing" : "Loading",
            image->filename+1);
    warning(buf);
    if(verbose_image_loading) {
      puts(buf);
      fflush(stdout);
    }
  }

  if(useExternalImages && getImageNum(image) != I_DEFAULT) {
    ret = W_LoadExternalImage(image);
    if(ret) ret = W_LoadInternalImage(image);	/* external load failed */
  } else if(image->compiled_in) {
    ret = W_LoadInternalImage(image);
  } else {
    ret = W_LoadExternalImage(image);
  }
  *imagedirend = 0;
  return ret;
}
    

/* similar to W_DrawImage except the clipmask used for image is the sum of the
   clipmasks for image and clipimage.  Currently this is only used for cloaking.
   [BDyess] */
void
W_OverlayImage(W_Window window, int x, int y, int frame, W_Image *image,
               int overframe, W_Image *overimage, W_Color color)
{
  int width, height;
  Pixmap workarea;
  struct window foolwin;
  int tmpOR;

  if(checkloaded(image) || checkloaded(overimage)) return;
  width = image->width > overimage->width ? image->width : overimage->width;
  height =image->height > overimage->height ? image->height : overimage->height;
  frame = frame % image->frames;
  overframe = overframe % overimage->frames;

  /* create a temporary offscreen working area [BDyess] */
  workarea = XCreatePixmap(W_Display, W_Root, width, height, 
                           DefaultDepth(W_Display, W_Screen));
  /* fool other routines that want a window into using the work area [BDyess] */
  foolwin.drawable = workarea;
  
  /* copy the first image there [BDyess] */
  W_DrawImageNoClip(W_Window2Void(&foolwin), 
                   (width - image->width) / 2, 
		   (height - image->height) / 2, 
		   frame, image, color);

  /* copy the second image over it [BDyess] */
  /* don't want to use OR... [BDyess] */
  tmpOR = useOR;
  useOR = 0;
  W_DrawImage(W_Window2Void(&foolwin), 
              (width - overimage->width) / 2,
	      (height - overimage->height) / 2, 
	      overframe, overimage, color);
  useOR = tmpOR;

  /* display the work area [BDyess] */
  if(xpm && !useOR) {
      XSetClipOrigin(W_Display, colortable[W_Black].contexts[BITMASKGC], x, 
		     (int)(y - (image->height * frame)));
      XSetClipMask(W_Display, colortable[W_Black].contexts[BITMASKGC], 
		   image->clipmask);
      XCopyArea(W_Display, workarea, W_Void2Window(window)->drawable,
	 colortable[W_Black].contexts[BITMASKGC], 0, 0, width, height, 
	 x - (width - image->width) / 2, 
	 y - (height - image->height) / 2);
  } else {    
      XCopyArea(W_Display, workarea, W_Void2Window(window)->drawable,
	   colortable[color].contexts[BITGC], 0, 0, width, height, x, y);
  }

  /* clean up [BDyess] */
  XFreePixmap(W_Display, workarea);
}
  
void
W_DrawImage(W_Window window, int x, int y, int frame, W_Image *image,
            W_Color color)
{
  int height, width;

  if(checkloaded(image)) return;
  if(frame < 0) {	/* draw the whole thing regardless of frames [BDyess] */
    height = image->height * image->frames;
    frame = 0;
  } else {		/* draw the frame given [BDyess] */
    height = image->height;
    frame = frame % image->frames;
  }
  width = image->width;
  /*
   * three modes:
   * xpm mode - full X-based clipping of color pixmaps
   * cookie mode - cookie-cutter clipping of color pixmaps
   * OR mode - OR drawing of pixmaps or bitmaps.
   */
  if(cookie) {
    if(image->xpm) {
      /* blank out area to draw into */
      if(image->clipmask) {
        /* a clipmask is not guaranteed [BDyess] */
	XCopyPlane(W_Display, image->clipmask, W_Void2Window(window)->drawable,
	     maskGC, 0,
	     (int)(height * frame), 
	     width, height, x, y, 1);
      } else {
        /* no clipmask, so use a rectangle [BDyess] */
	XFillRectangle(W_Display, W_Void2Window(window)->drawable,
		       maskGC, x, y, width, height);
      }
      /* draw image in it */
      XCopyArea(W_Display, image->pixmap, W_Void2Window(window)->drawable,
           colortable[color].contexts[BITGC], 0,
           (int)(height * frame), width, height, x, y);
    } else
      XCopyPlane(W_Display, image->pixmap, W_Void2Window(window)->drawable,
           colortable[color].contexts[BITGC], 0, 
           (int)(height * frame), 
	   width, height, x, y, 1);
  } else if(/*xpm &&*/ !useOR) {
    /* full clipping mode (no flashing, high CPU) [BDyess] */
    if(image->xpm) {
      XSetClipOrigin(W_Display, colortable[W_Black].contexts[BITMASKGC], x, 
		     (int)(y - (height * frame)));
      XSetClipMask(W_Display, colortable[W_Black].contexts[BITMASKGC], 
		   image->clipmask);
      XCopyArea(W_Display, image->pixmap, W_Void2Window(window)->drawable,
	 colortable[W_Black].contexts[BITMASKGC], 0,
	 (int)(height * frame), width, height, x, y);
    } else {
      XSetClipOrigin(W_Display, colortable[color].contexts[BITMASKGC], x, 
		     (int)(y - (height * frame)));
      XSetClipMask(W_Display, colortable[color].contexts[BITMASKGC], 
		   image->clipmask);
      XCopyPlane(W_Display, image->pixmap, W_Void2Window(window)->drawable,
	   colortable[color].contexts[BITMASKGC], 0, 
	   (int)(height * frame), 
	   width, height, x, y, 1);
    }
  } else {    
    /* OR mode - odd color mixing, fast [BDyess] */
    if(image->xpm)
      XCopyArea(W_Display, image->pixmap, W_Void2Window(window)->drawable,
	   colortable[color].contexts[BITGC], 0,
	   (int)(height * frame), width, height, x, y);
    else
      XCopyPlane(W_Display, image->pixmap, W_Void2Window(window)->drawable,
	   colortable[color].contexts[BITGC], 0, 
	   (int)(height * frame), 
	   width, height, x, y, 1);
  }
}

/* same as W_DrawImage except that clipmask is not used [BDyess] */
void
W_DrawImageNoClip(W_Window window, int x, int y, int frame, W_Image *image,
                  W_Color color)
{
  if(checkloaded(image)) return;
  frame = frame % image->frames;
  if(xpm) {
    if(image->xpm) {
      /*XSetClipMask(W_Display, colortable[W_Black].contexts[BITMASKGC], 
		   None);*/
      XCopyArea(W_Display, image->pixmap, W_Void2Window(window)->drawable,
	 colortable[W_Black].contexts[BITMASKNOCLIPGC], 0,
	 (int)(image->height * frame), image->width, image->height, x, y);
    } else {
      /*XSetClipMask(W_Display, colortable[color].contexts[BITMASKGC], 
		   None);*/
      XCopyPlane(W_Display, image->pixmap, W_Void2Window(window)->drawable,
	   colortable[color].contexts[BITMASKNOCLIPGC], 0, 
	   (int)(image->height * frame), 
	   image->width, image->height, x, y, 1);
    }
  } else {    
    if(image->xpm)
      XCopyArea(W_Display, image->pixmap, W_Void2Window(window)->drawable,
	   colortable[color].contexts[BITGC], 0,
	   (int)(image->height * frame), image->width, image->height, x, y);
    else
      XCopyPlane(W_Display, image->pixmap, W_Void2Window(window)->drawable,
	   colortable[color].contexts[BITGC], 0, 
	   (int)(image->height * frame), 
	   image->width, image->height, x, y, 1);
  }
}

/* same as W_DrawImage except image is drawn using GXor instead of GXcopy 
   [BDyess] */
void
W_DrawImageOr(W_Window window, int x, int y, int frame, W_Image *image,
              W_Color color)
{
  if(checkloaded(image)) return;
  frame = frame % image->frames;
  if(image->xpm) {
      XCopyArea(W_Display, image->pixmap, W_Void2Window(window)->drawable,
	 colortable[W_Black].contexts[BITGC], 0,
	 (int)(image->height * frame), image->width, image->height, x, y);
  } else {    
    XCopyPlane(W_Display, image->pixmap, W_Void2Window(window)->drawable,
	 colortable[color].contexts[BITGC], 0, 
	 (int)(image->height * frame), 
	 image->width, image->height, x, y, 1);
  }
}


void
W_DrawImageBar(W_Window win, int x, int y, int len, W_Image *image)
{
   if(len == 0) return;
   XSetClipMask(W_Display,colortable[W_Black].contexts[BITMASKGC],None);
   XCopyArea(W_Display, image->pixmap, W_Void2Window(win)->drawable,
       colortable[W_Black].contexts[BITMASKGC], 0, 0, 
       (unsigned)len+1, image->height, x, y);
}

void
W_TileWindow(W_Window window, W_Image *image)
{
    Window  win;

    if(checkloaded(image)) return;
#ifdef DEBUG
    printf("Tiling window %d\n", window);
#endif
    win = W_Void2Window(window)->window;

    if (image->xpm == 0) {
	Pixmap orig;
	orig = image->pixmap;
	image->pixmap = XCreatePixmap(W_Display, W_Root,
	      image->width, image->height, 
	      (unsigned)DefaultDepth(W_Display, W_Screen));
	XCopyPlane(W_Display, orig, image->pixmap,
	   colortable[W_White].contexts[0], 0, 0, image->width, image->height,
		   0, 0, 1);
	image->xpm = 1;
    }
    XSetWindowBackgroundPixmap(W_Display, win, image->pixmap);
    XClearWindow(W_Display, win);
}

void
W_UnTileWindow(W_Window window)
{
    Window  win;

#ifdef DEBUG
    printf("Untiling window %d\n", window);
#endif
    win = W_Void2Window(window)->window;

    XSetWindowBackground(W_Display, win, colortable[W_Black].pixelValue);
    XClearWindow(W_Display, win);
}

W_Window
W_MakeTextWindow(char *name, int x, int y, int width, int height,
                 W_Window parent, char *cursname, int border)
{
    return w_MakeWindow(name, x, y, width, height,
			parent, cursname, border, W_White, WIN_TEXT);
}

struct window *
newWindow(Window window, int type)
{
    struct window *neww;

    neww = (struct window *) malloc(sizeof(struct window));
    neww->window = window;
    neww->drawable = window;
    neww->isbuffered = 0;
    neww->buffer = 0;
    neww->type = type;
    neww->mapped = 0;
    neww->insensitive = 0;
    addToHash(neww);
    return (neww);
}


static struct window *
findWindow(Window window)
{
    struct windowlist *entry;

    entry = hashtable[hash(window)];
    while (entry != NULL) {
	if (entry->window->window == window)
	    return (entry->window);
	entry = entry->next;
    }
    return (NULL);
}

static void
addToHash(struct window *win)
{
    struct windowlist **new;

#ifdef DEBUG
    printf("Adding to %d\n", hash(win->window));
#endif
    new = &hashtable[hash(win->window)];
    while (*new != NULL) {
	new = &((*new)->next);
    }
    *new = (struct windowlist *) malloc(sizeof(struct windowlist));
    (*new)->next = NULL;
    (*new)->window = win;
}

W_Window
W_MakeScrollingWindow(char *name, int x, int y, int width, int height,
                      W_Window parent, char *cursname, int border)
{
    return w_MakeWindow(name, x, y, width, height, parent, cursname,
			border, W_White, WIN_SCROLL);
}

/* Add a string to the string list of the scrolling window.
 */
static void
AddToScrolling(struct window *win, W_Color color, char *str, int len)
{
    int cur = win->top % win->maxlines;
    char *newstring;
  
    if(win->top - win->bottom >= win->maxlines) {
      free(win->scrolldata[cur].string);
      win->bottom++;
    }
    newstring = (char*)malloc((unsigned)(len+1));
    strncpy(newstring, str, (unsigned)len);
    newstring[len] = 0;
    win->scrolldata[cur].string = newstring;
    win->scrolldata[cur].color = color;
    win->scrolldata[cur].len = len;
    if(win->currenttop == win->top) win->currenttop++;
    win->top++;
    W_UpdateScrollBar(W_Window2Void(win));
}

void
W_UpdateScrollBar(W_Window window)
{
  struct window *win = W_Void2Window(window);
  int height,bottom;
  int lines = win->top - win->bottom;
  struct window *scrollbarwin = W_Void2Window(win->scrollbarwin);
  int pheight = scrollbarwin->height;

  W_ClearWindow(win->scrollbarwin);
  XDrawLine(W_Display, scrollbarwin->drawable, colortable[W_Red].contexts[0], 
            0, 0, 0, pheight);
  if(lines == 0) lines = 1;
  height = pheight * win->height / lines;
  if(height > pheight) height = pheight;
  if(height < 1) height = 1;
  if(win->currenttop - win->bottom == 0)
    bottom = pheight;
  else
    bottom = (win->currenttop - win->bottom) * pheight / lines;
  XFillRectangle(W_Display, scrollbarwin->drawable,
#ifdef BROKEN_STIPPLE
		 colortable[W_Grey].contexts[0],
#else
		 stippleGC,
#endif /*BROKEN_STIPPLE*/
                 1, bottom - height, scrollbarwin->width - 1, height);
}

void
W_SetSensitive(W_Window window, int v)
{
    struct window *win = W_Void2Window(window);

    win->insensitive = !v;

    if (win->type == WIN_SCROLL)
	redrawScrolling(win);
}

/* this function will redraw a border window [BDyess] */
static void
redrawBorder(struct window *win)
{
    XPoint points[3];
    XGCValues val;

    if(win->border == 0) return;
    val.line_width = win->border*2 + 1;
    if(win->border_color == NONE) {
      /* GC is set up so that lines are drawn with thickness BORDER, so only one
	 line is needed for each side.  The background is set to DARK_GREY,
	 which is the color of the left and top sides, so only two other
	 lines need to be drawn. [BDyess] */
      XChangeGC(W_Display, borderGC, GCLineWidth, &val);
      points[0].x = win->border/2;
      points[0].y = win->height;
      points[1].x = win->width;
      points[1].y = win->height;
      points[2].x = win->width;
      points[2].y = win->border/2;
      XDrawLines(W_Display, win->drawable, borderGC, points, 3,
                 CoordModeOrigin);
    } else {	/* draw a color border */
      val.foreground = colortable[win->border_color].pixelValue;
      XChangeGC(W_Display, borderGC, GCLineWidth | GCForeground, &val);
      XDrawRectangle(W_Display, win->drawable, borderGC, win->border - 1,
		     win->border - 1, 
		     win->width - 2 * win->border + 2,
		     win->height - 2 * win->border + 2);
      XSetForeground(W_Display, borderGC, colortable[LIGHT_GREY].pixelValue);
    }
}

/* this function will redraw the main (background) window, which contains a 
   reverse-border.  [BDyess] */
static void
redrawReversedBorder(struct window *win)
{
    XPoint points[3];
    XGCValues val;

    /* GC is set up so that lines are drawn with thickness BORDER, so only one
       line is needed for each side.  The background is set to DARK_GREY,
       which is the color of the left and top sides, so only two other
       lines need to be drawn. [BDyess] */
    val.line_width = BORDER*2 + 1;
    XChangeGC(W_Display, borderGC, GCLineWidth, &val);
    points[0].x = 0;
    points[0].y = win->height - BORDER/2;
    points[1].x = 0;
    points[1].y = 0;
    points[2].x = win->width - BORDER/2;
    points[2].y = 0; 
    XDrawLines(W_Display, win->drawable, borderGC, points, 3, CoordModeOrigin);
}

static void
redrawScrolling(struct window *win)
{
    int i,y,loc;

    if(win->top == win->bottom) return; /*initial condition - no data [BDyess]*/
    if(win->isbuffered) 
      W_ClearBuffer(W_Window2Void(win));
    else
      XClearWindow(W_Display, win->window);

    for(i=win->currenttop-win->height, y = 0; i < win->currenttop; i++,y++) {
      if(i >= win->bottom) {	/* data available [BDyess] */
        loc = i % win->maxlines;
	XDrawImageString(W_Display, win->drawable, 
			 colortable[win->scrolldata[loc].color].contexts[1],
			 WIN_EDGE, 
			 MENU_PAD + y * W_Textheight + fonts[1].baseline,
			 win->scrolldata[loc].string, win->scrolldata[loc].len);
      }
    }
    W_UpdateScrollBar(W_Window2Void(win));
}

static void
resizeScrolling(struct window *win, int width, int height)
{
    win->height = (height - MENU_PAD * 2) / W_Textheight;
    win->width = (width - WIN_EDGE * 2) / W_Textwidth;
    XResizeWindow(W_Display, win->window, win->width * W_Textwidth + WIN_EDGE * 2,
		  win->height * W_Textheight + MENU_PAD * 2);
}

W_Window
W_MakeMenu(char *name, int x, int y, int width, int height, 
           W_Window parent, int border)
{
    return w_MakeWindow(name, x, y, width, height, parent,
			"left_ptr", border, W_White, WIN_MENU);
}

static void
redrawMenu(struct window *win)
{
    int     count;

    for (count = 1; count < win->height; count++) {
	XFillRectangle(W_Display, win->drawable,
		       colortable[W_Grey].contexts[0],
	  0, count * (W_Textheight + MENU_PAD * 2) + (count - 1) * MENU_BAR,
		       win->width * W_Textwidth + WIN_EDGE * 2, MENU_BAR);
    }
    for (count = 0; count < win->height; count++) {
	redrawMenuItem(win, count);
    }
}

static void
redrawMenuItem(struct window *win, int n)
{
    struct menuItem *items;
    int     addr;

    items = (struct menuItem *) win->data;

    XFillRectangle(W_Display, win->drawable,
		   colortable[W_Black].contexts[0],
	  WIN_EDGE, n * (W_Textheight + MENU_PAD * 2 + MENU_BAR) + MENU_PAD,
		   win->width * W_Textwidth, (unsigned)W_Textheight);
    if (items[n].string) {
	addr = fonts[fontNum(items[n].font)].baseline;
	XDrawImageString(W_Display, win->drawable,
		colortable[items[n].color].contexts[fontNum(items[n].font)],
			 WIN_EDGE + items[n].x * W_Textwidth,
	     n * (W_Textheight + MENU_PAD * 2 + MENU_BAR) + MENU_PAD + addr,
			 items[n].string, (int)strlen(items[n].string));
    }
}

static void
changeMenuItem(struct window *win, int x, int n, W_Color color, char *str,
               int len, W_Font font)
{
    struct menuItem *items;
    char   *news;

    items = (struct menuItem *) win->data;
    if (items[n].string) {
	free(items[n].string);
    }
    news = (char*)malloc((unsigned)(len + 1));
    strncpy(news, str, (unsigned)len);
    news[len] = 0;
    items[n].string = news;
    items[n].color = color;
    items[n].font = font;
    items[n].x = x;
    redrawMenuItem(win, n);
    XFlush(W_Display);
}

static  Cursor
make_cursor(char *bits, char *mask, unsigned int width, unsigned int height,
            unsigned int xhot, unsigned int yhot)
{
    Pixmap  cursbits;
    Pixmap  cursmask;
    XColor  whiteCol, blackCol;
    Cursor  curs;

    whiteCol.pixel = colortable[W_White].pixelValue;
    XQueryColor(W_Display, W_Colormap, &whiteCol);
    blackCol.pixel = colortable[W_Black].pixelValue;
    XQueryColor(W_Display, W_Colormap, &blackCol);

    cursbits = XCreateBitmapFromData(W_Display, DefaultRootWindow(W_Display),
				     bits, width, height);
    cursmask = XCreateBitmapFromData(W_Display, DefaultRootWindow(W_Display),
				     mask, width, height);

    curs = XCreatePixmapCursor(W_Display, cursbits, cursmask,
			       &whiteCol, &blackCol, xhot, yhot);

    XFreePixmap(W_Display, cursbits);
    XFreePixmap(W_Display, cursmask);
    return curs;
}

void
W_DefineTCrossCursor(W_Window window)
{
    return;
}

void
W_DefineTextCursor(W_Window window)
{
    static Cursor new = 0;
    struct window *win = W_Void2Window(window);
    XColor  f, b;

    if (!new) {
	f.pixel = colortable[W_Yellow].pixelValue;
	b.pixel = colortable[W_Black].pixelValue;

	XQueryColor(W_Display, W_Colormap, &f);
	XQueryColor(W_Display, W_Colormap, &b);

	new = XCreateFontCursor(W_Display, XC_xterm);

	XRecolorCursor(W_Display, new, &f, &b);
    }
    XDefineCursor(W_Display, win->window, new);

    return;
}

void
W_RevertCursor(W_Window window)
{
    struct window *win = W_Void2Window(window);

    XDefineCursor(W_Display, win->window, win->cursor);

    return;
}

void
W_DefineCursor(W_Window window, int width, int height, char *bits, char *mask,
               int xhot, int yhot)
{
    return;
}

void
W_Beep(void)
{
    XBell(W_Display, 0);
}

int
W_WindowWidth(W_Window window)
{
    return (W_Void2Window(window)->width);
}

int
W_WindowHeight(W_Window window)
{
    return (W_Void2Window(window)->height);
}

int
W_Socket(void)
{
    return (ConnectionNumber(W_Display));
}

void
W_DestroyWindow(W_Window window)
{
    struct window *win;

#ifdef DEBUG
    printf("Destroying %d\n", window);
#endif
    win = W_Void2Window(window);
    deleteWindow(win);
    XDestroyWindow(W_Display, win->window);
    if(win->buffer) XFreePixmap(W_Display, win->buffer);
    if(win->type != WIN_BORDER && win->borderwin) 
      W_DestroyWindow(win->borderwin);
    free((char *) win);
}

static void
deleteWindow(struct window *window)
{
    struct windowlist **rm;
    struct windowlist *temp;

    rm = &hashtable[hash(window->window)];
    while (*rm != NULL && (*rm)->window != window) {
	rm = &((*rm)->next);
    }
    if (*rm == NULL) {
	printf("Attempt to delete non-existent window!\n");
	return;
    }
    temp = *rm;
    *rm = temp->next;
    free((char *) temp);
}

void
W_SetIconWindow(W_Window win, W_Window icon)
{
    XWMHints hints;

    XSetIconName(W_Display, W_Void2Window(icon)->window, W_Void2Window(win)->name);

    hints.flags = IconWindowHint;
    hints.icon_window = W_Void2Window(icon)->window;
    XSetWMHints(W_Display, W_Void2Window(win)->window, &hints);
}

/* fixed so that it handles negative positions 12/21/93          */
/* note that this is NOT standard X syntax, but it was requested */
/* and it's how BRM-Hadley works.                       [BDyess] */
static void
checkGeometry(char *name, int *x, int *y, int *width, int *height)
{
    char   *adefault;
    char    buf[100];
    char   *s;

    *x = *y = INVALID_POSITION;

    sprintf(buf, "%s.geometry", name);
    adefault = stringDefault(buf,NULL);
    if (adefault == NULL)
	return;
    /* geometry should be of the form 502x885+1+1, 502x885, or +1+1 */
    s = adefault;
    if (*s != '+' && *s != '-') {
	while (*s != 'x' && *s != 0)
	    s++;
	*width = atoi(adefault);
	if (*s == 0)
	    return;
	s++;
	adefault = s;
	while (*s != '+' && *s != '-' && *s != 0)
	    s++;
	*height = atoi(adefault);
	if (*s == 0)
	    return;
    }
    adefault = s;
    s++;
    if (*s == '-')
	s++;			/* for the case where they have wxh+-x+y */
    while (*s != '+' && *s != '-' && *s != 0)
	s++;
    *x = atoi(adefault + 1);
    if (*adefault == '-')
	*x = -*x;
    if (*s == 0)
	return;
    *y = atoi(s + 1);
    if (*s == '-')
	*y = -*y;
    /* printf("width: %d, height: %d, x: %d, y: %d\n",*width, *height,*x,*y); */
    return;
}

static void
checkParent(char *name, W_Window *parent)
{
    char   *adefault;
    char    buf[100];
    int     i;
    struct windowlist *windows;

    sprintf(buf, "%s.parent", name);
    adefault = stringDefault(buf,NULL);
    if (adefault == NULL)
	return;
    /* parent must be name of other window or "root" */
    if (strcasecmp(adefault, "root") == 0) {
	*parent = W_Window2Void(&myroot);
	return;
    }
    for (i = 0; i < HASHSIZE; i++) {
	windows = hashtable[i];
	while (windows != NULL) {
	    if (strcasecmp(adefault, windows->window->name) == 0) {
		*parent = W_Window2Void(windows->window);
		return;
	    }
	    windows = windows->next;
	}
    }
}

#define cross_width 16
#define cross_height 16
#define cross_x_hot 7
#define cross_y_hot 7
static unsigned char cross_bits[] = {
    0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0xc0, 0x01, 0x80, 0x00,
    0x10, 0x04, 0x3f, 0x7e, 0x10, 0x04, 0x80, 0x00, 0xc0, 0x01, 0x80, 0x00,
0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x00, 0x00};
static unsigned char crossmask_bits[] = {
    0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0xe0, 0x03, 0xd0, 0x05,
    0xbf, 0x7e, 0x7f, 0x7f, 0xbf, 0x7e, 0xd0, 0x05, 0xe0, 0x03, 0xc0, 0x01,
0xc0, 0x01, 0xc0, 0x01, 0xc0, 0x01, 0x00, 0x00};

static void
checkCursor(char *name, char *cursname, Cursor *cursor)
{
    char    buf[100];
    unsigned cnum;
    char   *adefault;

    *cursor = 0;

    sprintf(buf, "%s.cursor", name);
    adefault = stringDefault(buf,cursname);
    if (adefault == NULL)
	return;

    cnum = XmuCursorNameToIndex(adefault);
    if (cnum != -1) {
	XColor  f, b;
	*cursor = XCreateFontCursor(W_Display, cnum);
	if (cnum == XC_xterm) {

	    f.pixel = colortable[W_Yellow].pixelValue;
	    b.pixel = colortable[W_Black].pixelValue;

	    XQueryColor(W_Display, W_Colormap, &f);
	    XQueryColor(W_Display, W_Colormap, &b);

	    XRecolorCursor(W_Display, *cursor, &f, &b);
	} else if (cnum == XC_pirate) {
	    f.pixel = colortable[W_Red].pixelValue;
	    b.pixel = colortable[W_Black].pixelValue;

	    XQueryColor(W_Display, W_Colormap, &f);
	    XQueryColor(W_Display, W_Colormap, &b);

	    XRecolorCursor(W_Display, *cursor, &f, &b);
	}
    } else
    if (0 == strcmp("bomb here", adefault)) {
	static Cursor bomb_here = 0;
	if (bomb_here == 0) {
	    bomb_here = make_cursor(cross_bits, crossmask_bits,
				    cross_width, cross_height,
				    cross_x_hot, cross_y_hot);
	}
	*cursor = bomb_here;
    }
}

int
checkMapped(char *name)
{
    char    buf[100];

    sprintf(buf, "%s.mapped", name);
    return (booleanDefault(buf, 0));
}

int
checkScrollSaveLines(char *name)
{
    char    buf[100];

    sprintf(buf, "%s.scrollSaveLines", name);
    return (intDefault(buf, scrollSaveLines));
}

int
checkBuffered(char *name)
{
    char    buf[100];

    sprintf(buf, "%s.buffered", name);


    /* defaults to on when in xpm mode for local window [BDyess] */
    /* don't turn it on unless asked when in OR mode [BDyess] */

    /*    if(xpm && !useOR && strcmp(name,"local") == 0) {*/
    /* buffer even if not in XPM mode */

    if(!useOR && strcmp(name,"local") == 0) {
      printf("Double Buffering automatically enabled.  Use -O if your machine can't handle it.\n");
      return (booleanDefault(buf, 1));
    } else {
      return (booleanDefault(buf, 0));
    }
}

void
W_WarpPointer(W_Window window, int x, int y)
{
    static int warped_from_x = 0, warped_from_y = 0;

    if (window == NULL) {
	if (W_in_message) {
	    XWarpPointer(W_Display, None, W_Root, 0, 0, 0, 0, warped_from_x, warped_from_y);
	    W_in_message = 0;
	}
    } else {
	findMouse(&warped_from_x, &warped_from_y);
	XWarpPointer(W_Display, None, W_Void2Window(window)->window, 0, 0, 0, 0, 0, 0);
	W_in_message = 1;
    }
}

static void
findMouse(int *x, int *y)
{
    Window  theRoot, theChild;
    int     wX, wY, rootX, rootY, ErrorVal;
    unsigned int wButtons;

    ErrorVal = XQueryPointer(W_Display, W_Root, &theRoot, &theChild, &rootX, &rootY, &wX, &wY, &wButtons);
    if (ErrorVal == True) {
	*x = wX;
	*y = wY;
    } else {
	*x = 0;
	*y = 0;
    }
}

int
findMouseInWin(int *x, int *y, W_Window window)
{
    Window  theRoot, theChild;
    int     wX, wY, rootX, rootY, ErrorVal;
    unsigned int wButtons;
    struct window *win = W_Void2Window(window);
    Window  thisWin = win->window;

    ErrorVal = XQueryPointer(W_Display, thisWin, &theRoot, &theChild,
			   &rootX, &rootY, &wX, &wY, &wButtons);
    if (ErrorVal == True) {
	/*
	   if it's in the window we specified then the values returned should
	   be within the with and height of the window
	*/
	if (wX <= win->width && wY <= win->height) {
	    *x = wX;
	    *y = wY;
	    return 1;
	}
    }
    *x = 0;
    *y = 0;
    return 0;
}

void
W_Flush(void)
{
    XFlush(W_Display);
}

void
W_TranslatePoints(W_Window window, int *x, int *y)
{
    struct window *win;
    win = W_Void2Window(window);

    if (win->type == WIN_TEXT) {
	*y = (*y - MENU_PAD) / W_Textheight;
	*x = (*x - MENU_PAD) / W_Textwidth;
    }
    return;
}

void
W_ResizeWindow(W_Window window, int neww, int newh)	/* TSH 2/93 */
{
    struct window *win = W_Void2Window(window);

    XResizeWindow(W_Display, win->window, 
                  (unsigned int) neww, 
                  (unsigned int) newh);
    /* resize the border to match [BDyess] */
    XResizeWindow(W_Display, W_Void2Window(win->borderwin)->window, 
                  (unsigned int) neww + win->border * 2,
                  (unsigned int) newh + win->border * 2);
}

void
W_ResizeMenu(W_Window window, int neww, int newh)/* TSH 2/93 */
{
    W_ResizeWindow(window, neww * W_Textwidth + WIN_EDGE * 2,
	      newh * (W_Textheight + MENU_PAD * 2) + (newh - 1) * MENU_BAR);
}

void
W_ResizeText(W_Window window, int neww, int newh)/* TSH 2/93 */
{
    W_ResizeWindow(window, neww * W_Textwidth + WIN_EDGE * 2,
		   newh * W_Textheight + MENU_PAD * 2);
}

void
W_Deiconify(W_Window window)
{
    struct window *win;
    win = W_Void2Window(window);
    /* according to ICCCM 4.1.4, this is how you deiconify a window. */
    XMapWindow(W_Display, win->window);
}

void
W_DrawShield(W_Window window, int centerx, int centery, 
             unsigned int diameter, W_Color color)
{
    XDrawArc(W_Display, W_Void2Window(window)->drawable, 
    	     colortable[color].contexts[0], 
	     (int)(centerx - diameter / 2), 
	     (int)(centery - diameter / 2), 
	     diameter - 1, diameter - 1, 0, 360 * 64);
}

/* synchronize the X server with the client [BDyess] */
void 
W_Sync(void)
{
  XSync(W_Display,False);
}

/* needed by newstats.c [BDyess] */
void
W_WriteArc(int filled, W_Window window, int x, int y, int width, int height,
           int angle1, int angle2, W_Color color)
{
    static GC pen = 0;
    struct window *win;
    unsigned long valuemask = 0;    /* Ignore XGCvalues and use defaults */
    XGCValues values;

    win = W_Void2Window(window);

    if(pen == 0) pen = XCreateGC(W_Display, W_Root, valuemask, &values);
    XSetForeground(W_Display, pen, colortable[color].pixelValue);
    if (filled) {
       XFillArc(W_Display, win->window, pen, x - (int)(width/2),
             y - (int)(height/2), width, height, angle1 * 64, angle2 * 64);
    } else {
       XDrawArc(W_Display, win->window, pen, x - (int)(width/2),
             y - (int)(height/2), width, height, angle1 * 64, angle2 * 64);
    }

    return;
}
