/*
 *  dialog - Display simple dialog boxes from shell scripts
 *
 *  AUTHOR: Savio Lam (lam836@cs.cuhk.hk)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "dialog.h"

static void Usage ();

extern int dialog_yesno_with_default(const char *title, const char *prompt, int height, 
				     int width, int yesno_default);
int callback_writeerr(GtkWidget *w, gpointer *pt);

static int separate_output = 0;

typedef int (jumperFn) (const char *title, int argc, const char *const *argv);

struct Mode {
	char *name;
	int argmin, argmax, argmod;
	jumperFn *jumper;
};

jumperFn j_yesno, j_msgbox, j_infobox, j_textbox, j_menu;
jumperFn j_checklist, j_radiolist, j_inputbox, j_gauge;

/*
 * All functions are used in the slackware root disk, apart from "gauge"
 */

static struct Mode modes[] =
{
	{"--yesno", 5, 6, 1, j_yesno},
	{"--msgbox", 5, 5, 1, j_msgbox},
	{"--infobox", 5, 5, 1, j_infobox},
	{"--textbox", 5, 5, 1, j_textbox},
	{"--menu", 8, 0, 2, j_menu},
	{"--checklist", 9, 0, 3, j_checklist},
	{"--radiolist", 9, 0, 3, j_radiolist},
	{"--inputbox", 5, 6, 1, j_inputbox},
	{"--gauge", 6, 6, 1, j_gauge},
	{NULL, 0, 0, 0, NULL}
};

static struct Mode *modePtr;

int gnome_mode;

void add_atk_namedesc(GtkWidget *widget, const gchar *name, const gchar *desc)
{
	AtkObject *atk_widget;

	g_return_if_fail (GTK_IS_WIDGET(widget));
	atk_widget = gtk_widget_get_accessible(widget);

	if (name)
		atk_object_set_name(atk_widget, name);
	if (desc)
		atk_object_set_description(atk_widget, desc);
}

void add_atk_relation(GtkWidget *obj1, GtkWidget *obj2, AtkRelationType type)
{

	AtkObject *atk_obj1, *atk_obj2;
	AtkRelationSet *relation_set;
	AtkRelation *relation;

	g_return_if_fail (GTK_IS_WIDGET(obj1));
	g_return_if_fail (GTK_IS_WIDGET(obj2));

	atk_obj1 = gtk_widget_get_accessible(obj1);
	atk_obj2 = gtk_widget_get_accessible(obj2);

	relation_set = atk_object_ref_relation_set (atk_obj1);
	relation = atk_relation_new(&atk_obj2, 1, type);
	atk_relation_set_add(relation_set, relation);
	g_object_unref(G_OBJECT (relation));

}


int main(int argc, char *argv[])
{
	int offset = 0, clear_screen = 0, end_common_opts = 0, retval;
	const char *title = NULL;

	bindtextdomain(GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

	if (argc < 2) {
		Usage();
		exit(-1);
	}
	
	if(getenv("DISPLAY"))
	{
		gnome_program_init ("gdialog", VERSION, LIBGNOMEUI_MODULE,
					argc, argv,NULL);

		gnome_mode=1;
	}
	
	if (!strcmp(argv[1], "--create-rc")) {
		
#ifndef NO_COLOR_CURSES
		if (argc != 3) {
			Usage();
			exit(-1);
		}
		create_rc(argv[2]);
		return 0;
#else
		fprintf(stderr, "This option is currently unsupported "
			"on your system.\n");
		return -1;
#endif
	}

	while (offset < argc - 1 && !end_common_opts) {		/* Common options */
		if (!strcmp(argv[offset + 1], "--title")) {
			if (argc - offset < 3 || title != NULL) {
				Usage();
				exit(-1);
			} else {
				title = argv[offset + 2];
				offset += 2;
			}
		} else if (!strcmp(argv[offset + 1], "--backtitle")) {
			if (backtitle != NULL) {
				Usage();
				exit(-1);
			} else {
				backtitle = argv[offset + 2];
				offset += 2;
			}
		} else if (!strcmp(argv[offset + 1], "--separate-output")) {
			separate_output = 1;
			offset++;
		} else if (!strcmp(argv[offset + 1], "--clear")) {
			if (clear_screen) {	/* Hey, "--clear" can't appear twice! */
				Usage();
				exit(-1);
			} else if (argc == 2) {		/* we only want to clear the screen */
				init_dialog();
				if(!gnome_mode) refresh();      /* init_dialog() will clear the screen for us */
				end_dialog();
				return 0;
			} else {
				clear_screen = 1;
				offset++;
			}
                } else if(!strcmp(argv[offset + 1], "--defaultno")) {
                    offset++;
		} else		/* no more common options */
			end_common_opts = 1;
	}

	if (argc - 1 == offset) {	/* no more options */
		Usage();
		exit(-1);
	}
	/* use a table to look for the requested mode, to avoid code duplication */

	for (modePtr = modes; modePtr->name; modePtr++)		/* look for the mode */
		if (!strcmp(argv[offset + 1], modePtr->name))
			break;

	if (!modePtr->name)
		Usage();
	if (argc - offset < modePtr->argmin)
		Usage();
	if (modePtr->argmax && argc - offset > modePtr->argmax)
		Usage();
	if ((argc - offset) % modePtr->argmod)
		Usage();

/*
 * Check the range of values for height & width of dialog box for text mode.
 */
	if (((atoi(argv[offset+3]) <= 0) || (atoi(argv[offset+3]) > 25) && !gnome_mode )
            || ((atoi(argv[offset+4]) <= 0) || (atoi(argv[offset+4]) > 80) && !gnome_mode)) {
		fprintf(stderr, "\nError, values for height (1..25) & width (1..80) \n");
		exit(-1);
	}
	init_dialog();
	retval = (*(modePtr->jumper)) (title, argc - offset, (const char * const *)argv + offset);

        if(retval == -10)
        {
            Usage();
            retval = !retval;
        }
        
	if (clear_screen) {	/* clear screen before exit */
		attr_clear(stdscr, LINES, COLS, screen_attr);
		if(!gnome_mode) refresh();
	}
	end_dialog();

	exit(retval);
}

/*
 * Print program usage
 */
static void Usage()
{
	fprintf(stderr, "\
\ndialog version 0.3, by Savio Lam (lam836@cs.cuhk.hk).\
\n  patched to version %s by Stuart Herbert (S.Herbert@shef.ac.uk)\
\n\
\n* Display dialog boxes from shell scripts *\
\n\
\nUsage: gdialog --clear\
\n       gdialog --create-rc <file>\
\n       gdialog [--title <title>] [--separate-output] [--backtitle <backtitle>] [--clear] <Box options>\
\n\
\nBox options:\
\n\
\n  [--defaultno] --yesno     <text> <height> <width> [--defaultno]\
\n  --msgbox    <text> <height> <width>\
\n  --infobox   <text> <height> <width>\
\n  --inputbox  <text> <height> <width> [<init>]\
\n  --textbox   <file> <height> <width>\
\n  --menu      <text> <height> <width> <menu height> <tag1> <item1>...\
\n  --checklist <text> <height> <width> <list height> <tag1> <item1> <status1>...\
\n  --radiolist <text> <height> <width> <list height> <tag1> <item1> <status1>...\
\n  --gauge     <text> <height> <width> <percent>\n",
                VERSION);
	exit(-1);
}

/*
 * These are the program jumpers
 */

int j_yesno(const char *t, int ac, const char *const *av)
{
    if(!strcmp(av[0], "--defaultno"))
    {
        return dialog_yesno_with_default(t, av[2], atoi(av[3]),
                                         atoi(av[4]), 0);
    }
    else if(ac == 5)
    {
	return dialog_yesno_with_default(t, av[2], atoi(av[3]),
                                         atoi(av[4]), 1);
    }
    else if(!strcmp(av[5], "--defaultno"))
    {
        return dialog_yesno_with_default(t, av[2], atoi(av[3]),
                                         atoi(av[4]), 0);
    }
    else
    {
        return -10;
    }
}

int j_msgbox(const char *t, int ac, const char *const *av)
{
	return dialog_msgbox(t, av[2], atoi(av[3]), atoi(av[4]), 1);
}

int j_infobox(const char *t, int ac, const char *const *av)
{
	return dialog_msgbox(t, av[2], atoi(av[3]), atoi(av[4]), 0);
}

int j_textbox(const char *t, int ac, const char *const *av)
{
	return dialog_textbox(t, av[2], atoi(av[3]), atoi(av[4]));
}

int j_menu(const char *t, int ac, const char *const *av)
{
	int ret = dialog_menu(t, av[2], atoi(av[3]), atoi(av[4]),
			      atoi(av[5]), (ac - 6) / 2, av + 6);
	if (ret >= 0) {
		fprintf(stderr, av[6 + ret * 2]);
		return 0;
	} else if (ret == -2)
		return 1;	/* CANCEL */
	return ret;		/* ESC pressed, ret == -1 */
}

int j_checklist(const char *t, int ac, const char *const *av)
{
	return dialog_checklist(t, av[2], atoi(av[3]), atoi(av[4]),
	 atoi(av[5]), (ac - 6) / 3, av + 6, FLAG_CHECK, separate_output);
}

int j_radiolist(const char *t, int ac, const char *const *av)
{
	return dialog_checklist(t, av[2], atoi(av[3]), atoi(av[4]),
	 atoi(av[5]), (ac - 6) / 3, av + 6, FLAG_RADIO, separate_output);
}

int j_inputbox(const char *t, int ac, const char *const *av)
{
	int ret = dialog_inputbox(t, av[2], atoi(av[3]), atoi(av[4]),
				  ac == 6 ? av[5] : (char *) NULL);
	if (ret == 0)
		fprintf(stderr, dialog_input_result);
	return ret;
}

int j_gauge(const char *t, int ac, const char *const *av)
{
	return dialog_gauge(t, av[2], atoi(av[3]), atoi(av[4]),
			    atoi(av[5]));
}

#ifdef WITH_GNOME

/*
 *	Gnome Callbacks
 */
 
int callback_writeerr(GtkWidget *w, gpointer *pt)
{
	char *p=(char *)pt;
	write(2, p, strlen(p));
	write(2, "\n", 1);
	gtk_main_quit();

	return 0;
}

int callback_exitby(GtkWidget *w, gpointer *pt)
{
	gint i = GPOINTER_TO_INT(pt);
	exit(i);
}

#endif
