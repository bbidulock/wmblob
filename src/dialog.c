
/* jlh */

/*
    this file is part of: wmblob

    the GNU GPL license applies to this file, see file COPYING for details.
    author: jean-luc herren.
*/

#include <stdio.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <sys/utsname.h>
#include <string.h>
#include <stdlib.h>

#include "config.h"
#include "wmblob.h"
#include "rcfile.h"

static int gtk_failed = 0; /* non-zero if gtk has failed initialization */
static SETTINGS new_settings;
static int exit_dialog;

/* pointer to important widgets */

static GtkWidget *dialog_window;

static GtkWidget *wg_color[3];

static GtkWidget *wg_n_blobs;
static GtkWidget *wg_multiplication;
static GtkWidget *wg_gravity;

static GtkWidget *wg_blob_size;
static GtkWidget *wg_blob_falloff;
static GtkWidget *wg_blob_presence;

static GtkWidget *wg_border_size;
static GtkWidget *wg_border_falloff;
static GtkWidget *wg_border_presence;

static GtkWidget *wg_preset_list;

static GtkWidget *color_dialog;

/* all prototypes */

static void set_from_settings(SETTINGS *);
static gboolean callback_expose(GtkWidget *, GdkEventExpose *, gpointer);
static gboolean callback_close(GtkWidget *, gpointer);
static gboolean callback_exit(GtkWidget *, gpointer);
static gboolean callback_preset_load(GtkWidget *, gpointer);
static gboolean callback_preset_save(GtkWidget *, gpointer);
static gboolean callback_preset_delete(GtkWidget *, gpointer);
static gboolean callback_about(GtkWidget *, gpointer);
static void callback_color_clicked(GtkWidget *, gpointer);
static void callback_color_okay(GtkWidget *, gpointer);
static void callback_color_cancel(GtkWidget *, gpointer);
static GtkWidget *create_color_button(int);
static GtkWidget *hbox_with_label(GtkWidget *, const char *);
static GtkWidget *new_frame(GtkWidget *, const char *);
static GtkWidget *vbox_spin_button(GtkWidget *, const char *, int, int);
static GtkWidget *create_dialog_left();
static void fill_in_presets(GtkWidget *);
static GtkWidget *create_dialog_right();
static GtkWidget *create_dialog();


/* set dialog state from the given settings */

static void set_from_settings(SETTINGS *settings)
{
	GdkColor col;
	int i;

	new_settings = *settings;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(wg_n_blobs),
			settings->n_blobs);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wg_gravity),
			settings->gravity);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(wg_blob_size),
			settings->blob_size);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(wg_blob_falloff),
			settings->blob_falloff);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(wg_blob_presence),
			settings->blob_presence);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(wg_border_size),
			settings->border_size);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(wg_border_falloff),
			settings->border_falloff);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(wg_border_presence),
			settings->border_presence);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(wg_multiplication),
			settings->multiplication);

	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(wg_preset_list)->entry), "");

	for (i = 0; i < 3; i++)
	{
		col.red = settings->color[i].r * 256;
		col.green = settings->color[i].g * 256;
		col.blue = settings->color[i].b * 256;

		gtk_widget_modify_bg(
			gtk_bin_get_child(GTK_BIN(wg_color[i])),
			GTK_STATE_NORMAL, &col);
	}
}


/* draw the area in the colored buttons */

static gboolean callback_expose(GtkWidget *widget, GdkEventExpose *event,
				gpointer data)
{
	GtkStyle *style;

	style = gtk_widget_get_style (widget);

	gdk_draw_rectangle(widget->window,
			style->bg_gc[GTK_STATE_NORMAL],
			TRUE,
			event->area.x, event->area.y,
			event->area.width, event->area.height);

	return(TRUE);
}


/* handler for the close button */

static gboolean callback_close(GtkWidget *widget, gpointer data)
{
	exit_dialog = 1;
	return(TRUE);
}


/* handler for the exit button */

static gboolean callback_exit(GtkWidget *widget, gpointer data)
{
	exit_dialog = 1;
	exit_wmblob = 1;
	return(TRUE);
}


/* handler for the load preset button */

static gboolean callback_preset_load(GtkWidget *widget, gpointer data)
{
	SETTINGS set;
	const char *p;

	p = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(wg_preset_list)->entry));

	if (preset_load(p, &set))
		set_from_settings(&set);
	p = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(wg_preset_list)->entry));

	return(TRUE);
}


/* handler for the save preset button */

static gboolean callback_preset_save(GtkWidget *widget, gpointer data)
{
	const char *p;
	char *tmp;

	p = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(wg_preset_list)->entry));
	if (!*p) return(TRUE);

	/* no comma! */
	if (strchr(p, ',')) return(TRUE);

	tmp = strdup(p);

	preset_save(tmp, &new_settings);
	fill_in_presets(GTK_WIDGET(wg_preset_list));

	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(wg_preset_list)->entry), tmp);
	free(tmp);

	return(TRUE);
}


/* handler for the delete preset button */

static gboolean callback_preset_delete(GtkWidget *widget, gpointer data)
{
	const char *p;

	p = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(wg_preset_list)->entry));
	preset_delete(p);

	fill_in_presets(GTK_WIDGET(wg_preset_list));

	return(TRUE);
}


/* handler for the about button */

static gboolean callback_about(GtkWidget *widget, gpointer data)
{
	char buf[256];
	struct utsname system;
	GtkWidget *dialog;
	char *add;

	uname(&system);

	if (strcmp(system.sysname, "Linux") == 0 ||
		strcmp(system.sysname, "linux") == 0)
		add = ", this is GOOD.";
	else
		add = "";

	sprintf(buf,
		"This is " PACKAGE_STRING "\n"
		"coded by jlh (aka jean-luc herren)\n\n"
		"You're running %s%s", system.sysname, add);

	dialog = gtk_message_dialog_new(GTK_WINDOW(dialog_window),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_INFO,
			GTK_BUTTONS_CLOSE,
			buf);

	g_signal_connect_swapped(GTK_OBJECT(dialog), "response",
			G_CALLBACK(gtk_widget_destroy),
			GTK_OBJECT(dialog));

	gtk_window_set_transient_for(GTK_WINDOW(dialog),
			GTK_WINDOW(dialog_window));
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_widget_show_all(dialog);

	return(TRUE);
}


/* handler for the apply button */

static gboolean callback_apply(GtkWidget *widget, gpointer data)
{
	new_settings.gravity = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(wg_gravity));
	new_settings.n_blobs = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(wg_n_blobs));

	new_settings.blob_size = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(wg_blob_size));
	new_settings.blob_falloff = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(wg_blob_falloff));
	new_settings.blob_presence = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(wg_blob_presence));

	new_settings.border_size = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(wg_border_size));
	new_settings.border_falloff = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(wg_border_falloff));
	new_settings.border_presence = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(wg_border_presence));

	new_settings.multiplication = gtk_spin_button_get_value_as_int(
			GTK_SPIN_BUTTON(wg_multiplication));

	/* the color informations are already in new_settings */

	apply_settings(&new_settings);
	settings_save();

	return(TRUE);
}


/* handler for the three colored buttons.  open a color selection dialog */

static void callback_color_clicked(GtkWidget *widget, gpointer data)
{
	GtkWidget *button;
	GtkColorSelection *colorsel;
	GdkColor col;
	int i;

	i = GPOINTER_TO_INT(data);

	col.red = new_settings.color[i].r * 256;
	col.green = new_settings.color[i].g * 256;
	col.blue = new_settings.color[i].b * 256;

	color_dialog = gtk_color_selection_dialog_new("Changing color");

	colorsel = GTK_COLOR_SELECTION(
			GTK_COLOR_SELECTION_DIALOG(color_dialog)->colorsel);

	/* set up the okay button */
	button = GTK_COLOR_SELECTION_DIALOG(color_dialog)->ok_button;
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(callback_color_okay), data);

	/* set up the cancel button */
	button = GTK_COLOR_SELECTION_DIALOG(color_dialog)->cancel_button;
	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(callback_color_cancel), data);

	/* remove the help button */
	button = GTK_COLOR_SELECTION_DIALOG(color_dialog)->help_button;
	gtk_widget_destroy(button);

	gtk_color_selection_set_previous_color(colorsel, &col);
	gtk_color_selection_set_current_color(colorsel, &col);
	gtk_color_selection_set_has_palette(colorsel, TRUE);

	gtk_window_set_transient_for(GTK_WINDOW(color_dialog),
			GTK_WINDOW(dialog_window));
	gtk_window_set_modal(GTK_WINDOW(color_dialog), TRUE);
	gtk_widget_show_all(color_dialog);
}


/* handler for the okay button of the color section dialog */

static void callback_color_okay(GtkWidget *dialog, gpointer data)
{
	GtkColorSelection *colorsel;
	GdkColor col;
	int i;

	i = GPOINTER_TO_INT(data);
	colorsel = GTK_COLOR_SELECTION(
		GTK_COLOR_SELECTION_DIALOG(color_dialog)->colorsel);

	gtk_color_selection_get_current_color(colorsel, &col);
	gtk_widget_modify_bg(gtk_bin_get_child(GTK_BIN(wg_color[i])),
			GTK_STATE_NORMAL, &col);

	new_settings.color[i].r = col.red / 256;
	new_settings.color[i].g = col.green / 256;
	new_settings.color[i].b = col.blue / 256;

	gtk_widget_destroy(color_dialog);
}


/* handler for the cancel button of the color section dialog */

static void callback_color_cancel(GtkWidget *dialog, gpointer data)
{
	gtk_widget_destroy(color_dialog);
}


/* create a button with a color area in it */

static GtkWidget *create_color_button(int i)
{
	GtkWidget *button;
	GtkWidget *area;

	button = gtk_button_new();
	area = gtk_drawing_area_new();

	g_signal_connect(G_OBJECT(button), "clicked",
			G_CALLBACK(callback_color_clicked), GINT_TO_POINTER(i));
	g_signal_connect(G_OBJECT(area), "expose_event",
			G_CALLBACK(callback_expose), NULL);

	gtk_widget_set_size_request(area, 24, 16);
	gtk_container_add(GTK_CONTAINER(button), area);

	return(button);
}


/* return a hbox containing a label and the given widget */

static GtkWidget *hbox_with_label(GtkWidget *widget, const char *name)
{
	GtkWidget *hbox;
	GtkWidget *label;

	hbox = gtk_hbox_new(FALSE, 5);

	label = gtk_label_new(name);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbox), widget, FALSE, FALSE, 0);

	return(hbox);
}


/* add a frame to parent and put a vbox in it (returned) */

static GtkWidget *new_frame(GtkWidget *parent, const char *name)
{
	GtkWidget *frame, *vbox;

	frame = gtk_frame_new(name);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start(GTK_BOX(parent), frame, FALSE, FALSE, 0);

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 5);

	return(vbox);
}


/* create a spin button with a label and append it to the given vbox.
   return the spin button */

static GtkWidget *vbox_spin_button(GtkWidget *vbox, const char *name,
		int min, int max)
{
	GtkWidget *spin, *box;

	spin = gtk_spin_button_new_with_range(min, max, 1);
	box = hbox_with_label(spin, name);
	gtk_box_pack_start(GTK_BOX(vbox), box, FALSE, FALSE, 0);

	return(spin);
}


/* create and return a vbox containing the left half of the dialog */

static GtkWidget *create_dialog_left()
{
	GtkWidget *left, *vbox;
	GtkWidget *widget;
	GtkWidget *table;

	left = gtk_vbox_new(FALSE, 5);

	vbox = new_frame(left, "Global");

	/* spin button for number of blobs */

	wg_n_blobs = vbox_spin_button(vbox, "Blob count:", 1, MAX_N_BLOBS);

	/* check boxes */

	wg_multiplication = vbox_spin_button(vbox, "Overlay Effect:", 0, 127);

	wg_gravity = gtk_check_button_new_with_mnemonic("_Gravity");
	gtk_box_pack_start(GTK_BOX(vbox), wg_gravity, TRUE, TRUE, 0);

	/* color buttons */

	vbox = new_frame(left, "Colors");

	table = gtk_table_new(2, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	widget = gtk_label_new("Center:");
	gtk_misc_set_alignment (GTK_MISC(widget), 0, 1);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 0, 1,
			GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
	wg_color[0] = create_color_button(0);
	gtk_table_attach_defaults(GTK_TABLE(table), wg_color[0], 1, 2, 0, 1);

	widget = gtk_label_new("Glow:");
	gtk_misc_set_alignment (GTK_MISC(widget), 0, 1);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 1, 2,
			GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
	wg_color[1] = create_color_button(1);
	gtk_table_attach_defaults(GTK_TABLE(table), wg_color[1], 1, 2, 1, 2);

	widget = gtk_label_new("Background:");
	gtk_misc_set_alignment (GTK_MISC(widget), 0, 1);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 2, 3,
			GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
	wg_color[2] = create_color_button(2);
	gtk_table_attach_defaults(GTK_TABLE(table), wg_color[2], 1, 2, 2, 3);

	/* blob style */

	vbox = new_frame(left, "Blob style");

	wg_blob_size = vbox_spin_button(vbox, "Size:", 0, 220);
	wg_blob_falloff = vbox_spin_button(vbox, "Falloff:", 0, 255);
	wg_blob_presence = vbox_spin_button(vbox, "Presence:", 0, 255);

	/* border style */

	vbox = new_frame(left, "Border style");

	wg_border_size = vbox_spin_button(vbox, "Size:", 0, 127);
	wg_border_falloff = vbox_spin_button(vbox, "Falloff:", 0, 255);
	wg_border_presence = vbox_spin_button(vbox, "Presence:", 0, 255);

	return(left);
}


/* fill the given combo widget with the preset list */

static void fill_in_presets(GtkWidget *combo)
{
	GList *items;
	GList *preset;

	preset = preset_list;
	items = NULL;

	while (preset)
	{
		items = g_list_append(items, preset->data);
		preset = g_list_next(preset);
	}

	if (!items)
		items = g_list_append(items, "");

	/* sort via strcmp() */
	items = g_list_sort_with_data(items, (GCompareDataFunc)strcmp, NULL);

	gtk_combo_set_popdown_strings(GTK_COMBO(combo), items);

	g_list_free(items);
}


/* create and return a vbox containing the right half of the dialog */

static GtkWidget *create_dialog_right()
{
	GtkWidget *right;
	GtkWidget *widget;
	GtkWidget *vbox;

	right = gtk_vbox_new(FALSE, 10);

	/* save, apply, close, exit */

	widget = gtk_button_new_with_mnemonic("_Apply");
	g_signal_connect(G_OBJECT(widget), "clicked",
			G_CALLBACK(callback_apply), NULL);
	gtk_box_pack_start(GTK_BOX(right), widget, FALSE, FALSE, 0);


	widget = gtk_button_new_with_mnemonic("_Close dialog");
	g_signal_connect(G_OBJECT(widget), "clicked",
			G_CALLBACK(callback_close), NULL);
	gtk_box_pack_start(GTK_BOX(right), widget, FALSE, FALSE, 0);

	widget = gtk_button_new_with_mnemonic("E_xit wmblob");
	g_signal_connect(G_OBJECT(widget), "clicked",
			G_CALLBACK(callback_exit), NULL);
	gtk_box_pack_start(GTK_BOX(right), widget, FALSE, FALSE, 0);

	widget = gtk_button_new_with_mnemonic("A_bout...");
	g_signal_connect(G_OBJECT(widget), "clicked",
			G_CALLBACK(callback_about), NULL);
	gtk_box_pack_start(GTK_BOX(right), widget, FALSE, FALSE, 0);

	/* preset stuff */

	vbox = new_frame(right, "Presets");

	widget = gtk_button_new_with_mnemonic("_Load");
	g_signal_connect(G_OBJECT(widget), "clicked",
			G_CALLBACK(callback_preset_load), NULL);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);

	widget = gtk_button_new_with_mnemonic("_Save");
	g_signal_connect(G_OBJECT(widget), "clicked",
			G_CALLBACK(callback_preset_save), NULL);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);

	widget = gtk_button_new_with_mnemonic("_Delete");
	g_signal_connect(G_OBJECT(widget), "clicked",
			G_CALLBACK(callback_preset_delete), NULL);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);

	wg_preset_list = gtk_combo_new();
	fill_in_presets(wg_preset_list);
	gtk_widget_set_size_request(wg_preset_list, 128, 20);
	gtk_box_pack_start(GTK_BOX(vbox), wg_preset_list, FALSE, FALSE, 0);

	return(right);
}


/* create and return the complete dialog */

static GtkWidget *create_dialog()
{
	GtkWidget *dialog, *hbox;
	GtkWidget *left, *right;

	/* create main window */

	dialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(dialog), "wmblob settings");
	gtk_container_set_border_width(GTK_CONTAINER(dialog), 8);
	g_signal_connect(G_OBJECT(dialog), "delete-event",
			G_CALLBACK(callback_close), NULL);

	/* box structure */

	left = create_dialog_left();
	right = create_dialog_right();

	hbox = gtk_hbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(hbox), left);
	gtk_container_add(GTK_CONTAINER(hbox), right);
	gtk_container_add(GTK_CONTAINER(dialog), hbox);

	gtk_widget_show_all(dialog);

	return(dialog);
}


/* do initializations for this file */

int init_dialog_c(int *argc, char ***argv)
{
	if (!gtk_init_check(argc, argv))
	{
		printf("wmblob: gtk_init() has failed, "
			"no settings dialog available\n");
		gtk_failed = 1;
		return(0);
	}

	return(1);
}


/* open the dialog */

int dialog_open()
{
	if (gtk_failed) return(0);

	dialog_window = create_dialog();
	exit_dialog = 0;

	set_from_settings(&current_settings);

	return(1);
}


/* handle pending events for the dialog */

int dialog_loop()
{
	if (gtk_failed) return(0);

	while (gtk_events_pending())
		gtk_main_iteration();

	if (exit_dialog)
	{
		gtk_widget_destroy(dialog_window);

		while (gtk_events_pending())
			gtk_main_iteration();

		return(0);
	}

	return(1);
}

