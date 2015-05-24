
/* jlh */

/*
    this file is part of: wmblob

    the GNU GPL license applies to this file, see file COPYING for details.
    author: jean-luc herren.
*/

#ifndef RCFILE_H
#define RCFILE_H

#include <glib.h>
#include "wmblob.h"

typedef struct
{
	char name[256];
	SETTINGS settings;
} PRESET;

extern GList *preset_list;

extern int init_rcfile_c();
extern void done_rcfile_c();
extern int settings_save();
extern void settings_load();

extern void preset_save(const char *, SETTINGS *);
extern void preset_delete(const char *);
extern int preset_load(const char *, SETTINGS *);

#endif

