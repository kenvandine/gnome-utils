#include <config.h>
#include <gtk/gtk.h>

#include "gtt.h"



gint main_timer = 0;
time_t last_timer = -1;



static gint timer_func(gpointer data)
{
	time_t t;
	struct tm t1, t0;

	t = time(NULL);
	if (last_timer != -1) {
		memcpy(&t0, localtime(&last_timer), sizeof(struct tm));
		memcpy(&t1, localtime(&t), sizeof(struct tm));
		if ((t0.tm_year != t1.tm_year) ||
		    (t0.tm_yday != t1.tm_yday)) {
			project_list_time_reset();
		}
	}
	last_timer = t;
	if (!cur_proj) return 1;
	cur_proj->secs++;
	cur_proj->day_secs++;
	if (config_show_secs) {
		if (cur_proj->label) update_label(cur_proj);
	} else if (cur_proj->day_secs % 60 == 0) {
		if (cur_proj->label) update_label(cur_proj);
	}
	return 1;
}




void menu_timer_state(int);

void start_timer(void)
{
	if (main_timer) return;
	main_timer = gtk_timeout_add(1000, timer_func, NULL);
}



void stop_timer(void)
{
	if (!main_timer) return;
	gtk_timeout_remove(main_timer);
	main_timer = 0;
}

