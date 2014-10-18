/*LINTLIBRARY*/
/*
 * Mini-Toolbox
 *
 * David Harrison
 * University of California, Berkeley
 * 1988, 1989
 *
 * This file contains routines which implement simple display widgets
 * which can be used to construct simple dialog boxes.
 * A mini-toolbox has been written here (overkill but I didn't
 * want to use any of the standards yet -- they are too unstable).
 */

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h>  /* for strcpy */
#include <stdio.h>
#include "xtb.h"

extern void abort();
extern XContext XrmUniqueQuark();

#define MAXKEYS		10

#ifdef __STDC__
#define FNPTR(fname, rtn, args)	rtn (*fname)args
#include <stdarg.h>
#define VARARGS(func, rtn, args)	rtn func args
#else
#define FNPTR(fname, rtn, args)	rtn (*fname)()
#include <varargs.h>
#define VARARGS(func, rtn, args)	/*VARARGS1*/ rtn func(va_alist) va_dcl
#endif

struct h_info {
    FNPTR(func, xtb_hret, (XEvent *evt, xtb_data info));	/* Function to call */
    xtb_data info;		/* Additional info  */
};

static Display *t_disp;		/* Display          */
static int t_scrn;		/* Screen           */

static unsigned long norm_pix;	/* Foreground color */
static unsigned long back_pix;	/* Background color */

static XFontStruct *norm_font;	/* Normal font      */

extern char *Malloc();

#define STRDUP(str)	(strcpy(Malloc((unsigned) (strlen(str)+1)), (str)))
extern void Free();


/*
 * Sets default parameters used by the mini-toolbox.
 */
void xtb_init(Display *disp, int scrn, unsigned long foreground, unsigned long background, XFontStruct *font)
{
    t_disp = disp;
    t_scrn = scrn;
    norm_pix = foreground;
    back_pix = background;
    norm_font = font;
}

#define gray_width 32
#define gray_height 32
static char gray_bits[] = {
    0x55, 0x55, 0x55, 0x55, 0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 0xaa, 0xaa, 0xaa, 0xaa,
    0x55, 0x55, 0x55, 0x55, 0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 0xaa, 0xaa, 0xaa, 0xaa,
    0x55, 0x55, 0x55, 0x55, 0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 0xaa, 0xaa, 0xaa, 0xaa,
    0x55, 0x55, 0x55, 0x55, 0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 0xaa, 0xaa, 0xaa, 0xaa,
    0x55, 0x55, 0x55, 0x55, 0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55,
    0xaa, 0xaa, 0xaa, 0xaa, 0x55, 0x55, 0x55, 0x55, 0xaa, 0xaa, 0xaa, 0xaa,
    0x55, 0x55, 0x55, 0x55, 0xaa, 0xaa, 0xaa, 0xaa
};

static  Pixmap make_stipple(Display *disp, Drawable able, char *bits, unsigned int width, int height)
{
	unsigned int     w,
					 h;
	Pixmap  stipple;
	int     bitmap_pad;
	GC      image_gc;
	XGCValues image_vals;
	XImage *stip_image;

	if (!XQueryBestStipple(disp, able, width, height, &w, &h)) {
		/* Can't query best stipple */
		return (Pixmap) 0;
	}

	if ((w > width) || (h > height)) {
		/* Given pattern is too small */
		return (Pixmap) 0;
	}

	if (!(stipple = XCreatePixmap(disp, able, w, h, 1))) {
		/* Can't create pixmap image */
		return (Pixmap) 0;
	}

	bitmap_pad = 8;

	stip_image = XCreateImage(disp, DefaultVisual(disp, DefaultScreen(disp)), 1, XYPixmap, 0, bits, width, height, bitmap_pad, 0);

	if (!stip_image) {
		return (Pixmap) 0;
	}

	image_gc = XCreateGC(disp, stipple, (unsigned long) 0, &image_vals);
	XPutImage(disp, stipple, image_gc, stip_image, 0, 0, 0, 0, w, h);

	return stipple;
}


/*
 * Returns a gray pixmap suitable for stipple fill.
 */
static Pixmap gray_map(Window win)
{
	static Pixmap map = (Pixmap) 0;

	if (!map) {
		map = make_stipple(t_disp, win, gray_bits, gray_width, gray_height);
	}
	return map;
}

static GC set_gc(Window win, unsigned long fg, unsigned long bg, Font font, int gray_p)
/*
 * Sets and returns the fields listed above in a global graphics context.  If
 * graphics context does not exist,  it is created.
 */
{
	static GC t_gc = (GC)0;
	XGCValues gcvals;
	unsigned long gcmask;

	gcvals.foreground = fg;
	gcvals.background = bg;
	gcvals.font = font;
	gcvals.stipple = gray_map(win);
	gcvals.fill_style = (gray_p ? FillStippled : FillSolid);

	gcmask = GCForeground | GCBackground | GCFont | GCFillStyle | GCStipple;

	if (t_gc == (GC)0) {
		t_gc = XCreateGC(t_disp, win, gcmask, &gcvals);
	}
	else {
		XChangeGC(t_disp, t_gc, gcmask, &gcvals);
	}
	return t_gc;
}


static XContext h_context = (XContext)0;

/*
 * Associates the event handling function `func' with the window `win'.
 * Additional information `info' will be passed to `func'.  The routine should
 * return one of the return codes given above.
 */
void xtb_register(Window win, FNPTR(func, xtb_hret, (XEvent *evt, xtb_data info)), xtb_data info)
{
	struct h_info *new_info;

	if (h_context == (XContext)0) {
		h_context = XUniqueContext();
	}

	new_info = (struct h_info *)Malloc(sizeof(struct h_info));
	new_info->func = func;
	new_info->info = info;
	XSaveContext(t_disp, win, h_context, (caddr_t) new_info);
}

/*
 * Returns the associated data with window `win'.
 */
xtb_data xtb_lookup(Window win)
{
	xtb_data data;

	if (!XFindContext(t_disp, win, h_context, (XPointer*) &data)) {
		return ((struct h_info *)data)->info;
	}
	else {
		return (xtb_data)0;
	}
}

/*
 * Dispatches an event to a handler if its ours.  Returns one of the return
 * codes given above (XTB_NOTDEF, XTB_HANDLED, or XTB_STOP).
 */
xtb_hret xtb_dispatch(XEvent *evt)
{
	struct h_info *info;

	if (!XFindContext(t_disp, evt->xany.window, h_context, (caddr_t *)&info)) {
		if (info->func) {
			return (*info->func)(evt, info->info);
		}
		else {
			return XTB_NOTDEF;
		}
	}
	else {
		return XTB_NOTDEF;
	}
}

/*
 * Removes `win' from the dialog association table.  `info' is returned to
 * allow the user to delete it (if desired).  Returns a non-zero status if the
 * association was found and properly deleted.
 */
int xtb_unregister(Window win, xtb_data *info)
{
	struct h_info *hi;

	if (!XFindContext(t_disp, win, h_context, (caddr_t *) & hi)) {
		(void)XDeleteContext(t_disp, win, h_context);
		*info = hi->info;
		Free((char *)hi);
		return 1;
	}
	else {
		return 0;
	}
}


#define BT_HPAD	3
#define BT_VPAD	2
#define BT_LPAD	3
#define BT_BRDR	1

struct b_info {
    FNPTR(func, xtb_hret, (Window win, int state, xtb_data val));
    /* Function to call */
    char   *text;		/* Text of button   */
    int     flag;		/* State of button  */
    int     na;			/* Non-zero if not active */
    int     line_y,
            line_w;		/* Entry/Exit line  */
    xtb_data val;		/* User defined info */
};

/*
 * Draws a button window
 */
static void bt_draw(Window win, struct b_info *ri)
{
	if (ri->flag) {
		XDrawImageString(t_disp, win, set_gc(win, back_pix, norm_pix, norm_font->fid, 0), BT_HPAD, BT_VPAD + norm_font->ascent, ri->text, strlen(ri->text));
	}
	else {
		XDrawImageString(t_disp, win, set_gc(win, norm_pix, back_pix, norm_font->fid, 0), BT_HPAD, BT_VPAD + norm_font->ascent, ri->text, strlen(ri->text));
	}

	if (ri->na) {
		Window  root;
		int     x,
				y;
		unsigned int w,
					 h,
					 b,
					 d;

		XGetGeometry(t_disp, win, &root, &x, &y, &w, &h, &b, &d);

		XFillRectangle(t_disp, win, set_gc(win, (ri->flag ? norm_pix : back_pix), back_pix, norm_font->fid, 1), 0, 0, w, h);
	}
}

/*
 * Draws a status line beneath the text to indicate the user has moved into the
 * button.
 */
static void bt_line(Window win, struct b_info *ri, unsigned long pix)
{
	XDrawLine(t_disp, win, set_gc(win, pix, back_pix, norm_font->fid, ri->na), BT_HPAD, ri->line_y, BT_HPAD + ri->line_w, ri->line_y);
}

/*
 * Handles button events.
 */
static  xtb_hret bt_h(XEvent *evt, xtb_data info)
{
	Window  win = evt->xany.window;
	struct b_info *ri = (struct b_info *)info;
	xtb_hret rtn = 0;

	switch (evt->type) {
		case Expose:
			bt_draw(win, ri);
			rtn = XTB_HANDLED;
			break;
		case EnterNotify:
			if (!ri->na) {
				bt_line(win, ri, norm_pix);
			}

			rtn = XTB_HANDLED;
			break;
		case LeaveNotify:
			if (!ri->na) {
				bt_line(win, ri, back_pix);
			}

			rtn = XTB_HANDLED;
			break;
		case ButtonPress:
			/* Nothing - just wait for button up */
			rtn = XTB_HANDLED;
			break;
		case ButtonRelease:
			if (!ri->na) {
				rtn = (*ri->func) (win, ri->flag, ri->val);
			}

			break;
		default:
			rtn = XTB_NOTDEF;
	}
	return rtn;
}

/*
 * Makes a new button under `win' with the text `text'.  The window, size, and
 * position of the button are returned in `frame'.  The initial position is
 * always (0, 0).  When the button is pressed,  `func' will be called with the
 * button window,  the current state of the button,  and `val'.  It is up to
 * `func' to change the state of the button (if desired).  The routine should
 * return XTB_HANDLED normally and XTB_STOP if the dialog should stop.  The
 * window will be automatically mapped.
 */
void xtb_bt_new(Window win, char *text, FNPTR(func, xtb_hret, (Window, int, xtb_data)), xtb_data val, xtb_frame *frame)
{
	XCharStruct bb;
	struct b_info *info;
	int     dir,
			ascent,
			descent;

	XTextExtents(norm_font, text, strlen(text), &dir, &ascent, &descent, &bb);

	frame->width = bb.width + 2 * BT_HPAD;
	frame->height = norm_font->ascent + norm_font->descent + BT_VPAD + BT_LPAD;

	frame->x_loc = frame->y_loc = 0;

	frame->win = XCreateSimpleWindow(t_disp, win, frame->x_loc, frame->y_loc, frame->width, frame->height, BT_BRDR, norm_pix, back_pix);

	XSelectInput(t_disp, frame->win, ExposureMask | ButtonPressMask | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask);

	info = (struct b_info *) Malloc(sizeof(struct b_info));

	info->func = func;
	info->text = STRDUP(text);
	info->flag = 0;
	info->na = 0;
	info->val = val;
	info->line_y = frame->height - 2;
	info->line_w = frame->width - 2 * BT_HPAD;

	xtb_register(frame->win, bt_h, (xtb_data) info);
	XMapWindow(t_disp, frame->win);
	frame->width += (2 * BT_BRDR);
	frame->height += (2 * BT_BRDR);
}

/*
 * Returns the state of button `win'.  If provided,  the button specific info
 * is returned in `info' and the active state of the button is returned in
 * `na'.
 */
int xtb_bt_get(Window win, xtb_data *stuff, int *na)
{
	struct b_info *info = (struct b_info *)xtb_lookup(win);

	if (stuff) {
		*stuff = info->val;
	}

	if (na) {
		*na = info->na;
	}

	return info->flag;
}

/*
 * Changes the value of a button and returns the new state.  The button is
 * drawn.  If set,  the button specific info will be set to `info'.  This
 * doesn't allow you to set the state to zero, but that may not be important.
 * If `na' is non-zero,  the button will be deactivated (no activity will be
 * allowed). The change in button appearance will be immediate.
 */
int xtb_bt_set(Window win, int val, xtb_data stuff, int na)
{
	struct b_info *info = (struct b_info *) xtb_lookup(win);

	info->flag = (val != 0);
	info->na = (na != 0);

	if (stuff) {
		info->val = stuff;
	}

	bt_draw(win, info);
	XFlush(t_disp);

	return info->flag;
}

/*
 * Deletes the button `win' and returns the user defined information in `info'
 * for destruction.
 */
void xtb_bt_del(Window win, xtb_data *info)
{
	struct b_info *bi;

	if (xtb_unregister(win, (xtb_data *)&bi)) {
		*info = bi->val;
		Free((char *)bi->text);
		Free((char *)bi);
		XDestroyWindow(t_disp, win);
	}
}


#define BR_XPAD		2
#define BR_YPAD		2
#define BR_INTER	2

struct br_info {
    Window  main_win;		/* Main button row    */
    int     which_one;		/* Which button is on */
    int     btn_cnt;		/* How many buttons   */
            FNPTR(func, xtb_hret, (Window win, int prev, int this, xtb_data val));
    xtb_data val;		/* User data          */
    Window *btns;		/* Button windows     */
};

/*
 * This handles events for button rows.  When a button is pressed, it turns off
 * the button selected in `which_one' and turns itself on.
 */
static xtb_hret br_h(Window win, int val, xtb_data info)
{
	struct br_info *real_info = (struct br_info *)info;
	int     i,
			prev;

	prev = real_info->which_one;

	if ((prev >= 0) && (prev < real_info->btn_cnt)) {
		(void) xtb_bt_set(real_info->btns[prev], 0, (xtb_data)0, 0);
	}

	for (i = 0; i < real_info->btn_cnt; i++) {

		if (win == real_info->btns[i]) {
			real_info->which_one = i;
			break;
		}
	}

	(void)xtb_bt_set(win, 1, (xtb_data)0, 0);

	/* Callback */
	if (real_info->func) {
		return (*real_info->func) (real_info->main_win, prev, real_info->which_one, real_info->val);
	}
	else {
		return XTB_HANDLED;
	}
}

/*
 * This routine makes a new row of buttons in the window `win' and returns a
 * frame containing all of these buttons.  These buttons are designed so that
 * only one of them will be active at a time.  Initially,  button `init' will
 * be activated (if init is less than zero,  none will be initially activated).
 * Whenever a button is pushed, `func' will be called with the button row
 * window,  the index of the previous button (-1 if none),  the index of the
 * current button,  and the user data, `val'.  The function is optional (if
 * zero,  no function will be called).  The size of the row is returned in
 * `frame'.  The window will be automatically mapped.  Initially,  the window
 * containing the buttons will be placed at 0,0 in the parent window.
 */
void xtb_br_new(Window win, int cnt, char *lbls[], int init, FNPTR(func, xtb_hret, (Window, int, int, xtb_data)), xtb_data val, xtb_frame *frame)
{
	struct br_info *info;
	xtb_frame sub_frame;
	int     i,
			x,
			y;

	frame->width = frame->height = 0;
	frame->x_loc = frame->y_loc = 0;
	frame->win = XCreateSimpleWindow(t_disp, win, 0, 0, 1, 1, 0, back_pix, back_pix);

	info = (struct br_info *)Malloc(sizeof(struct br_info));

	info->main_win = frame->win;
	info->btns = (Window *)Malloc((unsigned)(sizeof(Window) * cnt));
	info->btn_cnt = cnt;
	info->which_one = init;
	info->func = func;
	info->val = val;

	/* the handler is used simply to get information out */
	xtb_register(frame->win, (xtb_hret(*) ()) 0, (xtb_data) info);
	x = BR_XPAD;
	y = BR_YPAD;

	for (i = 0; i < cnt; i++) {
		xtb_bt_new(frame->win, lbls[i], br_h, (xtb_data) info, &sub_frame);
		info->btns[i] = sub_frame.win;
		XMoveWindow(t_disp, info->btns[i], x, y);
		x += (BR_INTER + sub_frame.width);

		if (sub_frame.height > frame->height) {
			frame->height = sub_frame.height;
		}

		if (i == init) {
			(void) xtb_bt_set(info->btns[i], 1, (xtb_data) 0, 0);
		}
	}

	frame->width = x - BR_INTER + BR_XPAD;
	frame->height += (2 * BR_YPAD);
	XResizeWindow(t_disp, frame->win, frame->width, frame->height);
	XMapWindow(t_disp, frame->win);
}

/*
 * This routine returns the index of the currently selected item of the button
 * row given by the window `win'.  Note:  no checking is done to make sure
 * `win' is a button row.
 */
int xtb_br_get(Window win)
{
    struct br_info *info = (struct br_info *)xtb_lookup(win);

    return info->which_one;
}

/*
 * Deletes a button row.  All resources are reclaimed.
 */
void xtb_br_del(Window win)
{
	struct br_info *info;
	int     i;

	if (xtb_unregister(win, (xtb_data *) & info)) {

		for (i = 0; i < info->btn_cnt; i++) {
			xtb_bt_del(info->btns[i], (xtb_data *) & info);
		}

		Free((char *)info->btns);
		Free((char *)info);

		XDestroyWindow(t_disp, win);
	}
}


/* Text widget */

#define TO_HPAD	1
#define TO_VPAD	1

struct to_info {
    char   *text;		/* Text to display */
    XFontStruct *ft;		/* Font to use     */
};

/*
 * Draws the text for a widget
 */
static void to_draw(Window win, struct to_info *ri)
{
    XDrawImageString(t_disp, win, set_gc(win, norm_pix, back_pix, ri->ft->fid, 0), TO_HPAD, TO_VPAD + ri->ft->ascent, ri->text, strlen(ri->text));
}

/*
 * Handles text widget events
 */
static  xtb_hret to_h(XEvent *evt, xtb_data info)
{
	Window  win = evt->xany.window;
	struct to_info *ri = (struct to_info *)info;

	switch (evt->type) {
		case Expose:
			to_draw(win, ri);
			return XTB_HANDLED;
		default:
			return XTB_NOTDEF;
	}
}

/*
 * Makes a new text widget under `win' with the text `text'.  The size of the
 * widget is returned in `w' and `h'.  The window is created and mapped at 0,0
 * in `win'.  The font used for the text is given in `ft'.
 */
void xtb_to_new(Window win, char *text, XFontStruct *ft, xtb_frame *frame)
{
	struct to_info *info;

	frame->width = XTextWidth(ft, text, strlen(text)) + 2 * TO_HPAD;
	frame->height = ft->ascent + ft->descent + 2 * TO_VPAD;

	frame->x_loc = frame->y_loc = 0;
	frame->win = XCreateSimpleWindow(t_disp, win, 0, 0, frame->width, frame->height, 0, back_pix, back_pix);

	XSelectInput(t_disp, frame->win, ExposureMask);

	info = (struct to_info *) Malloc(sizeof(struct to_info));

	info->text = STRDUP(text);
	info->ft = ft;

	xtb_register(frame->win, to_h, (xtb_data) info);
	XMapWindow(t_disp, frame->win);
}

/*
 * Deletes an output only text widget.
 */
void xtb_to_del(Window win)
{
	struct to_info *info;

	if (xtb_unregister(win, (xtb_data *) & info)) {
		if (info->text) {
			Free((char *)info->text);
		}

		Free((char *)info);
		XDestroyWindow(t_disp, win);
	}
}

/*
 * Input text widget
 */

#define TI_HPAD	2
#define TI_VPAD	2
#define TI_LPAD	3
#define TI_BRDR 2
#define TI_CRSP	1

struct ti_info {
    FNPTR(func, xtb_hret, (Window win, int ch, char *textcopy, xtb_data * val));
    /* Function to call   */
    int     maxlen;		/* Maximum characters */
    int     curidx;		/* Current index pos  */
    int     curxval;		/* Current draw loc   */
    char    text[MAXCHBUF];	/* Current text array */
    int     line_y,
            line_w;		/* Entry/Exit line    */
    int     focus_flag;		/* If on, we have focus */
    xtb_data val;		/* User info          */
};

/*
 * Returns the width of a string using XTextExtents.
 */
static int text_width(XFontStruct *font, char *str, int len)
{
	XCharStruct bb;
	int     dir,
			ascent,
			descent;

	XTextExtents(font, str, len, &dir, &ascent, &descent, &bb);
	return bb.width;
}

/*
 * Draws the cursor for the window.  Uses pixel `pix'.
 */
static void ti_cursor_on(Window win, struct ti_info *ri)
{
	XFillRectangle(t_disp, win, set_gc(win, norm_pix, back_pix, norm_font->fid, 0), ri->curxval + TI_HPAD + TI_CRSP, TI_VPAD, (ri->focus_flag ? 2 : 1), norm_font->ascent + norm_font->descent - 1);
}

/*
 * Draws the cursor for the window.  Uses pixel `pix'.
 */
static void ti_cursor_off(Window win, struct ti_info *ri)
{
    XFillRectangle(t_disp, win, set_gc(win, back_pix, back_pix, norm_font->fid, 0), ri->curxval + TI_HPAD + TI_CRSP, TI_VPAD, (ri->focus_flag ? 2 : 1), norm_font->ascent + norm_font->descent - 1);
}

/*
 * Draws the indicated text widget.  This includes drawing the text and cursor.
 * If `c_flag' is set,  the window will be cleared first.
 */
static void ti_draw(Window win, struct ti_info *ri, int c_flag)
{
	if (c_flag) {
		XClearWindow(t_disp, win);
	}

	/* Text */
	XDrawImageString(t_disp, win, set_gc(win, norm_pix, back_pix, norm_font->fid, 0), TI_HPAD, TI_VPAD + norm_font->ascent, ri->text, strlen(ri->text));

	/* Cursor */
	ti_cursor_on(win, ri);
}

/*
 * Draws a status line beneath the text in a text widget to indicate the user
 * has moved into the text field.
 */
static void ti_line(Window win, struct ti_info *ri, unsigned long pix)
{
    XDrawLine(t_disp, win, set_gc(win, pix, back_pix, norm_font->fid, 0), TI_HPAD, ri->line_y, TI_HPAD + ri->line_w, ri->line_y);
}

/* For debugging */
void focus_evt(XEvent *evt)
{
	switch (evt->xfocus.mode) {
		case NotifyNormal:
			printf("NotifyNormal");
			break;
		case NotifyGrab:
			printf("NotifyGrab");
			break;
		case NotifyUngrab:
			printf("NotifyUngrab");
			break;
	}

	printf(", detail = ");

	switch (evt->xfocus.detail) {
		case NotifyAncestor:
			printf("NotifyAncestor");
			break;
		case NotifyVirtual:
			printf("NotifyVirtual");
			break;
		case NotifyInferior:
			printf("NotifyInferior");
			break;
		case NotifyNonlinear:
			printf("NotifyNonLinear");
			break;
		case NotifyNonlinearVirtual:
			printf("NotifyNonLinearVirtual");
			break;
		case NotifyPointer:
			printf("NotifyPointer");
			break;
		case NotifyPointerRoot:
			printf("NotifyPointerRoot");
			break;
		case NotifyDetailNone:
			printf("NotifyDetailNone");
			break;
	}

	printf("\n");
}



/*
 * Handles text input events.
 */
static  xtb_hret ti_h(XEvent *evt, xtb_data info)
{
	Window  win = evt->xany.window;
	struct ti_info *ri = (struct ti_info *)info;
	char    keys[MAXKEYS],
			textcopy[MAXCHBUF];
	xtb_hret rtn = 0;
	int     nbytes,
			i;

	switch (evt->type) {
		case Expose:
			ti_draw(win, ri, 0);
			rtn = XTB_HANDLED;
			break;
		case KeyPress:
			nbytes = XLookupString(&evt->xkey, keys, MAXKEYS, (KeySym *) 0, (XComposeStatus *) 0);

			for (i = 0; i < nbytes; i++) {
				(void) strcpy(textcopy, ri->text);

				if ((rtn = (*ri->func) (win, (int) keys[i], textcopy, ri->val)) == XTB_STOP) {
					break;
				}
			}
			break;
		case FocusIn:
			focus_evt(evt);
			if (evt->xfocus.detail != NotifyPointer) {
				ti_cursor_off(win, ri);
				ri->focus_flag = 1;
				ti_cursor_on(win, ri);
			}

			break;
		case FocusOut:
			focus_evt(evt);
			if (evt->xfocus.detail != NotifyPointer) {
				ti_cursor_off(win, ri);
				ri->focus_flag = 0;
				ti_cursor_on(win, ri);
			}
			break;
		case EnterNotify:
			ti_line(win, ri, norm_pix);
			rtn = XTB_HANDLED;
			break;
		case LeaveNotify:
			ti_line(win, ri, back_pix);
			rtn = XTB_HANDLED;
			break;
		case ButtonPress:
			/* Wait for release */
			break;
		case ButtonRelease:
			/* Set input focus */
			XSetInputFocus(t_disp, win, RevertToParent, CurrentTime);
			break;
		default:
			rtn = XTB_NOTDEF;
			break;
	}
	return rtn;
}

/*
 * This routine creates a new editable text widget under `win' with the initial
 * text `text'.  The widget contains only one line of text which cannot exceed
 * `maxchar' characters.  The size of the widget is returned in `frame'.  Each
 * time a key is pressed in the window,  `func' will be called with the window,
 * the character, a copy of the text, and `val'.  The state of the widget can
 * be changed by the routines below.  May set window to zero if the maximum
 * overall character width (MAXCHBUF) is exceeded.
 */
void xtb_ti_new(Window win, char *text, int maxchar, FNPTR(func, xtb_hret, (Window, int, char *, xtb_data *)), xtb_data val, xtb_frame *frame)
{
	struct ti_info *info;

	if (maxchar >= MAXCHBUF) {
		frame->win = (Window) 0;
		return;
	}

	frame->width = XTextWidth(norm_font, "8", 1) * maxchar + 2 * TI_HPAD;
	frame->height = norm_font->ascent + norm_font->descent + TI_VPAD + TI_LPAD;

	frame->x_loc = frame->y_loc = 0;
	frame->win = XCreateSimpleWindow(t_disp, win, 0, 0, frame->width, frame->height, TI_BRDR, norm_pix, back_pix);

	XSelectInput(t_disp, frame->win, ExposureMask | KeyPressMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask | ButtonPressMask | ButtonReleaseMask);

	info = (struct ti_info *) Malloc(sizeof(struct ti_info));

	info->func = func;
	info->val = val;
	info->maxlen = maxchar;

	if (text) {
		(void)strcpy(info->text, text);
	}
	else {
		info->text[0] = '\0';
	}

	info->curidx = strlen(info->text);
	info->curxval = text_width(norm_font, info->text, info->curidx);
	info->line_y = frame->height - 2;
	info->line_w = frame->width - 2 * TI_HPAD;
	info->focus_flag = 0;

	xtb_register(frame->win, ti_h, (xtb_data) info);
	XMapWindow(t_disp, frame->win);

	frame->width += (2 * TI_BRDR);
	frame->height += (2 * TI_BRDR);
}

/*
 * This routine returns the information associated with text widget `win'.  The
 * text is filled into the passed buffer `text' which should be MAXCHBUF
 * characters in size.  If `val' is non-zero,  the user supplied info is
 * returned there.
 */
void xtb_ti_get(Window win, char text[MAXCHBUF], xtb_data *val)
{
	struct ti_info *info = (struct ti_info *)xtb_lookup(win);

	if (val) {
		*val = info->val;
	}

	(void)strcpy(text, info->text);
}

/*
 * This routine sets the text of a text widget.  The widget will be redrawn.
 * Note:  for incremental changes,  ti_ins and ti_dch should be used.  If `val'
 * is non-zero,  it will replace the user information for the widget.  The
 * widget is redrawn.  Will return zero if `text' is too long.
 */
int xtb_ti_set(Window win, char *text, xtb_data val)
{
	struct ti_info *info = (struct ti_info *)xtb_lookup(win);
	int     newlen;

	if (text) {
		if ((newlen = strlen(text)) >= info->maxlen) {
			return 0;
		}
	}
	else {
		newlen = 0;
	}

	info->curidx = newlen;

	if (text) {
		(void) strcpy(info->text, text);
	}
	else {
		info->text[0] = '\0';
	}

	info->curxval = text_width(norm_font, info->text, info->curidx);

	if (val) {
		info->val = val;
	}

	ti_draw(win, info, 1);

	return 1;
}

/*
 * Inserts the character `ch' onto the end of the text for `win'.  Will return
 * zero if there isn't any more room left.  Does all appropriate display
 * updates.
 */
int xtb_ti_ins(Window win, int ch)
{
	struct ti_info *info = (struct ti_info *)xtb_lookup(win);
	char    lstr[1];

	if (info->curidx >= info->maxlen - 1) {
		return 0;
	}

	info->text[info->curidx] = ch;
	info->text[info->curidx + 1] = '\0';

	/* Turn off cursor */
	ti_cursor_off(win, info);

	/* Text */
	lstr[0] = (char)ch;

	XDrawImageString(t_disp, win, set_gc(win, norm_pix, back_pix, norm_font->fid, 0), info->curxval + TI_HPAD, TI_VPAD + norm_font->ascent, lstr, 1);

	info->curidx += 1;
	info->curxval += text_width(norm_font, lstr, 1);
	ti_cursor_on(win, info);

	return 1;
}

/*
 * Deletes the character at the end of the text for `win'.  Will return zero if
 * there aren't any characters to delete.  Does all appropriate display
 * updates.
 */
int xtb_ti_dch(Window win)
{
	struct ti_info *info = (struct ti_info *)xtb_lookup(win);
	int     chw;

	if (info->curidx == 0) {
		return 0;
	}

	/* Wipe out cursor */
	ti_cursor_off(win, info);
	info->curidx -= 1;
	chw = text_width(norm_font, &(info->text[info->curidx]), 1);
	info->curxval -= chw;

	/* Wipe out character */
	XClearArea(t_disp, win, info->curxval + TI_HPAD, TI_VPAD, (unsigned int) chw + 1, (unsigned int) norm_font->ascent + norm_font->descent, False);

	info->text[info->curidx] = '\0';
	ti_cursor_on(win, info);

	return 1;
}

/*
 * Deletes an input text widget.  User defined data is returned in `info'.
 */
void xtb_ti_del(Window win, xtb_data *info)
{
	struct ti_info *ti;

	if (xtb_unregister(win, (xtb_data *)&ti)) {
		*info = ti->val;
		Free((char *)ti);
		XDestroyWindow(t_disp, win);
	}
}

/*
 * Simple colored output frame - usually used for drawing lines
 */

/*
 * This routine creates a new frame that displays a block of color whose size
 * is given by `width' and `height'.  It is usually used to draw lines.  No
 * user interaction is defined for the frame.  The color used is the default
 * foreground color set in xtb_init().
 */
void xtb_bk_new(Window win, unsigned width, unsigned height, xtb_frame *frame)
{
	frame->x_loc = frame->y_loc = 0;
	frame->width = width;
	frame->height = height;
	frame->win = XCreateSimpleWindow(t_disp, win, frame->x_loc, frame->y_loc, frame->width, frame->height, 0, norm_pix, norm_pix);
	XMapWindow(t_disp, frame->win);
}


/*
 * Deletes a block frame.
 */
void xtb_bk_del(Window win)
{
    XDestroyWindow(t_disp, win);
}

/*
 * Formatting support
 */

#define ERROR(msg) printf("%s\n", msg); abort();

/*
 * Returns formatting structure for a widget.
 */
xtb_fmt * xtb_w(xtb_frame *w)
{
    xtb_fmt *ret;

    ret = (xtb_fmt *)Malloc((unsigned)sizeof(xtb_fmt));
    ret->wid.type = W_TYPE;
    ret->wid.w = w;

    return ret;
}

/*
 * Builds a horizontal structure
 */
VARARGS(xtb_hort, xtb_fmt *, (xtb_just just, int padding, int interspace,...))
{
	va_list ap;
	xtb_fmt *ret,
			*val;

#ifdef __STDC__
	va_start(ap, interspace);
#else
	xtb_just just;
	int     padding,
			interspace;

	va_start(ap);
	just = va_arg(ap, xtb_just);
	padding = va_arg(ap, int);
	interspace = va_arg(ap, int);
#endif

	ret = (xtb_fmt *) Malloc((unsigned) sizeof(xtb_fmt));
	ret->align.type = A_TYPE;
	ret->align.dir = HORIZONTAL;
	ret->align.just = just;
	ret->align.padding = padding;
	ret->align.interspace = interspace;

	/* Build array of incoming xtb_fmt structures */
	ret->align.ni = 0;

	while ((val = va_arg(ap, xtb_fmt *)) != (xtb_fmt *) 0) {

		if (ret->align.ni < MAX_BRANCH) {
			ret->align.items[ret->align.ni] = val;
			ret->align.ni++;
		}
		else {
			ERROR("too many branches\n");
		}
	}
	return ret;
}


/*
 * Builds a vertical structure
 */
VARARGS(xtb_vert, xtb_fmt *, (xtb_just just, int padding, int interspace,...))
{
	va_list ap;
	xtb_fmt *ret,
			*val;

#ifdef __STDC__
	va_start(ap, interspace);
#else
	xtb_just just;
	int     padding,
			interspace;

	va_start(ap);
	just = va_arg(ap, xtb_just);
	padding = va_arg(ap, int);
	interspace = va_arg(ap, int);
#endif
	ret = (xtb_fmt *)Malloc((unsigned)sizeof(xtb_fmt));
	ret->align.type = A_TYPE;
	ret->align.dir = VERTICAL;
	ret->align.just = just;
	ret->align.padding = padding;
	ret->align.interspace = interspace;

	/* Build array of incoming xtb_fmt structures */
	ret->align.ni = 0;

	while ((val = va_arg(ap, xtb_fmt *)) != (xtb_fmt *) 0) {

		if (ret->align.ni < MAX_BRANCH) {
			ret->align.items[ret->align.ni] = val;
			ret->align.ni++;
		}
		else {
			ERROR("too many branches\n");
		}
	}
	return ret;
}

/*
 * Sets all position fields of widgets in `def' to x,y.
 */
static void xtb_fmt_setpos(xtb_fmt *def, int x, int y)
{
	int     i;

	switch (def->type) {
		case W_TYPE:
			def->wid.w->x_loc = x;
			def->wid.w->y_loc = y;
			break;
		case A_TYPE:
			for (i = 0; i < def->align.ni; i++) {
				xtb_fmt_setpos(def->align.items[i], x, y);
			}
			break;
		default:
			ERROR("bad type");
	}
}

/*
 * Adds the offset specified to all position fields of widgets in `def'.
 */
static void xtb_fmt_addpos(xtb_fmt *def, int x, int y)
{
	int     i;

	switch (def->type) {
		case W_TYPE:
			def->wid.w->x_loc += x;
			def->wid.w->y_loc += y;
			break;
		case A_TYPE:
			for (i = 0; i < def->align.ni; i++) {
				xtb_fmt_addpos(def->align.items[i], x, y);
			}
			break;
		default:
			ERROR("bad type");
	}
}

/*
 * Formats items horizontally subject to the widths and heights of the items
 * passed.
 */
static void xtb_fmt_hort(int nd, xtb_fmt *defs[], unsigned int widths[], unsigned int heights[], xtb_just just, int pad, int inter, unsigned int *rw, unsigned int *rh)
{
	int     i;
	int     max_height = 0;
	int     tot_width = 0;
	int     xspot;

	/* Find parameters */
	for (i = 0; i < nd; i++) {
		if (heights[i] > max_height) {
			max_height = heights[i];
		}

		tot_width += widths[i];
	}

	/* Place items -- assumes center justification */
	xspot = pad;

	for (i = 0; i < nd; i++) {
		switch (just) {
			case XTB_TOP:
				xtb_fmt_addpos(defs[i], xspot, pad);
				break;
			case XTB_BOTTOM:
				xtb_fmt_addpos(defs[i], xspot, max_height - heights[i] + pad);
				break;
			case XTB_CENTER:
			default:
				/* Everyone else center */
				xtb_fmt_addpos(defs[i], xspot, (max_height - heights[i]) / 2 + pad);
				break;
		}

		xspot += (widths[i] + inter);
	}

	/* Figure out resulting size */
	*rw = tot_width + (nd - 1) * inter + (2 * pad);
	*rh = max_height + (2 * pad);
}


/*
 * Formats items vertically subject to the widths and heights of the items
 * passed.
 */
static void xtb_fmt_vert(int nd, xtb_fmt *defs[], unsigned int widths[], unsigned int heights[], xtb_just just, int pad, int inter, unsigned int *rw, unsigned int *rh)
{
	int     i;
	int     max_width = 0;
	int     tot_height = 0;
	int     yspot;

	/* Find parameters */
	for (i = 0; i < nd; i++) {
		if (widths[i] > max_width) {
			max_width = widths[i];
		}

		tot_height += heights[i];
	}

	/* Place items -- assumes center justification */
	yspot = pad;

	for (i = 0; i < nd; i++) {
		switch (just) {
			case XTB_LEFT:
				xtb_fmt_addpos(defs[i], pad, yspot);
				break;
			case XTB_RIGHT:
				xtb_fmt_addpos(defs[i], max_width - widths[i] + pad, yspot);
				break;
			case XTB_CENTER:
			default:
				/* Everyone else center */
				xtb_fmt_addpos(defs[i], (max_width - widths[i]) / 2 + pad, yspot);
				break;
		}

		yspot += (heights[i] + inter);
	}

	/* Figure out resulting size */
	*rw = max_width + (2 * pad);
	*rh = tot_height + (nd - 1) * inter + (2 * pad);
}

/*
 * Recursive portion of formatter
 */
static void xtb_fmt_top(xtb_fmt *def, unsigned *w, unsigned *h)
{
	unsigned widths[MAX_BRANCH];
	unsigned heights[MAX_BRANCH];
	int     i;

	switch (def->type) {
		case A_TYPE:
			/* Formatting directive */
			/* place children and determine sizes */
			for (i = 0; i < def->align.ni; i++) {
				xtb_fmt_top(def->align.items[i], &(widths[i]), &(heights[i]));
			}
			/* now format based on direction */
			switch (def->align.dir) {
				case HORIZONTAL:
					xtb_fmt_hort(def->align.ni, def->align.items, widths, heights, def->align.just, def->align.padding, def->align.interspace, w, h);
					break;
				case VERTICAL:
					xtb_fmt_vert(def->align.ni, def->align.items, widths, heights, def->align.just, def->align.padding, def->align.interspace, w, h);
					break;
				default:
					ERROR("bad direction");
			}
			break;
		case W_TYPE:
			/* Simple widget - return size */
			*w = def->wid.w->width;
			*h = def->wid.w->height;
			break;
		default:
			ERROR("bad type");
	}
}

#ifdef DEBUG
/*
 * Dumps formatting structure for debugging purposes.
 */
static void xtb_fmt_debug(xtb_fmt *def)
{
	int     i;

	switch (def->type) {
		case W_TYPE:
			printf("%d %d %d %d\n", def->wid.w->x_loc, def->wid.w->y_loc, def->wid.w->width, def->wid.w->height);
			break;
		case A_TYPE:
			for (i = 0; i < def->align.ni; i++) {
				xtb_fmt_debug(def->align.items[i]);
			}
			break;
		default:
			ERROR("bad type");
	}
}
#endif

/*
 * Actually does formatting
 */
xtb_fmt *xtb_fmt_do(xtb_fmt *def, unsigned *w, unsigned *h)
{
	/* First zero out all positions */
	xtb_fmt_setpos(def, 0, 0);

	/* Now call recursive portion */
	xtb_fmt_top(def, w, h);

#ifdef DEBUG
	xtb_fmt_debug(def);
#endif
	return def;
}

/*
 * Frees resources associated with formatting routines
 */
void xtb_fmt_free(xtb_fmt *def)
{
	int     i;

	if (def->type == A_TYPE) {
		for (i = 0; i < def->align.ni; i++) {
			xtb_fmt_free(def->align.items[i]);
		}
	}
	Free((char *)def);
}

/*
 * Moves frames to the location indicated in the frame structure for each item.
 */
void xtb_mv_frames(int nf, xtb_frame frames[])
{
	int     i;

	for (i = 0; i < nf; i++) {
		XMoveWindow(t_disp, frames[i].win, frames[i].x_loc, frames[i].y_loc);
	}
}
