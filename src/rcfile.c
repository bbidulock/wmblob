
/* jlh */

/*
    this file is part of: wmblob

    the GNU GPL license applies to this file, see file COPYING for details.
    author: jean-luc herren.
*/

#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include "rcfile.h"
#include "wmblob.h"

/* string to be placed at the beginning of rc files */

#define RC_HEAD \
"# rc file for wmblob, see www.wmdockapps.org\n" \
"# you can edit this file by hand, but be VERY careful, wmblobs's parser\n" \
"# is VERY cheap.  do not change the format in any way.  better not edit\n" \
"# while wmblob is running.\n\n"

static char buf[1024];
static FILE *handle;
static char line[1024];

GList *preset_list;

/* prototypes */

static int set_file();
static void create_default_file();
static void save_entry(SETTINGS *);
static int read_line();
static void read_entry(SETTINGS *settings, char *);
static PRESET *get_preset(const char *);


/* initialize stuff in this file */

int init_rcfile_c()
{
	preset_list = NULL;
	return(1);
}


/* uninitialize stuff in this file */

void done_rcfile_c()
{
	GList *node;
	PRESET *preset;

	node = preset_list;

	while (node)
	{
		preset = (PRESET *)(node->data);
		free(preset);

		node = g_list_next(node);
	}

	g_list_free(preset_list);
}


/* set path to config file in buf */

static int set_file()
{
	char *home;

	home = getenv("HOME");

	if (!home)
	{
		printf("error: $HOME is not set.  cannot access rc file\n");
		return(0);
	}

	sprintf(buf, "%s/.wmblobrc", home);

	return(1);
}


/* write a default config file */

static void create_default_file()
{
	if (!set_file()) return;

	handle = fopen(buf, "wb");
	if (!handle) return;

	fprintf(handle, "%s", RC_HEAD);
	fprintf(handle,
"current=255,255,255,0,0,255,0,0,0,5,0,2,16,255,2,9,128,16\n"
"preset=default,255,255,255,0,0,255,0,0,0,5,0,2,16,255,2,9,128,16\n"
"preset=dots,0,0,0,0,191,255,255,255,255,40,0,2,3,32,0,16,127,20\n"
"preset=electric charge,0,0,0,0,0,255,255,255,255,50,0,5,32,255,0,0,0,64\n"
"preset=flying dirt,64,64,0,128,128,0,240,240,220,50,1,0,0,10,0,64,64,100\n"
"preset=fire,238,219,0,255,82,0,165,42,42,20,1,3,16,224,3,32,128,16\n"
"preset=glow-worms,255,255,0,30,144,255,0,0,0,20,0,0,0,127,0,0,0,127\n"
"preset=green contour,0,0,0,0,255,0,0,0,0,3,0,20,16,255,0,16,128,24\n"
"preset=lonely blue,255,255,255,0,0,255,0,0,0,1,0,1,16,255,0,8,255,64\n"
"preset=noise,0,0,0,255,255,0,173,216,230,50,0,220,255,255,0,0,0,16\n"
"preset=orange,255,255,255,255,165,0,255,255,255,4,0,50,16,128,0,64,128,0\n"
"preset=wiggle,0,0,0,0,191,255,255,255,255,30,0,10,20,64,3,33,128,32\n"
"preset=yellow,255,255,255,255,255,0,0,0,0,5,0,2,20,255,0,24,96,8\n"
		);

	fclose(handle);
}


/* save a set of settings */

static void save_entry(SETTINGS *settings)
{
	fprintf(handle, "%d,%d,%d,%d,%d,%d,%d,%d,%d,",
			settings->color[0].r,
			settings->color[0].g,
			settings->color[0].b,
			settings->color[1].r,
			settings->color[1].g,
			settings->color[1].b,
			settings->color[2].r,
			settings->color[2].g,
			settings->color[2].b);

	fprintf(handle, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			settings->n_blobs,
			settings->gravity,
			settings->blob_size,
			settings->blob_falloff,
			settings->blob_presence,
			settings->border_size,
			settings->border_falloff,
			settings->border_presence,
			settings->multiplication);
}


/* save current settings and presets */

int settings_save()
{
	GList *node;
	PRESET *preset;

	if (!set_file()) return(0);

	handle = fopen(buf, "wb");
	if (!handle) return(0);

	fprintf(handle, "%s", RC_HEAD);

	fprintf(handle, "current=");
	save_entry(&current_settings);

	node = preset_list;

	while (node)
	{
		preset = (PRESET *)node->data;
		fprintf(handle, "preset=%s,", preset->name);
		save_entry(&preset->settings);

		node = g_list_next(node);
	}

	fclose(handle);

	return(1);
}


/* read a line from the file */

static int read_line()
{
	int c;
	char *p;

	p = line;

	while (1)
	{
		c = getc(handle);

		if (c == EOF) return(0);

		if (c == '\n')
		{
			*p = '\0';
			return(1);
		}

		if (c == '\r') continue;

		*p++ = c;
	}
}


/* read a settings entry.  well, this is cheaper than cheap, I know */

static void read_entry(SETTINGS *settings, char *str)
{
	settings->color[0].r = atoi(str);
	str = strchr(str, ',') + 1;
	settings->color[0].g = atoi(str);
	str = strchr(str, ',') + 1;
	settings->color[0].b = atoi(str);
	str = strchr(str, ',') + 1;
	settings->color[1].r = atoi(str);
	str = strchr(str, ',') + 1;
	settings->color[1].g = atoi(str);
	str = strchr(str, ',') + 1;
	settings->color[1].b = atoi(str);
	str = strchr(str, ',') + 1;
	settings->color[2].r = atoi(str);
	str = strchr(str, ',') + 1;
	settings->color[2].g = atoi(str);
	str = strchr(str, ',') + 1;
	settings->color[2].b = atoi(str);
	str = strchr(str, ',') + 1;

	settings->n_blobs = atoi(str);
	str = strchr(str, ',') + 1;
	settings->gravity = atoi(str);
	str = strchr(str, ',') + 1;
	settings->blob_size = atoi(str);
	str = strchr(str, ',') + 1;
	settings->blob_falloff = atoi(str);
	str = strchr(str, ',') + 1;
	settings->blob_presence = atoi(str);
	str = strchr(str, ',') + 1;
	settings->border_size = atoi(str);
	str = strchr(str, ',') + 1;
	settings->border_falloff = atoi(str);
	str = strchr(str, ',') + 1;
	settings->border_presence = atoi(str);
	str = strchr(str, ',') + 1;
	settings->multiplication = atoi(str);
}


/* load current settings and presets.  cheap, cheap, cheap */

void settings_load()
{
	char *value;

	if (!set_file()) return;

	handle = fopen(buf, "rb");

	if (!handle)
	{
		/* could not open file.  store a default config file
		   and try again. */
		create_default_file();

		handle = fopen(buf, "rb");
		if (!handle) return;
	}

	while (1)
	{
		if (!read_line()) break;
		if (*line == '#') continue;
		if (*line == '\n') continue;
		if (*line == '\0') continue;

		/* look for the '=' */

		value = line + 1;
		while (*value != '=')
			value++;

		*value++ = '\0';

		if (strcmp(line, "current") == 0)
		{
			read_entry(&current_settings, value);
		}
		else if (strcmp(line, "preset") == 0)
		{
			PRESET *preset;
			char *p;

			p = strchr(value, ',');
			*p++ = '\0';

			preset = (PRESET *)malloc(sizeof(PRESET));
			strcpy(preset->name, value);
			read_entry(&preset->settings, p);
			preset_list = g_list_append(preset_list, preset);
		}
		else
			printf("unknown option `%s' in rc file\n", line);
	}

	fclose(handle);
}


/* look for a preset named name */

static PRESET *get_preset(const char *name)
{
	GList *preset;

	preset = preset_list;

	while (preset)
	{
		if (strcmp(((PRESET *)preset->data)->name, name) == 0)
			return(preset->data);

		preset = g_list_next(preset);
	}

	return(NULL);
}


/* save a preset and create it, if not already in the list */

void preset_save(const char *name, SETTINGS *settings)
{
	PRESET *preset;

	preset = get_preset(name);

	if (!preset)
	{
		preset = (PRESET *)malloc(sizeof(PRESET));
		strcpy(preset->name, name);
		preset_list = g_list_append(preset_list, preset);
	}

	preset->settings = *settings;

	settings_save();
}


/* delete the named preset */

void preset_delete(const char *name)
{
	PRESET *preset;

	preset = get_preset(name);
	if (!preset) return;

	preset_list = g_list_remove(preset_list, preset);
	free(preset);

	settings_save();
}


/* load the named preset */

int preset_load(const char *name, SETTINGS *settings)
{
	PRESET *preset;

	preset = get_preset(name);
	if (!preset) return(0);

	*settings = preset->settings;

	return(1);
}

