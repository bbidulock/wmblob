
/* jlh */

/*
    this file is part of: wmblob

    the GNU GPL license applies to this file, see file COPYING for details.
    author: jean-luc herren.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "getopt.h"
#include "config.h"
#include "wmblob.h"
#include "mydockapp.h"
#include "dialog.h"
#include "rcfile.h"

#define CEIL 4095 /* max pixel value in buffers */

#define HELP_STRING \
	PACKAGE_STRING", (c) 2002-04 by jlh (jean-luc herren, jlh at gmx dot ch)\n" \
"usage:\n" \
"   wmblob [ --help | -h | --version | --v ]\n" \
"   wmblob [ --display dpy | --windowed | --broken-wm ]...\n" \
"\n" \
"click in the window to open the settings dialog.\n" \

SETTINGS current_settings;

static short buf[4096];  /* rendering buffer (64x64 pixels) */
static short back[4096]; /* backgound (light borders & edges) */
static short blob_buf[1024]; /* one blob (32x32 pixels) */
static short sin_table[4096];

typedef struct
{
	/* speed and position in 1/16 pixel (per frame) */
	short x, y, vx, vy;
} BLOB;

static BLOB blob[MAX_N_BLOBS]; /* the blobs */
static int frame_clock; /* frame counter */
int exit_wmblob; /* non-zero, if we should exit */
static int dialog_is_open; /* non-zero, if dialog is open */

/* parameters for get_value() & set_params() */
/* some values are always held squared */
static int radius2, maximum, center, max_radius2;

static unsigned long my_color_map[64]; /* indices for our colors */
static XImage *ximage;
static char *specified_display;

/* prototypes */

static void set_params(int, int, int);
static int get_value(int);
static void build_blob();
static void build_sin_table();
static int sin_t(int);
static void build_background();
static void init_speed();
static void put_blob(int, int);
static void move();
static void draw();
static void show();
static void interpolate(int, int, COLOR *, COLOR *, COLOR *);
static void alloc_colors();
static void do_pending();


/*
 * the funtion doing the smooth falloff looks something like that:
 *
 *     ^    .
 *     |    .
 *     |    .
 * max-|----- . . . . . .
 *     |    .\_
 *     |    .  \__
 *     |    .     \___
 *     +------------------>
 *          ^         ^
 *          `radius   `max_radius
 *
 *
 *      maximum * (center - radius2) * (x2 - max_radius2)
 * y = ---------------------------------------------------
 *          (center - x2) * (radius2 - max_radius2)
 *
 * center virtually takes a value in [-inf, radius - 1] U [max_radius + 1, inf]
 *
 */


/* set parameter for later use by get_value() */

static void set_params(int size, int falloff, int presence)
{
	max_radius2 = 15 * 15;
	maximum = CEIL * presence / 255;
	radius2 = size;

	/* map falloff to center */
	if (falloff < 128)
		center = radius2 - 1 - (falloff * falloff * 800 / 16129);
	else
	{
		falloff = 255 - falloff;
		center = max_radius2 + 1 + (falloff * falloff * 800 / 16129);
	}
}


/* get a value of the falloff curve. */

static int get_value(int x2)
{
	int i;

	if (x2 <= radius2) return(maximum);

	if (x2 >= max_radius2) return(0);

	i = maximum * (center - radius2) * (x2 - max_radius2) /
		((center - x2) * (radius2 - max_radius2));

	return(i <= 0 ? 0 : i);
}


/* render a blob and put it in blob_buf[] */

static void build_blob()
{
	int x, y;
	short *blob_ptr;

	set_params(current_settings.blob_size,
		current_settings.blob_falloff,
		current_settings.blob_presence);

	blob_ptr = blob_buf;

	for (y = -16; y < 16; y++)
		for (x = -16; x < 16; x++)
			*blob_ptr++ = get_value(x * x + y * y);
}


/* build the sine table */

static void build_sin_table()
{
	int i;

	for (i = 0; i < 4096; i++)
		sin_table[i] = (int)(sin((float)i *
			M_PI * 2.0 / 4096.0) * 1024.0 + 0.5);
}


/* look up value in sine table */

static int sin_t(int a)
{
	return((int)sin_table[a & 0xfff]);
}


/* build the background image (smooth borders & edges) */

static void build_background()
{
	short i, j;
	int c, x, y;
	short *p;

	set_params(current_settings.border_size,
		current_settings.border_falloff,
		current_settings.border_presence);

	p = back;

	for (i = 0; i < 64; i++)
	{
		for (j = 0; j < 64; j++)
		{
			if (i < 32) x = i;
			else x = 63 - i;
			if (j < 32) y = j;
			else y = 63 - j;
			if (x < y) c = x;
			else c = y;
			/* clip edges */
			if (abs(x - y) < 4) c -= (4 - abs(x - y)) / 2;

			*p++ = get_value(c * c);
		}
	}
}


/* set the speed of each blob to a random value */

static void init_speed()
{
	int i;

	if (current_settings.gravity)
	{
		/* gravity mode */

		for (i = 0; i < MAX_N_BLOBS; i++)
		{
			blob[i].vx = 0;
			blob[i].vy = 0;
			blob[i].x = 512;
			blob[i].y = 1000;
		}
	}
	else
	{
		/* normal mode */

		for (i = 0; i < MAX_N_BLOBS; i++)
		{
			blob[i].vx = rand() % 1280 + 80;
			blob[i].vy = rand() % 1280 + 80;
			blob[i].x = 512;
			blob[i].y = 512;
		}
	}
}


/* place a blob in buf[] at the given location (in pixel) */

static void put_blob(int px, int py)
{
	short *blob_ptr, *buf_ptr;
	int sx, sy, x;
	int blob_jump, buf_jump;
	int c;

	if (px <= -16 || py <= -16 || px >= 80 || py >= 80)
		return;

	blob_ptr = blob_buf;
	buf_ptr = buf + px - 16 + (py - 16) * 64;

	if (px < 16)
	{
		sx = 16 + px;
		blob_ptr += 16 - px;
		buf_ptr += 16 - px;
	}
	else if (px > 48)
		sx = 80 - px;
	else
		sx = 32;

	blob_jump = 32 - sx;
	buf_jump = 64 - sx;

	if (py < 16)
	{
		sy = 16 + py;
		blob_ptr += (16 - py) * 32;
		buf_ptr += (16 - py) * 64;
	}
	else if (py > 48)
		sy = 80 - py;
	else
		sy = 32;

	while (sy--)
	{
		x = sx;

		while (x--)
		{
			c = *buf_ptr + *blob_ptr +
				*buf_ptr * *blob_ptr *
				current_settings.multiplication / CEIL;
			if (c < 0) c = 0;
			*buf_ptr = (c < CEIL) ? c : CEIL;
			buf_ptr++;
			blob_ptr++;
		}

		buf_ptr += buf_jump;
		blob_ptr += blob_jump;
	}
}


/* do blob movements */

static void move()
{
	int i;

	if (current_settings.gravity)
	{
		/* gravity mode */

		for (i = 0; i < current_settings.n_blobs; i++)
		{
			if (blob[i].y + blob[i].vy + 8 < 1024)
				blob[i].vy += 8;

			blob[i].x += blob[i].vx;
			blob[i].y += blob[i].vy;

			if (blob[i].x < 0)
			{
				blob[i].x = -blob[i].x;
				blob[i].vx = -blob[i].vx;
			}
			else if (blob[i].x >= 1024)
			{
				blob[i].x = 2047 - blob[i].x;
				blob[i].vx = -blob[i].vx;
			}

			if (blob[i].y < 0)
			{
				blob[i].y = -blob[i].y;
				blob[i].vy = -blob[i].vy;
			}
			else if (blob[i].y >= 1024)
			{
				blob[i].y = 2047 - blob[i].y;
				blob[i].vy = -(blob[i].vy / 2);
			}

			if (blob[i].y > 1008 && abs(blob[i].vy) <= 16)
			{
				blob[i].vy = -(rand() % 96 + 32);
				blob[i].vx = rand() % 128 - 64;
			}
		}

	}
	else
	{
		/* normal mode */

		for (i = 0; i < current_settings.n_blobs; i++)
		{
			blob[i].x = sin_t(frame_clock * blob[i].vx / 16)
					/ 2 + 512;
			blob[i].y = sin_t(frame_clock * blob[i].vy / 16
					+ 256) / 2 + 512;
		}
	}
}


/* draw all blobs in buf[] */

static void draw()
{
	int i;

	/* copy background in buf */
	memcpy(buf, back, 4096 * sizeof(short));

	for (i = 0; i < current_settings.n_blobs; i++)
		put_blob(blob[i].x / 16, blob[i].y / 16);

	frame_clock++;
}


/* copy content of buf to screen */

static void show()
{
	int x, y;
	short *p;

	p = buf;

	for (y = 0; y < 64; y++)
		for (x = 0; x < 64; x++)
			XPutPixel(ximage, x, y, my_color_map[*p++ * 63 / CEIL]);

	dockapp_copy2window(ximage);
//        XPutImage(display, iconwin, NormalGC, ximage, 0, 0, 0, 0, 64, 64);
//	XFlush(display);
}


/* interpolate color.  while i goes from 0 to max, color goes from c1 to c2 */

static void interpolate(int i, int max, COLOR *c1, COLOR *c2, COLOR *color)
{
	color->r = (c1->r * (max - i) + c2->r * i + max / 2) / max;
	color->g = (c1->g * (max - i) + c2->g * i + max / 2) / max;
	color->b = (c1->b * (max - i) + c2->b * i + max / 2) / max;
}


/* generate index map */

static void alloc_colors()
{
	XColor xcolor;
	COLOR color;
	COLOR c[3];
	int i;

	c[0] = current_settings.color[0];
	c[1] = current_settings.color[1];
	c[2] = current_settings.color[2];

        for (i = 0; i < 64; i++)
	{
		if (i <= 32)
			interpolate(i, 32, c + 2, c + 1, &color);
		else
			interpolate(i - 32, 31, c + 1, c, &color);

		xcolor.red = color.r << 8;
		xcolor.green = color.g << 8;
		xcolor.blue = color.b << 8;
		XAllocColor(display, color_map, &xcolor);
		my_color_map[i] = xcolor.pixel;
	}
}


/* handle pending events */

static void do_pending()
{
	XEvent event;

	while (XPending(display))
	{
		XNextEvent(display, &event);

		switch (event.type)
		{
			case DestroyNotify:
				XCloseDisplay(display);
				exit(0);
				break;

			case ButtonPress:
				if (!dialog_is_open)
				{
					dialog_open();
					dialog_is_open = 1;
				}
				break;
		}
	}
}


/* apply the given settings and rebuild blob & background */

void apply_settings(SETTINGS *settings)
{
	int is;

	is = current_settings.gravity != settings->gravity;

	current_settings = *settings;

	if (is)
		init_speed();

	build_blob();
	build_background();

	alloc_colors();
}


/* main loop */

void do_demo(void)
{
	dialog_is_open = 0;
	exit_wmblob = 0;

	while (!exit_wmblob)
	{
		move();
		draw();
		show();
		do_pending();
		usleep(50000);

		if (dialog_is_open)
			dialog_is_open = dialog_loop();
	}
}

/*  */

struct option option_list[] =
{
	{ "help", no_argument, NULL, 'h' },
	{ "version", no_argument, NULL, 'v' },
	{ "broken-wm", no_argument, NULL, 'b' },
	{ "windowed", no_argument, NULL, 'w' },
	{ "display", required_argument, NULL, 'd' },
	{ NULL, no_argument, NULL, 0 }
};

/* scan args */

void scan_args(int argc, char **argv)
{
	int c;

	dockapp_iswindowed = 0;
	dockapp_isbrokenwm = 0;
	specified_display = NULL;

	while (1)
	{
		c = getopt_long(argc, argv, "hv", option_list, NULL);

		if (c == -1) break;

		switch (c)
		{
			case 'v':
				printf(PACKAGE_STRING "\n");
				exit(0);
				break;

			case 'h':
				printf(HELP_STRING);
				exit(0);
				break;

			case 'w':
				dockapp_iswindowed = 1;
				break;

			case 'b':
				dockapp_isbrokenwm = 1;
				break;

			case 'd':
				specified_display = strdup(optarg);
				break;
		}
	}
}


/* home sweet home */

int main(int argc, char **argv)
{
	Pixmap pixmask;
	char mask[512];
	int i;

	scan_args(argc, argv);
	/* init dialog.c */
	init_dialog_c(&argc, &argv);
	init_rcfile_c();

	/* set defaults */
	current_settings.color[0].r = 255;
	current_settings.color[0].g = 255;
	current_settings.color[0].b = 255;
	current_settings.color[1].r = 0;
	current_settings.color[1].g = 0;
	current_settings.color[1].b = 255;
	current_settings.color[2].r = 0;
	current_settings.color[2].g = 0;
	current_settings.color[2].b = 0;

	current_settings.n_blobs = 5;
	current_settings.gravity = 0;

	current_settings.blob_size = 2;
	current_settings.blob_falloff = 64;
	current_settings.blob_presence = 255;

	current_settings.border_size = 2;
	current_settings.border_falloff = 32;
	current_settings.border_presence = 128;

	current_settings.multiplication = 16;

	/* load rc file */
	settings_load();

	/* init */
	srand(getpid());
	build_sin_table();
	build_blob();
	build_background();
	init_speed();

	/* build mask for window */

	memset(mask, 0x00, 24);
	memset(mask + 24, 0xff, 466);
	memset(mask + 488, 0x00, 24);

	for (i = 3; i < 61; i++)
	{
		mask[i * 8] = 0xf8;
		mask[i * 8 + 7] = 0x1f;
	}

	/* init window */

	dockapp_open_window(specified_display, "wmblob", 64, 64, argc, argv);
	free(specified_display);

	dockapp_set_eventmask(ButtonPressMask);

	pixmask = XCreateBitmapFromData(display, icon_window, mask, 64, 64);
	dockapp_setshape(pixmask, 0, 0);

	dockapp_show();

	ximage = dockapp_create_ximage(64, 64);

	alloc_colors();

	/* run it */
	frame_clock = 0;
	do_demo();

	XDestroyImage(ximage);
	done_rcfile_c();

	return(0);
}

