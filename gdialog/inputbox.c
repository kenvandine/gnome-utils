/*
 *  inputbox.c -- implements the input box
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

static GtkWidget *input;

static void cancelled(GtkWidget *w, gpointer *d)
{
	exit(-1);
}

static void okayed(GtkWidget *w, int button, gpointer *d)
{
	if(button == GTK_RESPONSE_OK) {
		gchar *p = (gchar *) gtk_entry_get_text(GTK_ENTRY(input));

		write(2,p, strlen(p));
		write(2, "\n", 1);
		exit (0);
	} else if (button == GTK_RESPONSE_CANCEL)
		exit(1);
}


unsigned char dialog_input_result[MAX_LEN + 1];

/*
 * Display a dialog box for inputing a string
 */
int dialog_inputbox(const char *title, const char *prompt, int height, int width,
		    const char *init)
{
	int i, x, y, box_y, box_x, box_width;
	int input_x = 0, scroll = 0, key = 0, button = GTK_RESPONSE_NONE;
	unsigned char *instr = dialog_input_result;
	WINDOW *dialog;

	if (gnome_mode) {
		GtkWidget *w;
		GList *labellist;

		w = gtk_dialog_new_with_buttons (title, NULL,
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						GTK_STOCK_OK,
						GTK_RESPONSE_OK,
						NULL);
		gtk_dialog_set_default_response (GTK_DIALOG (w),
						 GTK_RESPONSE_OK);
		gtk_window_set_position (GTK_WINDOW (w), GTK_WIN_POS_CENTER);

		label_autowrap (GTK_DIALOG (w)->vbox, prompt, width);

		input = gtk_entry_new ();
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (w)->vbox),
				    input, TRUE, TRUE, GNOME_PAD);
		gtk_signal_connect_object (GTK_OBJECT (input), "activate",
		        GTK_SIGNAL_FUNC (gtk_window_activate_default),
			GTK_OBJECT(w));

		if(init)
			gtk_entry_set_text(GTK_ENTRY(input),init);

		if(GTK_IS_ACCESSIBLE(gtk_widget_get_accessible(input))) {
			add_atk_namedesc(input, _("GDialog Entry"), _("Enter the text"));
			labellist = gtk_container_get_children(GTK_CONTAINER(GTK_DIALOG(w)->vbox));
			add_atk_relation(GTK_WIDGET(labellist->data), input, ATK_RELATION_LABEL_FOR);
			add_atk_relation(input, GTK_WIDGET(labellist->data), ATK_RELATION_LABELLED_BY);
		}

		gtk_signal_connect(GTK_OBJECT(w), "destroy",
			GTK_SIGNAL_FUNC(cancelled), NULL);
		gtk_signal_connect(GTK_OBJECT(w), "response",
			GTK_SIGNAL_FUNC(okayed), NULL);
		gtk_widget_show_all(w);
		gtk_widget_grab_focus (input);
		gtk_main();
		return 0;
	}
	/* center dialog box on screen */
	x = (COLS - width) / 2;
	y = (LINES - height) / 2;

#ifndef NO_COLOR_CURSES
	if (use_shadow)
		draw_shadow(stdscr, y, x, height, width);
#endif
	dialog = newwin(height, width, y, x);
#ifdef WITH_GPM
	mouse_setbase(x, y);
#endif
	keypad(dialog, TRUE);

	draw_box(dialog, 0, 0, height, width, dialog_attr, border_attr);
	wattrset(dialog, border_attr);
	wmove(dialog, height - 3, 0);
	waddch(dialog, ACS_LTEE);
	for (i = 0; i < width - 2; i++)
		waddch(dialog, ACS_HLINE);
	wattrset(dialog, dialog_attr);
	waddch(dialog, ACS_RTEE);
	wmove(dialog, height - 2, 1);
	for (i = 0; i < width - 2; i++)
		waddch(dialog, ' ');

	if (title != NULL) {
		wattrset(dialog, title_attr);
		wmove(dialog, 0, (width - strlen(title)) / 2 - 1);
		waddch(dialog, ' ');
		waddstr(dialog, title);
		waddch(dialog, ' ');
	}
	wattrset(dialog, dialog_attr);
	print_autowrap(dialog, prompt, width - 2, 1, 3);

	/* Draw the input field box */
	box_width = width - 6;
	getyx(dialog, y, x);
	box_y = y + 2;
	box_x = (width - box_width) / 2;
#ifdef WITH_GPM
	mouse_mkregion(y + 1, box_x - 1, 3, box_width + 2, 'i');
#endif
	draw_box(dialog, y + 1, box_x - 1, 3, box_width + 2,
		 border_attr, dialog_attr);

	x = width / 2 - 11;
	y = height - 2;
	print_button(dialog, "Cancel", y, x + 14, FALSE);
	print_button(dialog, "  OK  ", y, x, TRUE);

	/* Set up the initial value */
	wmove(dialog, box_y, box_x);
	wattrset(dialog, inputbox_attr);
	if (!init)
		instr[0] = '\0';
	else
		strcpy(instr, init);
	input_x = strlen(instr);
	if (input_x >= box_width) {
		scroll = input_x - box_width + 1;
		input_x = box_width - 1;
		for (i = 0; i < box_width - 1; i++)
			waddch(dialog, instr[scroll + i]);
	} else
		waddstr(dialog, instr);
	wmove(dialog, box_y, box_x + input_x);

	wrefresh(dialog);
	while (key != ESC) {
		key = mouse_wgetch(dialog);

		if (button == GTK_RESPONSE_NONE) {	/* Input box selected */
			switch (key) {
			case TAB:
			case KEY_UP:
			case KEY_DOWN:
			case M_EVENT + 'i':
			case M_EVENT + 'o':
			case M_EVENT + 'c':
				break;
			case KEY_LEFT:
				continue;
			case KEY_RIGHT:
				continue;
			case KEY_BACKSPACE:
			case 127:
				if (input_x || scroll) {
					wattrset(dialog, inputbox_attr);
					if (!input_x) {
						scroll = scroll < box_width - 1 ?
						    0 : scroll - (box_width - 1);
						wmove(dialog, box_y, box_x);
						for (i = 0; i < box_width; i++)
							waddch(dialog, instr[scroll + input_x + i] ?
							       instr[scroll + input_x + i] : ' ');
						input_x = strlen(instr) - scroll;
					} else
						input_x--;
					instr[scroll + input_x] = '\0';
					wmove(dialog, box_y, input_x + box_x);
					waddch(dialog, ' ');
					wmove(dialog, box_y, input_x + box_x);
					wrefresh(dialog);
				}
				continue;
			default:
				if (key < 0x100 && isprint(key)) {
					if (scroll + input_x < MAX_LEN) {
						wattrset(dialog, inputbox_attr);
						instr[scroll + input_x] = key;
						instr[scroll + input_x + 1] = '\0';
						if (input_x == box_width - 1) {
							scroll++;
							wmove(dialog, box_y, box_x);
							for (i = 0; i < box_width - 1; i++)
								waddch(dialog, instr[scroll + i]);
						} else {
							wmove(dialog, box_y, input_x++ + box_x);
							waddch(dialog, key);
						}
						wrefresh(dialog);
					} else
						flash();	/* Alarm user about overflow */
					continue;
				}
			}
		}
		switch (key) {
		case 'O':
		case 'o':
			delwin(dialog);
			return 0;
		case 'C':
		case 'c':
			delwin(dialog);
			return 1;
		case M_EVENT + 'i':	/* mouse enter events */
		case M_EVENT + 'o':	/* use the code for 'UP' */
		case M_EVENT + 'c':
			if(key == M_EVENT + 'o')
			{
				if(key == M_EVENT + 'c')
					button = GTK_RESPONSE_OK;
				else
					button = GTK_RESPONSE_CANCEL; 
			}
			else
			{
				if(key == M_EVENT + 'c')
					button = GTK_RESPONSE_NONE;
				else
					button = GTK_RESPONSE_OK; 
			}
								
		case KEY_UP:
		case KEY_LEFT:
			switch (button) {
			case GTK_RESPONSE_NONE:
				button = GTK_RESPONSE_CANCEL;	/* Indicates "Cancel" button is selected */
				print_button(dialog, "  OK  ", y, x, FALSE);
				print_button(dialog, "Cancel", y, x + 14, TRUE);
				wrefresh(dialog);
				break;
			case GTK_RESPONSE_OK:
				button = GTK_RESPONSE_NONE;	/* Indicates input box is selected */
				print_button(dialog, "Cancel", y, x + 14, FALSE);
				print_button(dialog, "  OK  ", y, x, TRUE);
				wmove(dialog, box_y, box_x + input_x);
				wrefresh(dialog);
				break;
			case GTK_RESPONSE_CANCEL:
				button = GTK_RESPONSE_OK;/* Indicates "OK" button is selected */
				print_button(dialog, "Cancel", y, x + 14, FALSE);
				print_button(dialog, "  OK  ", y, x, TRUE);
				wrefresh(dialog);
				break;
			}
			break;
		case TAB:
		case KEY_DOWN:
		case KEY_RIGHT:
			switch (button) {
			case GTK_RESPONSE_NONE:
				button = GTK_RESPONSE_OK;/* Indicates "OK" button is selected */
				print_button(dialog, "Cancel", y, x + 14, FALSE);
				print_button(dialog, "  OK  ", y, x, TRUE);
				wrefresh(dialog);
				break;
			case GTK_RESPONSE_OK:
				button = GTK_RESPONSE_CANCEL;	/* Indicates "Cancel" button is selected */
				print_button(dialog, "  OK  ", y, x, FALSE);
				print_button(dialog, "Cancel", y, x + 14, TRUE);
				wrefresh(dialog);
				break;
			case GTK_RESPONSE_CANCEL:
				button = GTK_RESPONSE_NONE;	/* Indicates input box is selected */
				print_button(dialog, "Cancel", y, x + 14, FALSE);
				print_button(dialog, "  OK  ", y, x, TRUE);
				wmove(dialog, box_y, box_x + input_x);
				wrefresh(dialog);
				break;
			}
			break;
		case ' ':
		case '\n':
			delwin(dialog);
			if(button == GTK_RESPONSE_NONE || button == GTK_RESPONSE_OK)
				return 0;
			else if (button == GTK_RESPONSE_CANCEL)
				return 1;
		case ESC:
			break;
		}
	}

	delwin(dialog);
	return -1;		/* ESC pressed */
}
