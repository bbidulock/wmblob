
/* jlh */

/*
    this file is part of: wmblob

    this file has been modified to better fit wmblob's needs.
    I don't recommend to use it for new dockapps.  use the original
    dockapp.c instead.
*/

/*
 * Copyright (c) 1999 Alfredo K. Kojima
 * Copyright (c) 2001, 2002 Seiichi SATO <ssato@sh.rim.or.jp>
 * Copyright (c) 2004 Jean-Luc Herren <jlh-at-gmx-dot-ch>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 * This code is based on libdockapp-0.4.0
 * modified by Seiichi SATO <ssato@sh.rim.or.jp>
 */

#include "mydockapp.h"
#include <sys/time.h>

#define WINDOWED_SIZE_W 64
#define WINDOWED_SIZE_H 64

/* global */
Display	*display;
int dockapp_iswindowed = 0;
int dockapp_isbrokenwm = 0;
int color_map;

/* private */
Window window;
Window icon_window;
static GC gc = NULL;
static int depth;
static Atom delete_win;
static int width, height;
static int offset_w, offset_h;
static int screen;
static Visual *visual;

void dockapp_open_window(char *display_specified, char *appname,
		    unsigned w, unsigned h, int argc, char **argv)
{
	XClassHint *classhint;
	XWMHints *wmhints;
	XTextProperty title;
	XSizeHints sizehints;
	Window root;
	int ww, wh;

	/* Open Connection to X Server */

	display = XOpenDisplay(display_specified);

	if (!display)
	{
		fprintf(stderr, "%s: could not open display %s!\n",
				argv[0],
				XDisplayName(display_specified));
		exit(1);
	}

	root = DefaultRootWindow(display);
	screen = DefaultScreen(display);
	color_map = DefaultColormap(display, screen);

	width = w;
	height = h;

	if (dockapp_iswindowed)
	{
		offset_w = (WINDOWED_SIZE_W - w) / 2;
		offset_h = (WINDOWED_SIZE_H - h) / 2;
		ww = WINDOWED_SIZE_W;
		wh = WINDOWED_SIZE_H;
	}
	else
	{
		offset_w = offset_h = 0;
		ww = w;
		wh = h;
	}

	/* Create windows */

	icon_window = XCreateSimpleWindow(display, root, 0, 0, ww, wh, 0, 0, 0);

	if (dockapp_isbrokenwm)
		window = XCreateSimpleWindow(
			display, root, 0, 0, ww, wh, 0, 0, 0);
	else
		window = XCreateSimpleWindow(
			display, root, 0, 0, 1, 1, 0, 0, 0);

	/* Set ClassHint */

	classhint = XAllocClassHint();

	if (!classhint)
	{
		fprintf(stderr, "%s: can't allocate memory for wm hints!\n",
				argv[0]);
		exit(1);
	}

	classhint->res_class = "DockApp";
	classhint->res_name = appname;
	XSetClassHint(display, window, classhint);
	XFree(classhint);

	/* Set WMHints */

	wmhints = XAllocWMHints();

	if (!wmhints)
	{
		fprintf(stderr, "%s: can't allocate memory for wm hints!\n",
				argv[0]);
		exit(1);
	}

	wmhints->flags = IconWindowHint | WindowGroupHint;

	if (!dockapp_iswindowed)
	{
		wmhints->flags |= StateHint;
		wmhints->initial_state = WithdrawnState;
	}

	wmhints->window_group = window;
	wmhints->icon_window = icon_window;
	XSetWMHints(display, window, wmhints);
	XFree(wmhints);

	/* Set WM Protocols */

	delete_win = XInternAtom(display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols (display, icon_window, &delete_win, 1);

	/* Set Size Hints */

	sizehints.flags = USSize;

	if (!dockapp_iswindowed)
	{
		sizehints.flags |= USPosition;
		sizehints.x = sizehints.y = 0;
	}
	else
	{
		sizehints.flags |= PMinSize | PMaxSize;
		sizehints.min_width = sizehints.max_width = WINDOWED_SIZE_W;
		sizehints.min_height = sizehints.max_height = WINDOWED_SIZE_H;
	}

	sizehints.width = ww;
	sizehints.height = wh;
	XSetWMNormalHints(display, icon_window, &sizehints);

	/* Set WindowTitle for AfterStep Wharf */
	XStringListToTextProperty(&appname, 1, &title);
	XSetWMName(display, window, &title);
	XSetWMName(display, icon_window, &title);

	/* Set Command to start the app so it can be docked properly */
	XSetCommand(display, window, argv, argc);

	depth = DefaultDepth(display, DefaultScreen(display));
	gc = DefaultGC(display, DefaultScreen(display));

	XFlush(display);
}


void dockapp_set_eventmask(long mask)
{
	XSelectInput(display, icon_window, mask);
	XSelectInput(display, window, mask);
}


void dockapp_show(void)
{
	if (!dockapp_iswindowed)
		XMapRaised(display, window);
	else
		XMapRaised(display, icon_window);

	XFlush(display);
}


XImage *dockapp_create_ximage(int w, int h)
{
	XImage *ximage;

	ximage = XCreateImage(display, visual, depth,
			ZPixmap, 0, NULL, w, h, 32, 0);
	ximage->data = (char *)malloc(ximage->bytes_per_line * h);

	return(ximage);
}

void dockapp_destroy_ximage(XImage *ximage)
{
	XDestroyImage(ximage);
}


void dockapp_setshape(Pixmap mask, int x_ofs, int y_ofs)
{
	XShapeCombineMask(display, icon_window, ShapeBounding, -x_ofs, -y_ofs,
			mask, ShapeSet);
	XShapeCombineMask(display, window, ShapeBounding, -x_ofs, -y_ofs,
			mask, ShapeSet);
	XFlush(display);
}


void dockapp_copy2window (XImage *src)
{
	if (dockapp_isbrokenwm)
		XPutImage(display, window, gc, src,
			0, 0, 0, 0, width, height);
	else
		XPutImage(display, icon_window, gc, src,
			0, 0, 0, 0, width, height);

	XFlush(display);
}


Bool dockapp_nextevent_or_timeout(XEvent *event, unsigned long miliseconds)
{
    struct timeval timeout;
    fd_set rset;

    XSync(display, False);
    if (XPending(display)) {
	XNextEvent(display, event);
	return True;
    }

    timeout.tv_sec = miliseconds / 1000;
    timeout.tv_usec = (miliseconds % 1000) * 1000;

    FD_ZERO(&rset);
    FD_SET(ConnectionNumber(display), &rset);
    if (select(ConnectionNumber(display)+1, &rset, NULL, NULL, &timeout) > 0) {
	XNextEvent(display, event);
	if (event->type == ClientMessage) {
	    if (event->xclient.data.l[0] == delete_win) {
		XDestroyWindow(display,event->xclient.window);
		XCloseDisplay(display);
		exit(0);
	    }
	}
	if (dockapp_iswindowed) {
		event->xbutton.x -= offset_w;
		event->xbutton.y -= offset_h;
	}
	return True;
    }

    return False;
}

