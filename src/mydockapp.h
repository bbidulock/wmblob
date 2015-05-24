
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>

#include <stdio.h>

#if STDC_HEADERS
# include <stdlib.h>
# include <stddef.h>
#else
# if HAVE_STDLIB_H
#  include <stdlib.h>
# endif
#endif

#if HAVE_STRING_H
# if !STDC_HEADERS && HAVE_MEMORY_H
#  include <memory.h>
# endif
# include <string.h>
#endif
#if HAVE_STRINGS_H
# include <strings.h>
#endif

#if HAVE_SELECT
# include <sys/select.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#if HAVE_UNISTD_H
# include <unistd.h>
#else
/* We are in trouble. */
#endif

extern Display *display;
extern int dockapp_iswindowed;
extern int dockapp_isbrokenwm;
extern int color_map;

extern Window window;
extern Window icon_window;

extern void dockapp_open_window(char *, char *, unsigned, unsigned,
				int, char **);
extern void dockapp_set_eventmask(long);
extern void dockapp_show();
extern void dockapp_setshape(Pixmap, int, int);
extern void dockapp_copy2window(XImage *);
extern Bool dockapp_nextevent_or_timeout(XEvent *, unsigned long);
extern XImage *dockapp_create_ximage(int, int);
extern void dockapp_destroy_ximage(XImage *);

