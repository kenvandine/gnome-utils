#include <config.h>
#include <gtk/gtk.h>

#include "gtt.h"

#include <X11/Xlib.h>
#include <signal.h>


#undef DIE_ON_NORMAL_ERROR


static void die(void)
{
	fprintf(stderr, " - saving and dying\n");
	project_list_save(NULL);
	unlock_gtt();
	exit(1);
}



static void sig_handler(int signum)
{
	fprintf(stderr, "%s: Signal %d caught", APP_NAME, signum);
	die();
}


#ifdef DIE_ON_NORMAL_ERROR
static int x11_error_handler(Display *d, XErrorEvent *e)
{
	fprintf(stderr, "%s: X11 error caight", APP_NAME);
	die();
}
#endif

static int x11_io_error_handler(Display *d)
{
	fprintf(stderr, "%s: fatal X11 io error caight", APP_NAME);
	die();
	return 0; /* keep the compiler happy */
}

void err_init(void)
{
	static int inited = 0;
	
	if (inited) return;
#ifdef DIE_ON_NORMAL_ERROR
	XSetErrorHandler(x11_error_handler);
#endif
	signal(SIGINT, sig_handler);
	signal(SIGKILL, sig_handler);
	signal(SIGTERM, sig_handler);
	signal(SIGHUP, sig_handler);
	signal(SIGPIPE, sig_handler);
	XSetIOErrorHandler(x11_io_error_handler);
}

