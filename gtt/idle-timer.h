/* idle-timer.h -- detect user inactivity 
 * Originally taken from:
 * xscreensaver, Copyright (c) 1993-2001 Jamie Zawinski <jwz@jwz.org>
 * hacked for use with gtt, Copyright (c) 2001 Linas Vepstas <linas@linas.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

#ifndef __IDLE_TIMER_H__
#define __IDLE_TIMER_H__

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <string.h>
#include <stdio.h>

typedef struct IdleTimeout IdleTimeout;
typedef struct pref_s IdleTimeoutPrefs;
typedef struct IdleTimeoutScreen IdleTimeoutScreen;

struct pref_s
{
	Time timeout;
	Time pointer_timeout;
	Time notice_events_timeout;
};


/* This structure holds all the data that applies to the program as a whole,
   or to the non-screen-specific parts of the display connection.
 */
struct IdleTimeout {
  IdleTimeoutPrefs prefs;

  int nscreens;
  IdleTimeoutScreen *screens;
  IdleTimeoutScreen *default_screen;	/* ...on which dialogs will appear. */

  Display *dpy;

  Bool using_xidle_extension;	   /* which extension is being used.         */
  Bool using_mit_saver_extension;  /* Note that `p->use_*' is the *request*, */
  Bool using_sgi_saver_extension;  /* and `si->using_*' is the *reality*.    */
  Bool using_proc_interrupts;

# ifdef HAVE_MIT_SAVER_EXTENSION
  int mit_saver_ext_event_number;
  int mit_saver_ext_error_number;
# endif
# ifdef HAVE_SGI_SAVER_EXTENSION
  int sgi_saver_ext_event_number;
  int sgi_saver_ext_error_number;
# endif

  guint timer_id;		/* Timer to implement `prefs.timeout' */
  guint check_pointer_timer_id;	/* `prefs.pointer_timeout' */

  time_t last_activity_time;		   /* Used only when no server exts. */
  time_t last_wall_clock_time;             /* Used to detect laptop suspend. */
  IdleTimeoutScreen *last_activity_screen;

  Bool emergency_lock_p;        /* Set when the wall clock has jumped
                                   (presumably due to laptop suspend) and we
                                   need to lock down right away instead of
                                   waiting for the lock timer to go off. */

};


/* This structure holds all the data that applies to the screen-specific parts
   of the display connection; if the display has multiple screens, there will
   be one of these for each screen.
 */
struct IdleTimeoutScreen 
{
  IdleTimeout *global;

  Screen *screen;
  // Widget toplevel_shell;

  int poll_mouse_last_root_x;		/* Used only when no server exts. */
  int poll_mouse_last_root_y;
  Window poll_mouse_last_child;
  unsigned int poll_mouse_last_mask;
};



/* =======================================================================
   server extensions and virtual roots
   ======================================================================= */

#ifdef HAVE_MIT_SAVER_EXTENSION
extern Bool query_mit_saver_extension (IdleTimeout *);
#endif
#ifdef HAVE_SGI_SAVER_EXTENSION
extern Bool query_sgi_saver_extension (IdleTimeout *);
#endif
#ifdef HAVE_XIDLE_EXTENSION
extern Bool query_xidle_extension (IdleTimeout *);
#endif
#ifdef HAVE_PROC_INTERRUPTS
extern Bool query_proc_interrupts_available (IdleTimeout *, const char **why);
#endif

/* return number of seconds that system has been idle */
IdleTimeout * idle_timeout_new (void);
int poll_idle_time (IdleTimeout *si, Bool until_idle_p);


#endif /* __IDLE_TIMER_H__ */
