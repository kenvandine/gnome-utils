/* Calendar properties dialog box
 *
 * Copyright (C) 1998 the Free Software Foundation
 *
 * Authors: Miguel de Icaza <miguel@kernel.org>
 *          Federico Mena <federico@nuclecu.unam.mx>
 */
#include <config.h>
#include <langinfo.h>
#include <gnome.h>
#include "gnome-cal.h"
#include "gnome-month-item.h"
#include "main.h"
#include "mark.h"

/* These specify the page numbers in the preferences notebook */
enum {
	PROP_TIME_DISPLAY,
	PROP_COLORS
};

static GtkWidget *prop_win;		/* The preferences dialog */

/* Widgets for the time display page */

static GtkWidget *time_format_12;	/* Radio button for 12-hour format */
static GtkWidget *time_format_24;	/* Radio button for 24-hour format */
static GtkWidget *start_on_sunday;	/* Check button for weeks starting on Sunday */
static GtkWidget *start_on_monday;	/* Check button for weeks starting on Monday */
static GtkWidget *start_omenu;		/* Option menu for start of day */
static GtkWidget *end_omenu;		/* Option menu for end of day */
static GtkWidget *start_items[24];	/* Menu items for start of day menu */
static GtkWidget *end_items[24];	/* Menu items for end of day menu */

/* Widgets for the colors page */

static GtkWidget *color_pickers[COLOR_PROP_LAST];
static GnomeCanvasItem *month_item;


/* Callback used when the property box is closed -- just sets the prop_win variable to null. */
static int
prop_cancel (void)
{
	prop_win = NULL;
	return FALSE;
}

/* Returns the index of the active item in a menu */
static int
get_active_index (GtkWidget *menu)
{
	GtkWidget *active;

	active = gtk_menu_get_active (GTK_MENU (menu));
	return GPOINTER_TO_INT (gtk_object_get_user_data (GTK_OBJECT (active)));
}

/* Applies the settings in the time display page */
static void
prop_apply_time_display (void)
{
	/* Day begin/end */

	day_begin = get_active_index (gtk_option_menu_get_menu (GTK_OPTION_MENU (start_omenu)));
	day_end   = get_active_index (gtk_option_menu_get_menu (GTK_OPTION_MENU (end_omenu)));
	gnome_config_set_int ("/calendar/Calendar/Day start", day_begin);
	gnome_config_set_int ("/calendar/Calendar/Day end", day_end);

	/* Time format */

	am_pm_flag = GTK_TOGGLE_BUTTON (time_format_12)->active;
	gnome_config_set_bool ("/calendar/Calendar/AM PM flag", am_pm_flag);

	/* Week start */

	week_starts_on_monday = GTK_TOGGLE_BUTTON (start_on_monday)->active;
	gnome_config_set_bool ("/calendar/Calendar/Week starts on Monday", week_starts_on_monday);

	gnome_config_sync ();
	time_format_changed ();
}

/* Applies the settings in the colors page */
static void
prop_apply_colors (void)
{
	int i;
	char *cspec;

	for (i = 0; i < COLOR_PROP_LAST; i++) {
		gnome_color_picker_get_i16 (GNOME_COLOR_PICKER (color_pickers[i]),
					    &color_props[i].r, &color_props[i].g, &color_props[i].b, NULL);

		cspec = build_color_spec (color_props[i].r, color_props[i].g, color_props[i].b);
		gnome_config_set_string (color_props[i].key, cspec);
	}

	gnome_config_sync ();
	colors_changed ();
}

/* Callback used when the Apply button is clicked. */
static void
prop_apply (GtkWidget *w, int page)
{
	switch (page) {
	case PROP_TIME_DISPLAY:
		prop_apply_time_display ();
		break;

	case PROP_COLORS:
		prop_apply_colors ();
		break;

	case -1:
		break;

	default:
		g_warning ("We have a loose penguin!");
		g_assert_not_reached ();
	}
}

/* Notifies the property box that the data has changed */
static void
prop_changed (void)
{
	gnome_property_box_changed (GNOME_PROPERTY_BOX (prop_win));
}

/* Builds and returns a two-element radio button group surrounded by a frame.  The radio buttons are
 * stored in the specified variables, and the first radio button's state is set according to the
 * specified flag value.  The buttons are connected to the prop_changed() function to update the property
 * box's dirty state.
 */
static GtkWidget *
build_two_radio_group (char *title,
		       char *radio_1_title, GtkWidget **radio_1_widget,
		       char *radio_2_title, GtkWidget **radio_2_widget,
		       int radio_1_value)
{
	GtkWidget *frame;
	GtkWidget *vbox;

	frame = gtk_frame_new (title);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	*radio_1_widget = gtk_radio_button_new_with_label (NULL, radio_1_title);
	gtk_box_pack_start (GTK_BOX (vbox), *radio_1_widget, FALSE, FALSE, 0);
	
	*radio_2_widget = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (*radio_1_widget),
								       radio_2_title);
	gtk_box_pack_start (GTK_BOX (vbox), *radio_2_widget, FALSE, FALSE, 0);

	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (*radio_1_widget), radio_1_value);
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (*radio_2_widget), !radio_1_value);

	gtk_signal_connect (GTK_OBJECT (*radio_1_widget), "toggled",
			    (GtkSignalFunc) prop_changed,
			    NULL);

	return frame;
}

/* Callback invoked when a menu item from the start/end time option menus is selected.  It adjusts
 * the other menu to the proper time, if needed.
 */
static void
hour_activated (GtkWidget *widget, gpointer data)
{
	int start, end;

	if (data == start_omenu) {
		/* Adjust the end menu */

		start = GPOINTER_TO_INT (gtk_object_get_user_data (GTK_OBJECT (widget)));
		end = get_active_index (gtk_option_menu_get_menu (GTK_OPTION_MENU (end_omenu)));

		if (end < start)
			gtk_option_menu_set_history (GTK_OPTION_MENU (end_omenu), start);
	} else if (data == end_omenu) {
		/* Adjust the start menu */

		end = GPOINTER_TO_INT (gtk_object_get_user_data (GTK_OBJECT (widget)));
		start = get_active_index (gtk_option_menu_get_menu (GTK_OPTION_MENU (start_omenu)));

		if (start > end)
			gtk_option_menu_set_history (GTK_OPTION_MENU (start_omenu), end);
	} else
		g_assert_not_reached ();

	gnome_property_box_changed (GNOME_PROPERTY_BOX (prop_win));
}

/* Builds an option menu of 24 hours */
static GtkWidget *
build_hours_menu (GtkWidget **items, int active)
{
	GtkWidget *omenu;
	GtkWidget *menu;
	int i;
	char buf[100];
	struct tm tm;

	omenu = gtk_option_menu_new ();
	menu = gtk_menu_new ();

	memset (&tm, 0, sizeof (tm));

	for (i = 0; i < 24; i++) {
		tm.tm_hour = i;
		strftime (buf, 100, "%I:%M %p", &tm);

		items[i] = gtk_menu_item_new_with_label (buf);
		gtk_object_set_user_data (GTK_OBJECT (items[i]), GINT_TO_POINTER (i));
		gtk_signal_connect (GTK_OBJECT (items[i]), "activate",
				    (GtkSignalFunc) hour_activated,
				    omenu);

		gtk_menu_append (GTK_MENU (menu), items[i]);
		gtk_widget_show (items[i]);
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
	gtk_option_menu_set_history (GTK_OPTION_MENU (omenu), active);
	return omenu;
}

/* Creates the time display page in the preferences dialog */
static void
create_time_display_page (void)
{
	GtkWidget *table;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *hbox2;
	GtkWidget *hbox3;
	GtkWidget *w;

	table = gtk_table_new (2, 2, FALSE);
	gtk_container_border_width (GTK_CONTAINER (table), GNOME_PAD_SMALL);
	gtk_table_set_row_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
	gtk_table_set_col_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
	gnome_property_box_append_page (GNOME_PROPERTY_BOX (prop_win), table,
					gtk_label_new (_("Time display")));

	/* Time format */

	w = build_two_radio_group (_("Time format"),
				   _("12-hour (AM/PM)"), &time_format_12,
				   _("24-hour"), &time_format_24,
				   am_pm_flag);
	gtk_table_attach (GTK_TABLE (table), w,
			  0, 1, 0, 1,
			  GTK_EXPAND | GTK_FILL,
			  GTK_EXPAND | GTK_FILL,
			  0, 0);

	/* Weeks start on */

	w = build_two_radio_group (_("Weeks start on"),
				   _("Sunday"), &start_on_sunday,
				   _("Monday"), &start_on_monday,
				   !week_starts_on_monday);
	gtk_table_attach (GTK_TABLE (table), w,
			  0, 1, 1, 2,
			  GTK_EXPAND | GTK_FILL,
			  GTK_EXPAND | GTK_FILL,
			  0, 0);

	/* Day range */

	frame = gtk_frame_new (_("Day range"));
	gtk_table_attach (GTK_TABLE (table), frame,
			  1, 2, 0, 2,
			  GTK_EXPAND | GTK_FILL,
			  GTK_EXPAND | GTK_FILL,
			  0, 0);

	vbox = gtk_vbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_container_border_width (GTK_CONTAINER (vbox), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	w = gtk_label_new (_("Please select the start and end hours you want\n"
			     "to be displayed in the day view and week view.\n"
			     "Times outside this range will not be displayed\n"
			     "by default."));
	gtk_label_set_justify (GTK_LABEL (w), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (w), 0.0, 0.0);
	gtk_box_pack_start (GTK_BOX (vbox), w, FALSE, FALSE, 0);

	hbox2 = gtk_hbox_new (FALSE, GNOME_PAD);
	gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, FALSE, 0);

	/* Day start */

	hbox3 = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (hbox2), hbox3, FALSE, FALSE, 0);

	w = gtk_label_new (_("Day start:"));
	gtk_box_pack_start (GTK_BOX (hbox3), w, FALSE, FALSE, 0);

	start_omenu = build_hours_menu (start_items, day_begin);
	gtk_box_pack_start (GTK_BOX (hbox3), start_omenu, FALSE, FALSE, 0);

	/* Day end */

	hbox3 = gtk_hbox_new (FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (hbox2), hbox3, FALSE, FALSE, 0);

	w = gtk_label_new (_("Day end:"));
	gtk_box_pack_start (GTK_BOX (hbox3), w, FALSE, FALSE, 0);

	end_omenu = build_hours_menu (end_items, day_end);
	gtk_box_pack_start (GTK_BOX (hbox3), end_omenu, FALSE, FALSE, 0);
}

/* Called when the canvas for the month item is size allocated.  We use this to change the canvas'
 * scrolling region and the month item's size.
 */
static void
canvas_size_allocate (GtkWidget *widget, GtkAllocation *allocation, gpointer data)
{
	gnome_canvas_item_set (month_item,
			       "width", (double) (allocation->width - 1),
			       "height", (double) (allocation->height - 1),
			       NULL);

	gnome_canvas_set_scroll_region (GNOME_CANVAS (widget),
					0, 0,
					allocation->width, allocation->height);
}

/* Returns a color spec based on the color pickers */
static char *
color_spec_from_picker (int num)
{
	int r, g, b;

	gnome_color_picker_get_i16 (GNOME_COLOR_PICKER (color_pickers[num]), &r, &g, &b, NULL);

	return build_color_spec (r, g, b);
}

/* Callback used to query prelight color information for the properties box */
static char *
fetch_prelight_spec (gpointer data)
{
	return color_spec_from_picker (COLOR_PROP_PRELIGHT_DAY_BG);
}

/* Marks fake event days in the month item sample */
static void
fake_mark_days (void)
{
	static int day_nums[] = { 1, 4, 8, 16, 17, 18, 20, 25, 28 }; /* some random days */
	int day_index;
	int i;
	GnomeCanvasItem *item;

	for (i = 0; i < (sizeof (day_nums) / sizeof (day_nums[0])); i++) {
		day_index = gnome_month_item_day2index (GNOME_MONTH_ITEM (month_item), day_nums[i]);
		item = gnome_month_item_num2child (GNOME_MONTH_ITEM (month_item), GNOME_MONTH_ITEM_DAY_BOX + day_index);
		gnome_canvas_item_set (item,
				       "fill_color", color_spec_from_picker (COLOR_PROP_MARK_DAY_BG),
				       NULL);
	}
}

/* Switches the month item to the current date and highlights the current day's number */
static void
set_current_day (void)
{
	struct tm *tm;
	time_t t;
	GnomeCanvasItem *item;
	int day_index;

	/* Set the date */

	t = time (NULL);
	tm = localtime (&t);

	gnome_canvas_item_set (month_item,
			       "year", tm->tm_year + 1900,
			       "month", tm->tm_mon,
			       NULL);

	/* Highlight current day */

	day_index = gnome_month_item_day2index (GNOME_MONTH_ITEM (month_item), tm->tm_mday);
	item = gnome_month_item_num2child (GNOME_MONTH_ITEM (month_item), GNOME_MONTH_ITEM_DAY_LABEL + day_index);
	gnome_canvas_item_set (item,
			       "fill_color", color_spec_from_picker (COLOR_PROP_CURRENT_DAY_FG),
			       "font", CURRENT_DAY_FONT,
			       NULL);
}

/* Sets the colors of the month item to the current prerences */
static void
reconfigure_month (void)
{
	/* We have to do this in two calls to gnome_canvas_item_set(), as color_spec_from_picker()
	 * returns a pointer to a static string -- and we need two values.
	 */

	gnome_canvas_item_set (month_item,
			       "heading_color", color_spec_from_picker (COLOR_PROP_HEADING_COLOR),
			       NULL);

	gnome_canvas_item_set (month_item,
			       "outline_color", color_spec_from_picker (COLOR_PROP_OUTLINE_COLOR),
			       NULL);

	gnome_canvas_item_set (month_item,
			       "day_box_color", color_spec_from_picker (COLOR_PROP_EMPTY_DAY_BG),
			       NULL);

	gnome_canvas_item_set (month_item,
			       "day_color", color_spec_from_picker (COLOR_PROP_DAY_FG),
			       NULL);

	gnome_canvas_item_set (month_item,
			       "day_font", NORMAL_DAY_FONT,
			       NULL);

	fake_mark_days ();
	set_current_day ();

	/* Reset prelighting information */

	month_item_prepare_prelight (GNOME_MONTH_ITEM (month_item), fetch_prelight_spec, NULL);
}

/* Callback used when a color is changed */
static void
color_set (void)
{
	reconfigure_month ();
	prop_changed ();
}

/* Creates the colors page in the preferences dialog */
static void
create_colors_page (void)
{
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *table;
	GtkWidget *w;
	int i;

	frame = gtk_frame_new (_("Colors for months"));
	gtk_container_border_width (GTK_CONTAINER (frame), GNOME_PAD_SMALL);
	gnome_property_box_append_page (GNOME_PROPERTY_BOX (prop_win), frame,
					gtk_label_new (_("Colors")));

	hbox = gtk_hbox_new (FALSE, GNOME_PAD);
	gtk_container_border_width (GTK_CONTAINER (hbox), GNOME_PAD_SMALL);
	gtk_container_add (GTK_CONTAINER (frame), hbox);

	table = gtk_table_new (COLOR_PROP_LAST, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), GNOME_PAD_SMALL);
	gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);

	/* Create the color pickers */

	for (i = 0; i < COLOR_PROP_LAST; i++) {
		/* Label */

		w = gtk_label_new (_(color_props[i].label));
		gtk_misc_set_alignment (GTK_MISC (w), 0.0, 0.5);
		gtk_table_attach (GTK_TABLE (table), w,
				  0, 1, i, i + 1,
				  GTK_FILL, 0,
				  0, 0);

		/* Color picker */

		color_pickers[i] = gnome_color_picker_new ();
		gnome_color_picker_set_title (GNOME_COLOR_PICKER (color_pickers[i]), _(color_props[i].label));
		gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (color_pickers[i]),
					    color_props[i].r, color_props[i].g, color_props[i].b, 0);
		gtk_table_attach (GTK_TABLE (table), color_pickers[i],
				  1, 2, i, i + 1,
				  0, 0,
				  0, 0);
		gtk_signal_connect (GTK_OBJECT (color_pickers[i]), "color_set",
				    (GtkSignalFunc) color_set,
				    NULL);
	}

	/* Create the sample calendar */

	w = gnome_canvas_new ();
	gtk_box_pack_start (GTK_BOX (hbox), w, TRUE, TRUE, 0);

	month_item = gnome_month_item_new (gnome_canvas_root (GNOME_CANVAS (w)));
	gnome_canvas_item_set (month_item,
			       "start_on_monday", week_starts_on_monday,
			       NULL);
	reconfigure_month ();
	
	gtk_signal_connect (GTK_OBJECT (w), "size_allocate",
			    canvas_size_allocate,
			    NULL);

}

/* Creates and displays the preferences dialog for the whole application */
void
properties (void)
{
	if (prop_win)
		return;

	prop_win = gnome_property_box_new ();
	gtk_window_set_title (GTK_WINDOW (prop_win), _("Preferences"));

	create_time_display_page ();
	create_colors_page ();

	gtk_signal_connect (GTK_OBJECT (prop_win), "destroy",
			    (GtkSignalFunc) prop_cancel, NULL);

	gtk_signal_connect (GTK_OBJECT (prop_win), "delete_event",
			    (GtkSignalFunc) prop_cancel, NULL);

	gtk_signal_connect (GTK_OBJECT (prop_win), "apply",
			    (GtkSignalFunc) prop_apply, NULL);

	gtk_widget_show_all (prop_win);
}

char *
build_color_spec (int r, int g, int b)
{
	static char spec[100];

	sprintf (spec, "#%04x%04x%04x", r, g, b);
	return spec;
}

void
parse_color_spec (char *spec, int *r, int *g, int *b)
{
	g_return_if_fail (spec != NULL);
	g_return_if_fail (r != NULL);
	g_return_if_fail (r != NULL);
	g_return_if_fail (r != NULL);

	if (sscanf (spec, "#%04x%04x%04x", r, g, b) != 3) {
		g_warning ("Invalid color specification %s, returning black", spec);

		*r = *g = *b = 0;
	}
}

char *
color_spec_from_prop (int propnum)
{
	return build_color_spec (color_props[propnum].r, color_props[propnum].g, color_props[propnum].b);
}
