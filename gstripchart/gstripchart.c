/*
 * This is gstripchart.  The gstripchart program produces
 * stripchart-like graphs using a file-based parameter input mechanism
 * and a Gtk-based display mechanism.  It is a part of the Gnome
 * project, http://www.gnome.org/.
 *
 * Copyright (C) 1998,1999 John Kodis <kodis@jagunet.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */

#include "config.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <X11/Xlib.h>	/* for the ex-pee-arse-geometry routine */
#include <X11/Xutil.h>	/* for flags returned above */

#ifdef HAVE_GNOME_APPLET
#include <applet-widget.h>
#endif
#include <gnome.h>

#define NELS(a) (sizeof(a) / sizeof(*a))

#ifdef HAVE_LIBGTOP
#include <glibtop.h>
#include <glibtop/union.h>
typedef struct
{
  glibtop_cpu     cpu;
  glibtop_mem     mem;
  glibtop_swap    swap;
  glibtop_uptime  uptime;
  glibtop_loadavg loadavg;
  glibtop_netload netload;
}
gtop_struct;
#endif

static char *prog_name = "gstripchart";
static char *prog_version = "1.6";
static char *config_file = NULL;
static float chart_interval = 5.0;
static float chart_filter = 0.0;
static float slider_interval = 0.2;
static float slider_filter = 0.0;
static int include_menubar = 0;
static int include_slider = 1;
static int status_outline = 0;
static int minor_tick=0, major_tick=0;
static int geometry_flags;
static int geometry_w=200, geometry_h=50;
static int geometry_x, geometry_y;
static int root_width=1, root_height;
static int update_count = 0;
static int update_chart = 0;

/*
 * streq -- case-blind string comparison returning true on equality.
 */
static int
streq(const char *s1, const char *s2)
{
  return strcasecmp(s1, s2) == 0;
}

/*
 * isident -- ctype-style test for identifier chars.
 */
static int
isident(int c)
{
  return isalnum(c) || c=='-' || c=='_';
}

/*
 * digit_shift -- a hi_lo_fmt helper function.
 */
static void
digit_shift(char *str, char *digits, int width, int dpos)
{
  int w;
  if (++dpos <= 0)
    {
      *str++ = '.';
      for (w = 1; w < width; w++)
        *str++ = (++dpos <= 0) ? '0' : *digits++;
    }
  else
    for (w = 0; w < width; w++)
      {
        *str++ = *digits++;
        if (--dpos == 0)
          {
            *str++ = '.';
            w++;
          }
      }
}

/*
 * hi_lo_fmt -- formats a hi and lo value to the same magnitude.
 */
static void
hi_lo_fmt(double hv, char *hs, double lv, char *ls)
{
  int e, p, t, f;
  char s[100];

  sprintf(s, "% 22.16e", hv);
  e = atoi(s+20);
  s[2] = s[1]; s[1] = '0';
  t = 3 * ((e - (e < 0)) / 3);
  p = e - t;
  hs[0] = s[0];
  digit_shift(hs+1, s+2, 4, p);
  hs[6] = '\0';
  if (0 <= t && t <= 5)
    hs[5] = "\0KMGTP"[t/3];
  else
    sprintf(hs+5, "e%+d", t);

  if (ls)
    {
      sprintf(s, "% 22.16e", lv);
      f = atoi(s+20);
      s[2] = s[1]; s[1] = '0';
      p = f - t;
      ls[0] = s[0];
      digit_shift(ls+1, s+2, 4, p);
      ls[6] = '\0';
      if (0 <= t && t <= 5)
	ls[5] = "\0KMGTP"[t/3];
      else
	sprintf(ls+5, "e%+d", t);
    }
}

/* 
 * Expr -- the info required to evaluate an expression.
 *
 * The last and now arrays are doubles in order to handle values that
 * are very large integers without loss of precision.  The resultant
 * val is expected to fall in a much narrower range with less need for
 * precision.  A float is used to conserve storage space in the
 * history list. 
 */
typedef struct
{
  char *eqn_base, *eqn_src;
  double t_diff;
  int vars;

  char *s;
  jmp_buf err_jmp;
  float val;

  double *last, *now;
#ifdef HAVE_LIBGTOP
  gtop_struct *gtp_now, *gtp_last;
#endif
}
Expr;

/*
 * eval_error -- called to report an error in expression evaluation.
 *
 * Only pre-initialization errors are reported.  After initialization,
 * the expression evaluator just returns a result of zero, and keeps
 * on truckin'.
 */
static void
eval_error(Expr *e, char *msg, ...)
{
  va_list args;
  e->val = 0.0;
  if (update_chart)
    longjmp(e->err_jmp, 1);
  fflush(stdout);
  va_start(args, msg);
  fprintf(stderr, "%s: %s: ", prog_name, e->eqn_src? e->eqn_src: "");
  vfprintf(stderr, msg, args);
  fprintf(stderr, "\n");
  va_end(args);
  exit(EXIT_FAILURE);
}

/*
 * trimtb -- trims trailing whitespace from a string.
 */
static char *
trimtb(char *s)
{
  char *e = s + strlen(s) - 1;
  while (e >= s && isspace(*e))
    *e-- = '\0';
  return s;
}

/*
 * skipbl -- skips over leading whitespace in a string.
 */
static char *
skipbl(char *s)
{
  while (isspace(*s))
    s++;
  return s;
}

/*
 * stripbl -- skips some chars, then strips any leading whitespace.
 */
static void
stripbl(Expr *e, int skip)
{
  e->s = skipbl(e->s + skip);
}

#ifdef HAVE_LIBGTOP
typedef struct
{
  char *name;
  char type;	/* L=long, D=double */
  int *used;
  size_t off;
}
Gtop_data;

/*
 * Flags indicating that one or more of the libgtop values is
 * referenced, and so the corresponding libgtop routine should be
 * called on each eval cycle.
 */
static int gtop_cpu;
static int gtop_mem;
static int gtop_swap;
static int gtop_uptime;
static int gtop_loadavg;
static int gtop_netload;

#define GTOP_OFF(el) offsetof(gtop_struct, el)

Gtop_data gtop_vars[] = 
{
  /* glibtop cpu stats */
  { "cpu_total",        'L', &gtop_cpu,     GTOP_OFF(cpu.total)             },
  { "cpu_user",         'L', &gtop_cpu,     GTOP_OFF(cpu.user)              },
  { "cpu_nice",         'L', &gtop_cpu,     GTOP_OFF(cpu.nice)              },
  { "cpu_sys",          'L', &gtop_cpu,     GTOP_OFF(cpu.sys)               },
  { "cpu_idle",         'L', &gtop_cpu,     GTOP_OFF(cpu.idle)              },
  { "cpu_freq",         'L', &gtop_cpu,     GTOP_OFF(cpu.frequency)         },

  /* glibtop memory stats */
  { "mem_total",        'L', &gtop_mem,     GTOP_OFF(mem.total)             },
  { "mem_used",	        'L', &gtop_mem,     GTOP_OFF(mem.used)              },
  { "mem_free",	        'L', &gtop_mem,     GTOP_OFF(mem.free)              },
  { "mem_shared",       'L', &gtop_mem,     GTOP_OFF(mem.shared)            },
  { "mem_buffer",       'L', &gtop_mem,     GTOP_OFF(mem.buffer)            },
  { "mem_cached",       'L', &gtop_mem,     GTOP_OFF(mem.cached)            },
  { "mem_user",	        'L', &gtop_mem,     GTOP_OFF(mem.user)              },
  { "mem_locked",       'L', &gtop_mem,     GTOP_OFF(mem.locked)            },

  /* glibtop swap stats */
  { "swap_total",       'L', &gtop_swap,    GTOP_OFF(swap.total)            },
  { "swap_used",        'L', &gtop_swap,    GTOP_OFF(swap.used)             },
  { "swap_free",        'L', &gtop_swap,    GTOP_OFF(swap.free)             },
  { "swap_pagein",      'L', &gtop_swap,    GTOP_OFF(swap.pageout)          },
  { "swap_pageout",     'L', &gtop_swap,    GTOP_OFF(swap.pagein)           },

  /* glibtop uptime stats */
  { "uptime",           'D', &gtop_uptime,  GTOP_OFF(uptime.uptime)         },
  { "idletime",         'D', &gtop_uptime,  GTOP_OFF(uptime.idletime)       },

  /* glibtop loadavg stats */
  { "load_running",     'L', &gtop_loadavg, GTOP_OFF(loadavg.nr_running)    },
  { "load_tasks",       'L', &gtop_loadavg, GTOP_OFF(loadavg.nr_tasks)      },
  { "load_1m",          'D', &gtop_loadavg, GTOP_OFF(loadavg.loadavg[0])    },
  { "load_5m",          'D', &gtop_loadavg, GTOP_OFF(loadavg.loadavg[1])    },
  { "load_15m",         'D', &gtop_loadavg, GTOP_OFF(loadavg.loadavg[2])    },

  /* netload stats -- Linux-specific for the time being */
  { "net_pkts_in",      'L', &gtop_netload, GTOP_OFF(netload.packets_in)    },
  { "net_pkts_out",     'L', &gtop_netload, GTOP_OFF(netload.packets_out)   },
  { "net_pkts_total",   'L', &gtop_netload, GTOP_OFF(netload.packets_total) },

  { "net_bytes_in",     'L', &gtop_netload, GTOP_OFF(netload.bytes_in)      },
  { "net_bytes_out",    'L', &gtop_netload, GTOP_OFF(netload.bytes_out)     },
  { "net_bytes_total",  'L', &gtop_netload, GTOP_OFF(netload.bytes_total)   },

  { "net_errs_in",      'L', &gtop_netload, GTOP_OFF(netload.errors_in)     },
  { "net_errs_out",     'L', &gtop_netload, GTOP_OFF(netload.errors_out)    },
  { "net_errs_total",   'L', &gtop_netload, GTOP_OFF(netload.errors_total)  },

  /* end of array marker */
  { NULL,	         0,  NULL,          0 }
};

/*
 * gtop_value -- looks up a glibtop datum name, and returns its value.
 */
static int
gtop_value(
  char *str, int delta, 
  gtop_struct *gtp_now, gtop_struct *gtp_last, double *val)
{
  int i;
  for (i=0; gtop_vars[i].name; i++)
    {
      if (streq(str, gtop_vars[i].name))
	{
	  char *cp = ((char *)gtp_now) + gtop_vars[i].off;
	  if (update_chart == 0)
	    {
	      (*gtop_vars[i].used)++;
	      *val = 0;
	    }
	  else if (gtop_vars[i].type == 'D')
	    {
	      double df = *((double *)cp);
	      if (delta)
		{
		  cp = ((char *)gtp_last) + gtop_vars[i].off;
		  df -= *((double *)cp);
		}
	      *val = df;
	    }
	  else /* if (gtop_vars[i].type == 'L') */
	    {
	      unsigned long ul = *((unsigned long *)cp);
	      if (delta)
		{
		  cp = ((char *)gtp_last) + gtop_vars[i].off;
		  ul -= *((unsigned long *)cp);
		}
	      *val = ul;
	    }
	  return 1;
	}
    }
  return 0;
}

/*
 * get_all_netload -- a Linux-specific routine to get net load
 * information for all interfaces
 */
static void
get_all_netload(glibtop_netload *netload)
{
  FILE *fd;
  memset(netload, 0, sizeof(*netload));
  if ((fd = fopen("/proc/net/dev", "r")) != NULL)
    {
      unsigned long bytes_in, pkts_in, errs_in, bytes_out, pkts_out, errs_out;
      fscanf(fd, "%*[^\n]\n%*[^\n]\n");
      while (fscanf(fd,
	"%*[^:]:%ld%ld%ld%*d%*d%*d%*d%*d%ld%ld%ld%*d%*d%*d%*d%*d",
	&bytes_in, &pkts_in, &errs_in, &bytes_out, &pkts_out, &errs_out) == 6)
	{
	  netload->packets_in    += pkts_in;
	  netload->packets_out   += pkts_out;
	  netload->packets_total += pkts_in + pkts_out;
	  netload->bytes_in      += bytes_in;
	  netload->bytes_out     += bytes_out;
	  netload->bytes_total   += bytes_in + bytes_out;
	  netload->errors_in     += errs_in;
	  netload->errors_out    += errs_out;
	  netload->errors_total  += errs_in + errs_out;
	}
      fclose(fd);
    }
}
#endif

/*
 * add_op requires a forward prototype since it gets called from
 * num_op when recursing to evaluate a parenthesized expression.
 */
static double add_op(Expr *e);

/*
 * num_op -- evaluates numeric constants, parenthesized expressions,
 * and named variables. 
 */
static double
num_op(Expr *e)
{
  double val=0;
  if (isdigit(*e->s) || (*e->s && strchr("+-.", *e->s) && isdigit(e->s[1])))
    {
      char *r;
      val = strtod(e->s, &r);
      stripbl(e, (int)(r - e->s));
    }
  else if (*e->s == '(')
    {
      stripbl(e, 1);
      val = add_op(e);
      if (*e->s == ')')
	stripbl(e, 1);
      else
	eval_error(e, _("rparen expected"));
    }
  else if (*e->s == '$' || *e->s == '~')
    {
      int c, id_intro;
      char *idp, id[1000]; /* FIX THIS */

      id_intro = *e->s++;
      for (idp = id; isalnum(c = (*idp++ = *e->s++)) || c == '_'; )
	;
      idp[-1] = '\0';
      e->s--;

      if (isdigit(*id))
	{
	  int id_num = atoi(id);
	  if (id_num > e->vars)
	    eval_error(e, _("no such field: %d"), id_num);
	  val = e->now[id_num-1];
	  if (id_intro == '~')
	    val -= e->last[id_num-1];
	}
      else if (streq(id, "i"))	/* nominal update interval */
	{
	  val = chart_interval;
	  /* if (e->s[-1] == '~') val = 0; */
	}
      else if (streq(id, "t"))	/* delta time, in seconds */
	{
	  val = e->t_diff;
	  /* if (e->s[-1] == '~') val = 0; */
	}
      else if (streq(id, "u"))	/* update count, for debugging */
	{
	  val = update_count;
	}
      else if (streq(id, "c"))	/* chart update count, for debugging */
	{
	  val = update_chart;
	}
#ifdef HAVE_LIBGTOP
      else if (gtop_value(id, id_intro == '~', e->gtp_now, e->gtp_last, &val))
	  ; /* gtop_value handles the assignment to val */
#endif
      else if (!*id)
	eval_error(e, _("missing variable identifer"));
      else
	eval_error(e, _("invalid variable identifer: %s"), id);
      stripbl(e, 0);
    }
  else
    eval_error(e, _("number expected"));
  return val;
}

/*
 * mul_op -- evaluates multiplication, division, and remaindering.
 */
static double
mul_op(Expr *e)
{
  double val1 = num_op(e);
  while (*e->s=='*' || *e->s=='/' || *e->s=='%')
    {
      char c = *e->s;
      stripbl(e, 1);
      if (c == '*')
	val1 *= num_op(e);
      else
	{
	  double val2 = num_op(e);
	  if (val2 == 0) /* FIX THIS: there's got to be a better way. */
	    val1 = 0;
	  else if (c == '/')
	    val1 /= val2;
	  else
	    val1 = fmod(val1, val2);
	}
    }
  return val1;
}

/*
 * add_op -- evaluates addition and subtraction,
 */
static double
add_op(Expr *e)
{
  double val = mul_op(e);
  while (*e->s=='+' || *e->s=='-')
    {
      char c = *e->s;
      stripbl(e, 1);
      if (c == '+')
	val += mul_op(e);
      else
	val -= mul_op(e);
    }
  return val;
}

/*
 * eval -- sets up an Expr, then calls add_op to evaluate an expression.
 */
static double eval(
  char *eqn, char *src, double t_diff, 
#ifdef HAVE_LIBGTOP
  gtop_struct *gtp_now,
  gtop_struct *gtp_last,
#endif
  int vars, double *last, double *now )
{
  Expr e;
  e.eqn_base = e.s = eqn;
  e.eqn_src = src;
  e.vars = vars;
  e.last = last;
  e.now  = now;
  e.t_diff = t_diff;
#ifdef HAVE_LIBGTOP
  e.gtp_now = gtp_now;
  e.gtp_last = gtp_last;
#endif

  if (setjmp(e.err_jmp))
    return e.val;

  stripbl(&e, 0);
  e.val = add_op(&e);
  if (*e.s)
    eval_error(&e, _("extra gunk at end: \"%s\""), e.s);

  return e.val;
}

/*
 * Param -- struct describing a single parameter.
 */
typedef struct
{
  char active;		/* set if this parameter should be evaluated */
  char is_led;		/* set if this is an LED display, rather than a plot */
  char id_char;		/* single-char parameter name abbreviation */
  char *ident;		/* paramater name */
  char *filename;	/* the file to read this parameter from */
  char *pattern;	/* marks the line in the file for this paramater */
  char *eqn;		/* equation used to compute this paramater value */
  char *eqn_src;	/* where the eqn was defined, for error reporting */
  float hi, lo;		/* highest and lowest values expected */
  float top, bot;	/* highest and lowest display values */
  char *color_name;	/* the name of the fg color for this parameter */
  GdkColor gdk_color;	/* the rgb values for the color */
  GdkGC *gdk_gc;	/* the graphics context entry for the color */
  int num_leds;
  GdkColor *led_color;	/* the rgb values for the LED colors */
  GdkGC **led_gc;	/* the graphics context entries for the LED colors */
  int vars;		/* how many vars to read from the line in the file */
  double *last, *now;	/* the last and current vars, as enumerated above */
  float *val;		/* value history */
}
Param;

/*
 * Param_glob -- an array of Pamams, and a few common variables.
 */
typedef struct
{
  int params;
  Param **parray;
  int max_val;		/* how many vals are allocated  */
  int num_val;		/* how many vals are currently in use */
  int new_val;		/* the index of the newest val */
  double lpf_const;
  double t_diff;
  struct timeval t_last, t_now;
#ifdef HAVE_LIBGTOP
  gtop_struct gtop_now, gtop_last;
#endif
}
Param_glob;

static Param_glob chart_glob, slider_glob;

/*
 * display routines: the display variable will be set to one of these.
 */
static void no_display(void);
static void numeric_with_ident(void);
static void numeric_with_graph(void);
static void gnome_graph(void);
static void applet(void);

/*
 * The display variable points to the display proccessing routine.
 */
static void (*display)(void) = gnome_graph;

/*
 * defns_error -- reports error and exits during config file parsing.
 */
static void
defns_error(char *fn, int ln, char *fmt, ...)
{
  va_list args;
  fflush(stdout);
  va_start(args, fmt);
  fprintf(stderr, "%s: ", prog_name);
  if (fn)
    fprintf(stderr, _("%s, line %d: "), fn, ln);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
  exit(EXIT_FAILURE);
}

/* 
 * split -- breaks a 'key: val' string into two pieces, returning the
 * broken-out parts of the original string, and a count of how many
 * pieces were found, which should be 2.  This is ugly, and could
 * stand improvement. 
 */
static int
split(char *str, char **key, char **val)
{
  int p=0;
  *key = *val = NULL;
  str = skipbl(str);
  if (isalpha(*str))
    {
      p = 1;
      *key = str;
      while (isident(*str))
	str++;
      if (*str)
	{
	  *str++ = '\0';
	  while (isspace(*str))
	    str++;
	  if (*str)
	    {
	      *val = str;
	      p = 2;
	    }
	}
    }
  return p;
}

/*
 * yes_no -- evaluates yes/no response strings.
 */
static int
yes_no(char *str)
{
  if (str)
    {
      str = skipbl(str);
      if (*str == '1' || *str == 'y' || *str == 'Y')
	return 1;
      if (streq("on", str) || streq("true", str))
	return 1;
    }
  return 0;
}

/* 
 * read_param_defns -- reads parameter definitions from the
 * configuration file.  Allocates space and performs other param
 * initialization.
 */
static int
read_param_defns(Param_glob *pgp)
{
  int i, j, lineno = 0, params = 0;
  Param **p = NULL;
  char fn[FILENAME_MAX], home_fn[FILENAME_MAX], etc_fn[FILENAME_MAX];
#ifdef CONFDIR
  char conf_fn[FILENAME_MAX];
#endif
  FILE *fd;

  if (config_file)
    {
      strcpy(fn, config_file);
      if ((fd=fopen(fn, "r")) == NULL)
	defns_error(NULL, 0, _("can't open config file \"%s\""), config_file);
    }
  else
    {
      /* Try in the current working directory. */
      sprintf(fn, "%s", "gstripchart.conf");
      if ((fd=fopen(fn, "r")) == NULL)
	{
	  /* Try in the user's home directory. */
	  sprintf(fn, "%s/%s", getenv("HOME"), ".gstripchart.conf");
	  strcpy(home_fn, fn);
	  if ((fd=fopen(fn, "r")) == NULL)
	    {
	      /* Try in the /etc directory. */
	      strcpy(fn, "/etc/gstripchart.conf");
	      strcpy(etc_fn, fn);
	      if ((fd=fopen(fn, "r")) == NULL)
		{
#ifdef CONFDIR
		  /* Try in the conf directory. */
		  sprintf(fn, "%s/%s", CONFDIR, "gstripchart.conf");
		  strcpy(conf_fn, fn);
		  if ((fd=fopen(fn, "r")) == NULL)
#endif
		    {
#ifdef CONFDIR
		      defns_error(NULL, 0,
			_("can't open config file \"./gstripchart.conf\", "
			  "\"%s\", \"%s\", or \"%s\""), 
			home_fn, etc_fn, conf_fn);
#else
		      defns_error(NULL, 0,
			_("can't open config file \"./gstripchart.conf\", "
			  "\"%s\", or \"%s\""), 
			home_fn, etc_fn);
#endif
		    }
		}
	    }
	}
    }

  while (!feof(fd) && !ferror(fd))
    {
      char *bp, buf[1000]; /* FIX THIS */
      if (fgets(buf, sizeof(buf), fd))
	{
	  lineno++;
	  bp = skipbl(buf);
	  if (*bp && *bp != '#')
	    {
	      char *key, *val;
	      trimtb(bp);
	      split(bp, &key, &val);

	      /* Handle config file equivalents for some of the
                 command line options.  FIX THIS: These should be
                 restricted to the beginning of the file, before the
                 first paramater. */
	      if (streq(key, "chart-interval"))
		chart_interval = atof(val);
	      else if (streq(key, "chart-filter"))
		chart_filter = atof(val);
	      else if (streq(key, "slider-interval"))
		slider_interval = atof(val);
	      else if (streq(key, "slider-filter"))
		slider_filter = atof(val);
	      else if (streq(key, "menu"))
		include_menubar = yes_no(val);
	      else if (streq(key, "slider"))
		include_slider = yes_no(val);
	      else if (streq(key, "status-outline"))
		status_outline = yes_no(val);
	      else if (streq(key, "minor_ticks"))
		minor_tick = atoi(val);
	      else if (streq(key, "major_ticks"))
		major_tick = atoi(val);
	      /* An "identifier" or "begin" keyword introduces a new
                 parameter.  We bump the params count, and allocate
                 and initialize a new Param struct with default
                 values. */
	      else if (streq(key, "identifier") || streq(key, "begin"))
		{
		  params++;
		  p = realloc(p, params * sizeof(*p));
		  p[params-1] = malloc(sizeof(*p[params-1]));
		  p[params-1]->ident = strdup(val);
		  p[params-1]->active = 1;
		  p[params-1]->is_led = 0;
		  p[params-1]->vars = 0;
		  p[params-1]->id_char = '*';
		  p[params-1]->pattern = NULL;
		  p[params-1]->filename = NULL;
		  p[params-1]->eqn = NULL;
		  p[params-1]->eqn_src = NULL;
		  p[params-1]->color_name = NULL;
		  p[params-1]->num_leds = 0;
		  p[params-1]->led_color = NULL;
		  p[params-1]->led_gc = NULL;
		  p[params-1]->hi = p[params-1]->top = 1.0;
		  p[params-1]->lo = p[params-1]->bot = 0.0;
		}
	      else if (streq(key, "end"))
		{
		  if (!streq(p[params-1]->ident, val))
		    defns_error(fn, lineno, "found %s when expecting %s",
		      val, p[params-1]->ident);
		}
	      else if (params == 0)
		defns_error(fn, lineno,_("identifier or begin must be first"));
	      else if (streq(key, "id_char"))
		p[params-1]->id_char = val[0];
	      else if (streq(key, "color"))
		{
		  p[params-1]->color_name = strdup(val);
		  if (display == applet || display == gnome_graph)
		    if (!gdk_color_parse(val, &p[params-1]->gdk_color))
		      defns_error(fn,lineno, _("unrecognized color: %s"), val);
		}
	      else if (streq(key, "lights"))
		{
		  int n;
		  Param *pp = p[params-1];
		  char *t = strtok(val, ",");
		  for (n = 0; t != NULL; n++, t = strtok(NULL, ","))
		    {
		      char *cname = strdup(skipbl(t));
		      trimtb(cname);
		      pp->led_color = realloc(
			pp->led_color, (n+1) * sizeof(*pp->led_color));
		      if (display == applet || display == gnome_graph)
			if (!gdk_color_parse(cname, &pp->led_color[n]))
			  {
			    defns_error(
			      fn, lineno, _("unrecognized color: %s"), cname);
			  }
		      free(cname);
		    }
		  pp->num_leds = n;
		  pp->is_led = 1;
		}
	      else if (streq(key, "active"))
		p[params-1]->active = yes_no(val);
	      else if (streq(key, "filename"))
		p[params-1]->filename = strdup(val);
	      else if (streq(key, "pattern"))
		p[params-1]->pattern = strdup(val);
	      else if (streq(key, "fields"))
		p[params-1]->vars = atoi(val);
	      else if (streq(key, "equation"))
		{
		  char src[FILENAME_MAX+20];
		  sprintf(src, "%s, line %d", fn, lineno);
		  p[params-1]->eqn_src = strdup(src);
		  p[params-1]->eqn = strdup(val);
		}
	      else if (streq(key, "maximum"))
		p[params-1]->hi = p[params-1]->top = atof(val);
	      else if (streq(key, "minimum"))
		p[params-1]->lo = p[params-1]->bot = atof(val);
	      else
		defns_error(fn, lineno, _("invalid option: \"%s\""), bp);
	    }
	}
    }
  fclose(fd);

  /* Allocate space after sizes have been established. */
  for (i=0; i<params; i++)
    {
      p[i]->val  = malloc(pgp->max_val * sizeof(p[i]->val[0]));
      p[i]->last = malloc(p[i]->vars * sizeof(p[i]->last[0]));
      p[i]->now  = malloc(p[i]->vars * sizeof(p[i]->now[0]));
      /* The initial `now' values get used as the first set of `last'
         values.  Set them to zero rather than using whatever random
         cruft gets returned by malloc. */
      for (j = 0; j < p[i]->vars; j ++)
	p[i]->now[j] = 0;
    }

  pgp->params = params;
  pgp->parray = p;
  return params;
}

/* 
 * split_and_extract -- reads whitespace delimited floats into now values.
 */
static int
split_and_extract(char *str, Param *p)
{
  int i = 0;
  char *t = strtok(str, " \t:");
  while (t && i < p->vars)
    {
      p->now[i] = atof(t);
      t = strtok(NULL, " \t:");
      i++;
    }
  return i;
}

/*
 * cap -- quickly finds the next highest number of the form {1,2,5}eX.
 */
static double
cap(double x)
{
  static const double ranges[] = { .1,.2,.5, 1,2,5, 10,20,50 };

  static const double pow10[] =
  {
    1e-30, 1e-29, 1e-28, 1e-27, 1e-26, 1e-25, 1e-24, 1e-23, 1e-22, 1e-21,
    1e-20, 1e-19, 1e-18, 1e-17, 1e-16, 1e-15, 1e-14, 1e-13, 1e-12, 1e-11,
    1e-10, 1e-09, 1e-08, 1e-07, 1e-06, 1e-05, 1e-04, 1e-03, 1e-02, 1e-01,
    1e+00, 1e+01, 1e+02, 1e+03, 1e+04, 1e+05, 1e+06, 1e+07, 1e+08, 1e+09,
    1e+10, 1e+11, 1e+12, 1e+13, 1e+14, 1e+15, 1e+16, 1e+17, 1e+18, 1e+19,
    1e+20, 1e+21, 1e+22, 1e+23, 1e+24, 1e+25, 1e+26, 1e+27, 1e+28, 1e+29, 1e30
  };

  int e, f, j;
  double d;

  if (x < pow10[1] || pow10[NELS(pow10)-2] < x)
    {
      errno = EDOM;
      return x;
    }

  frexp( x, &e );		/* e gets the base 2 log of x */
  f = e * 0.90308998699194;	/* scale e by 3 / ( log(10) / log(2) ) */
  j = f % 3 + 3;		/* j gets index of nearest value in ranges */
  d = pow10[ 30 + f / 3 ];	/* d gets decade value */

  while (ranges[j] * d > x) --j;
  while (ranges[j] * d < x) ++j;

  return ranges[j] * d;
}

#ifdef HAVE_LIBGTOP
/*
 * glibtop_get_swap_occasionally -- like glibtop_get_swap, except that
 * glibtop_get_swap only gets called at most once every 15 seconds.
 * This ugly hack was put in because on Linux, reading /proc/meminfo
 * to get swap informetion takes ~ 10ms, and this was causing
 * gstripchart to chew up significant CPU time.
 */
static void
glibtop_get_swap_occasionally(glibtop_swap *swap)
{
  time_t time_now = time(NULL);
  static time_t time_last;
  static glibtop_swap swap_last;

  if ((time_now - time_last) > 15)
    {
      glibtop_get_swap(&swap_last);
      time_last = time_now;
    }
  *swap = swap_last;
}
#endif

static void
update_values(Param_glob *pgp, Param_glob *slave_pgp)
{
  int param_num, last_val_pos, next_val_pos;

  update_count++;
  pgp->t_last = pgp->t_now;
  gettimeofday(&pgp->t_now, NULL);
  pgp->t_diff = (pgp->t_now.tv_sec - pgp->t_last.tv_sec) +
    (pgp->t_now.tv_usec - pgp->t_last.tv_usec) / 1e6;
#ifdef HAVE_LIBGTOP
  pgp->gtop_last = pgp->gtop_now;
  if (gtop_cpu)
    glibtop_get_cpu(&pgp->gtop_now.cpu);
  if (gtop_mem)
    glibtop_get_mem(&pgp->gtop_now.mem);
  if (gtop_swap)
    glibtop_get_swap_occasionally(&pgp->gtop_now.swap);
  if (gtop_uptime)
    glibtop_get_uptime(&pgp->gtop_now.uptime);
  if (gtop_loadavg)
    glibtop_get_loadavg(&pgp->gtop_now.loadavg);
  if (gtop_netload)
    get_all_netload(&pgp->gtop_now.netload);
#endif

  if (update_chart < 3)
    {
      last_val_pos = 0;
      pgp->num_val = next_val_pos = 1;
    }
  else
    {
      last_val_pos = pgp->new_val;
      if (pgp->num_val < pgp->max_val)
	pgp->num_val++;
      next_val_pos = ++pgp->new_val;
      if (next_val_pos >= pgp->max_val)
	next_val_pos = pgp->new_val = 0;
    }

  for (param_num = 0; param_num < pgp->params; param_num++)
    {
      if (pgp->parray[param_num]->active)
	{
	  Param *p = pgp->parray[param_num];
	  double prev, val = 0;
	  FILE *pu = NULL;
	  char buf[1000];

	  if (p->filename)
	    {
	      if (*p->filename == '|')
		pu = popen(p->filename+1, "r");
	      else
		pu = fopen(p->filename, "r");
	    }

	  *buf = '\0';
	  if (pu)
	    {
	      fgets(buf, sizeof(buf), pu);
	      if (p->pattern)
		while (!ferror(pu) && !feof(pu) && !strstr(buf, p->pattern))
		  fgets(buf, sizeof(buf), pu);
	      if (*p->filename == '|') pclose(pu); else fclose(pu);
	    }

	  /* Copy now vals to last vals, update now vals, and compute
	     a new param value based on the new now vals.  */
	  memcpy(p->last, p->now, p->vars * sizeof(p->last[0]));
	  if ((!p->pattern || strstr(buf, p->pattern))
	      && p->vars <= split_and_extract(buf, p))
	    {
	      val = eval(
		p->eqn, p->eqn_src, pgp->t_diff,
#ifdef HAVE_LIBGTOP
		&pgp->gtop_now, &pgp->gtop_last,
#endif
		p->vars, p->last, p->now );
	    }
	  /* Low-pass filter the new val, and add to the val history. */
	  prev = update_chart < 3 ? 0 : p->val[last_val_pos];
	  p->val[next_val_pos] = prev + pgp->lpf_const * (val - prev);
	}
    }
}

/*
 * no_display -- does everything but displaying stuff.  For timing purposes.
 */
static void
no_display(void)
{
  while (1)
    {
      update_chart++;
      update_values(&chart_glob, NULL);
      usleep((int)(1e6 * chart_interval));
    }
}

/*
 * numeric_with_ident -- prints a line of numeric values.
 */
static void
numeric_with_ident(void)
{
  int p;

  while (1)
    {
      update_chart++;
      update_values(&chart_glob, NULL);

      for (p=0; p<chart_glob.params; p++)
	if (chart_glob.parray[p]->active)
	  fprintf(stdout, " %12.3f",
	    chart_glob.parray[p]->val[chart_glob.new_val]);
      fprintf(stdout, "\n");
      for (p=0; p<chart_glob.params; p++)
	if (chart_glob.parray[p]->active)
	  fprintf(stdout, " %12s", chart_glob.parray[p]->ident);
      fprintf(stdout, "\r");
      fflush(stdout);
      usleep((int)(1e6 * chart_interval));
    }
}

/*
 * numeric_with_graph -- creates a lame vertically scrolling plot.
 */
static void
numeric_with_graph(void)
{
  int p, width=76;
  char buf[79+1];

  memset(buf, ' ', sizeof(buf));
  buf[sizeof(buf)-1] = '\0';

  while (1)
    {
      update_chart++;
      update_values(&chart_glob, NULL);

      for (p = 0; p < chart_glob.params; p++)
	if (chart_glob.parray[p]->active)
	  {
	    float v = chart_glob.parray[p]->val[chart_glob.new_val];
	    float t = chart_glob.parray[p]->top;
	    float s = (t<=0) ? 0 : v / t;
	    char per[8];
	    int i = (int)((width-1) * s + 0.5);
	    buf[i] = chart_glob.parray[p]->id_char;
	    sprintf(per, "%d", (int)(100*s));
	    memcpy(buf+i+1, per, strlen(per));
	  }
      trimtb(buf);
      fprintf(stdout, "%s\n", buf);
      usleep((int)(1e6 * chart_interval));
    }
}

static int
readjust_top_for_width(int width)
{
  float hi, t, top;
  int p, n, i, v, readjust = 0;

  n = width < chart_glob.num_val ? width : chart_glob.num_val;
  for (p = 0; p < chart_glob.params; p++)
    {
      /* Find the largest value that'll be visible at this width. */
      v = chart_glob.new_val;
      t = chart_glob.parray[p]->val[v];
      for (i = 1; i < n; i++)
	{
	  if (--v < 0)
	    v = chart_glob.max_val - 1;
	  if (t < chart_glob.parray[p]->val[v])
	    t = chart_glob.parray[p]->val[v];
	}
      /* If the highest visible value of this parameter differs
         significantly from the current top value, adjust top values
         and return 1 to initiate a redraw. */
      hi = chart_glob.parray[p]->hi;
      top = hi<t ? cap(t) : hi;
      if ((fabs(chart_glob.parray[p]->top - top) / top) > 0.01)
	{
	  chart_glob.parray[p]->top = top;
	  if (include_slider)
	    slider_glob.parray[p]->top = top;
#if 0
	  printf("top[%d] %s\t= %g\t=> %g\n",
	    p, chart_glob.parray[p]->ident, t, chart_glob.parray[p]->top);
#endif
	  readjust = 1;
	}
    }
  return readjust;
}

/*
 * Gtk display handler stuff...
 */
static GdkPixmap *pixmap;
static GdkColormap *colormap;

static gint
config_handler(GtkWidget *widget, GdkEventConfigure *e, gpointer whence)
{
  int p, c, w = widget->allocation.width, h = widget->allocation.height;

#ifdef HAVE_GNOME_APPLET
  if (display == applet)
    {
      AppletWidget *app = APPLET_WIDGET(widget->parent->parent);
      GNOME_Panel_OrientType po = applet_widget_get_panel_orient(app);
      GNOME_Panel_SizeType   ps = applet_widget_get_panel_size(app);
      int fs = applet_widget_get_free_space(app);
      printf("panel: %d (%c), %d (%c), %d\n",
	po, "UDLR"[po], ps, "TNLH"[ps], fs);
    }
#endif
  /* On the initial configuration event, get the window colormap and
     allocate a color in it for each parameter color. */
  if (colormap == NULL)
    {
      colormap = gdk_window_get_colormap(widget->window);
      for (p = 0; p < chart_glob.params; p++)
	{
	  Param *cgp = chart_glob.parray[p];
	  if (cgp->is_led)
	    {
	      cgp->led_gc = malloc(cgp->num_leds * sizeof(*cgp->led_gc));
	      for (c = 0; c < cgp->num_leds; c++)
		{
		  gdk_color_alloc(colormap, &cgp->led_color[c]);
		  cgp->led_gc[c] = gdk_gc_new(widget->window);
		  gdk_gc_set_foreground(cgp->led_gc[c], &cgp->led_color[c]);
		}
	    }
	  else
	    {
	      gdk_color_alloc(colormap, &cgp->gdk_color);
	      cgp->gdk_gc = gdk_gc_new(widget->window);
	      gdk_gc_set_foreground(cgp->gdk_gc, &cgp->gdk_color);
	      if (include_slider)
		slider_glob.parray[p]->gdk_gc = cgp->gdk_gc;
	    }
	}
    }

  /* Free any previous pixmap, create a pixmap of window size and
     depth, and fill it with the window background color. */
  if (pixmap)
    gdk_pixmap_unref(pixmap);
  pixmap = gdk_pixmap_new(widget->window, w, h, -1);
  gdk_draw_rectangle(
    pixmap, widget->style->bg_gc[GTK_WIDGET_STATE(widget)], TRUE, 0,0, w,h);

  /* Adjust top values appropriately for the new chart window width. */
  readjust_top_for_width(w);

  return FALSE;
}

/*
 * overlay_tick_marks -- draws tick marks along the horizontal center
 * of the chart window at the major and minor intervals.
 */
static void
overlay_tick_marks(GtkWidget *widget, int minor, int major)
{
  int p, q, w = widget->allocation.width, c = widget->allocation.height / 2;

  if (minor)
    for (q = 1, p = w-1; p >= 0; p -= minor)
      {
	int d = 1;
	if (major && --q == 0)
	  d += 2, q = major;
	gdk_draw_line(widget->window, widget->style->black_gc,
	  p, c-d, p, c+d);
      }
}

/*
 * overlay_status_box -- draws a box in the upper-left corner of the
 * chart window. 
 */
static void
overlay_status_box(GtkWidget *widget)
{
  int p, x=0, y=0, s=10;

  for (p = 0; p < chart_glob.params; p++)
    {
      Param *cgp = chart_glob.parray[p];
      if (cgp->is_led)
	{
	  int c = cgp->val[chart_glob.new_val] + 0.5;
	  if (status_outline)
	    gdk_draw_rectangle(
	      widget->window, widget->style->black_gc, FALSE, x,y, s,s);
	  if (c > 0)
	    {
	      if (c > cgp->num_leds)
		c = cgp->num_leds;
	      gdk_draw_rectangle(
		widget->window, cgp->led_gc[c-1], TRUE, x+1,y+1, s-1,s-1);
	    }
	  x += s;
	}
    }
}

/*
 * val2y -- scales a parameter value into a y coordinate value.
 */
static int
val2y(float val, float top, int height)
{
  return height - (height * val / top);
}

static gint
chart_expose_handler(GtkWidget *widget, GdkEventExpose *event)
{
  int p, w = widget->allocation.width, h = widget->allocation.height;

  //printf("exposed: %d,%d\n", w, h);
  /* Plot as much of the value history as is available and as the
     window will hold.  Plot points from newest to oldest until we run
     out of data or the window is full. */
  for (p = chart_glob.params; p--; )
    if (chart_glob.parray[p]->active && !chart_glob.parray[p]->is_led)
      {
	float top = chart_glob.parray[p]->top;
	int n = w < chart_glob.num_val ? w : chart_glob.num_val;
	int i, j = chart_glob.new_val;
	int x0, x1 = w - 1;
	int y0, y1 = val2y(chart_glob.parray[p]->val[j], top, h);
	for (i=0; i<n; i++)
	  {
	    if (--j < 0) j = chart_glob.max_val - 1;
	    x0 = x1; y0 = y1;
	    x1 = x0 - 1;
	    y1 = val2y(chart_glob.parray[p]->val[j], top, h);
	    gdk_draw_line(pixmap, chart_glob.parray[p]->gdk_gc, x0,y0, x1,y1);
	  }
      }

  /* Draw the exposed portions of the pixmap in its window. */
  gdk_draw_pixmap(
    widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)], pixmap,
    event->area.x, event->area.y,
    event->area.x, event->area.y,
    event->area.width, event->area.height);

  overlay_tick_marks(widget, minor_tick, major_tick);
  overlay_status_box(widget);

  return FALSE;
}

static gint
chart_timer_handler(GtkWidget *widget)
{
  int p, w = widget->allocation.width, h = widget->allocation.height;
  /* Collect new parameter values.  If the scale has changed, clear
     the pixmap and fake an expose event to reload the pixmap.
     Otherwise plot each value in the RHS of the pixmap. */
  update_chart++;
  update_values(&chart_glob, include_slider? &slider_glob: NULL);
  if (readjust_top_for_width(w))
    {
      GdkEventExpose expose;
      expose.area.x = expose.area.y = 0;
      expose.area.width = w; expose.area.height = h;
      gdk_draw_rectangle(
	pixmap, widget->style->bg_gc[GTK_WIDGET_STATE(widget)],
	TRUE, 0,0, w-1,h-1);
      chart_expose_handler(widget, &expose);
    }
  else
    {
      /* Shift the pixmap one pixel left, and clear the RHS  */
      gdk_window_copy_area(
	pixmap, widget->style->fg_gc[GTK_WIDGET_STATE(widget)], 0, 0,
	pixmap, 1,0, w-1,h);
      gdk_draw_rectangle(
	pixmap, widget->style->bg_gc[GTK_WIDGET_STATE(widget)],
	TRUE, w-1,0, 1,h);
      for (p = chart_glob.params; p--; )
	if (chart_glob.parray[p]->active && !chart_glob.parray[p]->is_led)
	  {
	    float top = chart_glob.parray[p]->top;
	    int i = chart_glob.new_val;
	    int m = chart_glob.max_val;
	    int y1 = val2y(chart_glob.parray[p]->val[i], top, h);
	    int y0 = val2y(chart_glob.parray[p]->val[(i+m-1) % m], top, h);
	    gdk_draw_line(pixmap, chart_glob.parray[p]->gdk_gc,
	      w-2,y0, w-1,y1);
	  }
      gdk_draw_pixmap(
	widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)], 
	pixmap, 0,0, 0,0, w,h);
      overlay_tick_marks(widget, minor_tick, major_tick);
      overlay_status_box(widget);
    }
  return TRUE;
}

static gint
slider_redraw(GtkWidget *widget)
{
  int p, w = widget->allocation.width, h = widget->allocation.height;
  GdkGC *bg = widget->style->bg_gc[GTK_WIDGET_STATE(widget)];
  gdk_draw_rectangle(widget->window, bg, TRUE, 0,0, w,h);
  for (p = slider_glob.params; p--; )
    if (slider_glob.parray[p]->active && !chart_glob.parray[p]->is_led)
      {
	GdkPoint tri[3];
	int y = val2y(
	  slider_glob.parray[p]->val[slider_glob.new_val],
	  slider_glob.parray[p]->top, h);
	tri[0].x = 0; tri[0].y = y;
	tri[1].x = w; tri[1].y = y-w/2;
	tri[2].x = w; tri[2].y = y+w/2;
	gdk_draw_polygon(
	  widget->window, slider_glob.parray[p]->gdk_gc, TRUE, tri, 3);
      }
  return FALSE;
}

static gint
slider_expose_handler(GtkWidget *widget, GdkEventExpose *event)
{
  slider_redraw(widget);
  return FALSE;
}

static gint
slider_timer_handler(GtkWidget *widget)
{
  update_values(&slider_glob, NULL);
  slider_redraw(widget);
  return TRUE;
}

static gint
exit_callback(void)
{
  gtk_main_quit();
  return TRUE;
}

GnomeUIInfo file_menu[] =
{
  {
    GNOME_APP_UI_ITEM, N_("E_xit"), N_("Terminate the stripchart program"), 
    exit_callback, NULL, NULL, 
    GNOME_APP_PIXMAP_NONE, GNOME_STOCK_MENU_EXIT, 'x', GDK_CONTROL_MASK, NULL
  },
  GNOMEUIINFO_END
};

/*
 * about_callback -- pops up the standard "about" window.
 */
static gint
about_callback(void)
{
  const gchar *authors[] = { "John Kodis, kodis@jagunet.com", NULL };
  GtkWidget *about = gnome_about_new(
    _(prog_name), prog_version,
    _("Copyright 1998 John Kodis"),
    authors,
    _("The GNOME stripchart program plots various user-specified parameters "
      "as a function of time.  Its main use is to chart system performance "
      "parameters such as CPU load, CPU utilization, network traffic levels, "
      "and the like.  Other more ingenious uses are left as an exercise for "
      "the interested user."),
    "/usr/local/share/pixmaps/gnoapp-logo.xpm");
  gtk_widget_show(about);
  return 1;
}

GnomeUIInfo help_menu[] =
{
  {
    GNOME_APP_UI_ITEM, N_("_About"), N_("Info about the striphart program"),
    about_callback, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL
  },
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_HELP("gstripchart"),
  GNOMEUIINFO_END
};

GnomeUIInfo mainmenu[] =
{
  GNOMEUIINFO_SUBTREE(N_("_File"), file_menu),
  GNOMEUIINFO_SUBTREE(N_("_Help"), help_menu),
  GNOMEUIINFO_END
};

/*
 * help_menu_action -- pops up an instance of the Gnome help browser,
 * and points it toward the gstripchart help file.
 */
static void
help_menu_action(GtkWidget *menu)
{
  static GnomeHelpMenuEntry help_entry = { "gstripchart", "index.html" };
  gnome_help_display(NULL, &help_entry);
}

/*
 * Preferences stuff...
 */
static GtkWidget **param_active = NULL;

static void
prefs_apply(GtkWidget *button, gpointer dialog)
{
  int p;
  for (p = 0; p < chart_glob.params; p++)
    {
      chart_glob.parray[p]->active =
	GTK_TOGGLE_BUTTON(param_active[p])->active;
      if (include_slider)
	slider_glob.parray[p]->active =
	  GTK_TOGGLE_BUTTON(param_active[p])->active;
    }
}

static void
prefs_cancel(GtkWidget *button, gpointer dialog)
{
  gnome_dialog_close(GNOME_DIALOG(dialog));
}

static void
prefs_okay(GtkWidget *button, gpointer dialog)
{
  prefs_apply(button, dialog);
  prefs_cancel(button, dialog);
}

/*
 * prefs_callback -- opens a dialog window for examining and adjusting
 * chart parameters.
 */
static gint
prefs_callback(GtkWidget *chart, gpointer unused)
{
  int p;
  GtkWidget *dialog, *notebook, *vbox, *clist, *active, *label;

  notebook = gtk_notebook_new();
  param_active = realloc(param_active,
    chart_glob.params * sizeof(*param_active));
  for (p = 0; p < chart_glob.params; p++)
    {
      char lo[20], hi[20], range[100];
      char *row[2], *ttls[2];
      ttls[0] = _("Param"); ttls[1] = _("Value");

      clist = gtk_clist_new_with_titles(NELS(ttls), ttls);

      row[0] = _("Identifier"); row[1] = _(chart_glob.parray[p]->ident);
      gtk_clist_append(GTK_CLIST(clist), row);
      row[0] = _("Color"); row[1] = _(chart_glob.parray[p]->color_name);
      gtk_clist_append(GTK_CLIST(clist), row);
      row[0] = _("Filename"); row[1] = chart_glob.parray[p]->filename;
      gtk_clist_append(GTK_CLIST(clist), row);
      row[0] = _("Pattern"); row[1] = chart_glob.parray[p]->pattern;
      gtk_clist_append(GTK_CLIST(clist), row);
      row[0] = _("Equation"); row[1] = chart_glob.parray[p]->eqn;
      gtk_clist_append(GTK_CLIST(clist), row);
      row[0] = _("Expected range"); row[1] = range;
      hi_lo_fmt(chart_glob.parray[p]->lo, lo, chart_glob.parray[p]->hi, hi);
      sprintf(range, "%s ... %s", lo, hi);
      gtk_clist_append(GTK_CLIST(clist), row);
      row[0] = _("Displayed range"); row[1] = range;
      hi_lo_fmt(chart_glob.parray[p]->bot, lo, chart_glob.parray[p]->top, hi);
      sprintf(range, "%s ... %s", lo, hi);
      gtk_clist_append(GTK_CLIST(clist), row);
      row[0] = _("Current value"); row[1] = range;
      hi_lo_fmt(chart_glob.parray[p]->val[chart_glob.new_val], range, 0, NULL);
      gtk_clist_append(GTK_CLIST(clist), row);

      gtk_clist_columns_autosize(GTK_CLIST(clist));
      gtk_widget_show(clist);

      param_active[p] = active = gtk_check_button_new_with_label(_("Active"));
      gtk_toggle_button_set_active(
	GTK_TOGGLE_BUTTON(active), chart_glob.parray[p]->active);
      gtk_widget_show(active);

      vbox = gtk_vbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(vbox), clist, TRUE, TRUE, 0);
      gtk_box_pack_start(GTK_BOX(vbox), active, TRUE, TRUE, 0);
      gtk_widget_show(vbox);

      label = gtk_label_new(_(chart_glob.parray[p]->ident));
      gtk_widget_show(label);
      gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
    }
  gtk_widget_show(notebook);

  dialog = gnome_dialog_new(_("Gnome Stripchart Parameters"),
    GNOME_STOCK_BUTTON_OK, GNOME_STOCK_BUTTON_APPLY,
    GNOME_STOCK_BUTTON_CANCEL, GNOME_STOCK_BUTTON_HELP, NULL);
  gnome_dialog_set_parent(GNOME_DIALOG(dialog), GTK_WINDOW(chart));
  gnome_dialog_button_connect(GNOME_DIALOG(dialog), 0,
    GTK_SIGNAL_FUNC(prefs_okay), GNOME_DIALOG(dialog));
  gnome_dialog_button_connect(GNOME_DIALOG(dialog), 1,
    GTK_SIGNAL_FUNC(prefs_apply), GNOME_DIALOG(dialog));
  gnome_dialog_button_connect(GNOME_DIALOG(dialog), 2,
    GTK_SIGNAL_FUNC(prefs_cancel), GNOME_DIALOG(dialog));
  gnome_dialog_button_connect(GNOME_DIALOG(dialog), 3,
    GTK_SIGNAL_FUNC(help_menu_action), GNOME_DIALOG(dialog));
  gtk_container_add(GTK_CONTAINER(GNOME_DIALOG(dialog)->vbox), notebook);

  gtk_widget_show(dialog);
  return 1;
}

/*
 * text_load_clist -- fills a clist with chart ident, current, and top values.
 */
static void
text_load_clist(GtkWidget *txt, GtkWidget *box)
{
  int n;
  gtk_clist_freeze(GTK_CLIST(txt));
  gtk_clist_clear(GTK_CLIST(txt));
  for (n = 0; n < chart_glob.params; n++)
    {
      Param *p = chart_glob.parray[n];
      char val_str[100], top_str[100];
      char *row_strs[3];

      row_strs[0] = _(p->ident);
      row_strs[1] = val_str;
      row_strs[2] = top_str;
      hi_lo_fmt(p->top, top_str, p->val[chart_glob.new_val], val_str);
      gtk_clist_append(GTK_CLIST(txt), row_strs);
      gtk_clist_set_foreground(GTK_CLIST(txt), n, &p->gdk_color);
      gtk_clist_set_background(GTK_CLIST(txt), n, &box->style->bg[0]);
    }
  gtk_clist_columns_autosize(GTK_CLIST(txt));
  gtk_clist_thaw(GTK_CLIST(txt));
}

/*
 * text_update -- updates the text box values on response to a box click.
 */
static void
text_update(GtkWidget *box, GdkEvent *event, GtkWidget *txt)
{
  text_load_clist(txt, box);
}

/*
 * text_popup -- pops up a top level textual display of current
 * parameter values in response to a mouse click.
 */
static void
text_popup(GtkWidget *widget, GdkEvent *event)
{
  static GtkWidget *box, *txt;
  GdkEventButton *button = (GdkEventButton*)event;

  if (box)
    {
      gtk_widget_destroy(GTK_WIDGET(box));
      box = NULL;
    }
  else
    {
      char *titles[3];
      titles[0] = _("Param"); titles[1] = _("Current"); titles[2] = _("Top");
      txt = gtk_clist_new_with_titles(NELS(titles), titles);
      gtk_widget_show(txt);

      box = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      gtk_widget_set_style(GTK_WIDGET(box), widget->style);
      gtk_container_add(GTK_CONTAINER(box), txt);
      gtk_window_set_transient_for(GTK_WINDOW(box), GTK_WINDOW(widget));
      gtk_widget_set_uposition(GTK_WIDGET(box), button->x_root,button->y_root);
      gtk_signal_connect(GTK_OBJECT(box),
	"button_press_event", GTK_SIGNAL_FUNC(text_update), txt);
      text_load_clist(txt, widget);
      gtk_widget_show(box);
    }
}

/*
 * menu_popup -- pops up a top level menu in response to a mouse click. 
 */
static void
menu_popup(GtkWidget *widget, GdkEvent *event)
{
  static GtkWidget *menu_item, *menu;

  if (menu == NULL)
    {
      menu = gtk_menu_new();

      menu_item = gtk_menu_item_new_with_label(_("Help"));
      gtk_menu_append(GTK_MENU(menu), menu_item);
      gtk_signal_connect_object(GTK_OBJECT(menu_item), "activate",
	GTK_SIGNAL_FUNC(help_menu_action), GTK_OBJECT(widget));
      gtk_widget_show(menu_item);

      menu_item = gtk_menu_item_new_with_label(_("About"));
      gtk_menu_append(GTK_MENU(menu), menu_item);
      gtk_signal_connect_object(GTK_OBJECT(menu_item), "activate",
	GTK_SIGNAL_FUNC(about_callback), NULL);
      gtk_widget_show(menu_item);

      menu_item = gtk_menu_item_new_with_label(_("Params"));
      gtk_menu_append(GTK_MENU(menu), menu_item);
      gtk_signal_connect_object(GTK_OBJECT(menu_item), "activate",
        GTK_SIGNAL_FUNC(prefs_callback), GTK_OBJECT(widget));
      gtk_widget_show(menu_item);

      menu_item = gtk_menu_item_new_with_label(_("Exit"));
      gtk_menu_append(GTK_MENU(menu), menu_item);
      gtk_signal_connect_object(GTK_OBJECT(menu_item), "activate",
        GTK_SIGNAL_FUNC(exit_callback), NULL);
      gtk_widget_show(menu_item);
    }

  /* FIX THIS: Should we use gnome_popup_menu() instead? */
  gtk_menu_popup(
    GTK_MENU(menu), NULL, NULL, NULL, NULL, 
    ((GdkEventButton*)event)->button, ((GdkEventButton*)event)->time);
}

/*
 * click_handler -- activated on any mouse click in the chart window.
 * Creates and pops up a text box for button 1, and a menu for button 3.
 */
static void
click_handler(GtkWidget *widget, GdkEvent *event, gpointer unused)
{
  GdkEventButton *button = (GdkEventButton*)event;
  switch (button->button)
    {
    case 1: text_popup(widget, event); break;
    case 3: menu_popup(widget, event); break;
    }
}

/*
 * save_handler -- run in response to a "save-yourself" signal.
 */
static int
save_handler(GnomeClient *client,
  gint phase, GnomeRestartStyle restart,
  gint shutdown, GnomeInteractStyle interact,
  gint fast, GtkWidget *frame)
{
  int i = 0;
  char *argv[20];

  argv[i++] = program_invocation_name;
  argv[i++] = "-g";
  argv[i++] = gnome_geometry_string(frame->window);
  if (config_file)
    {
      argv[i++] = "-f";
      argv[i++] = config_file;
    }
  gnome_client_set_restart_command(client, i, argv);

  return TRUE;
}

/*
 * gnome_graph -- the display processor for the main, Gtk-based
 * graphical display. 
 */
static void
gnome_graph(void)
{
  GnomeClient *client;
  GtkWidget *frame, *h_box, *drawing;
  const int slide_w=10;		/* Set the slider width. */

  /* Create a drawing area.  Add it to the window, show it, and
     set its expose event handler. */
  drawing = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(drawing), geometry_w, geometry_h);
  gtk_widget_set_events(drawing, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
  gtk_widget_show(drawing);

  h_box = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(h_box), drawing, TRUE, TRUE, 0);

  gtk_signal_connect(GTK_OBJECT(drawing),
    "configure_event", (GtkSignalFunc)config_handler, "draw");
  gtk_signal_connect(GTK_OBJECT(drawing),
    "expose_event", (GtkSignalFunc)chart_expose_handler, NULL);
  if (include_slider)
    {
      GtkWidget *sep = gtk_vseparator_new();
      GtkWidget *slider = gtk_drawing_area_new();

      gtk_box_pack_start(GTK_BOX(h_box), sep, FALSE, TRUE, 0);
      gtk_widget_show(sep);

      gtk_drawing_area_size(GTK_DRAWING_AREA(slider), slide_w, geometry_h);
      gtk_box_pack_start(GTK_BOX(h_box), slider, FALSE, FALSE, 0);
      gtk_widget_show(slider);

      gtk_widget_set_events(slider, GDK_EXPOSURE_MASK);
      gtk_signal_connect(GTK_OBJECT(slider),
        "expose_event", (GtkSignalFunc)slider_expose_handler, NULL);

      gtk_timeout_add((int)(1000 * slider_interval),
        (GtkFunction)slider_timer_handler, slider);
    }
  gtk_widget_show(h_box);

  /* Create a top-level window. Set the title, minimum size (_usize),
     initial size (_default_size), and establish delete and destroy
     event handlers. */
  frame = gnome_app_new(_("gstripchart"), _("Gnome stripchart viewer"));
  gtk_widget_set_usize(frame, 1, 1); /* min_w, min_h */
  gtk_window_set_default_size(GTK_WINDOW(frame), geometry_w, geometry_h);
  gtk_signal_connect(GTK_OBJECT(frame),
    "destroy", GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
  gtk_signal_connect(GTK_OBJECT(frame),
    "delete_event", GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

  /* Set up the pop-up menu handler.  If a mennubar was requested, set
     that up as well.  Pack the whole works into the top-level frame. */
  gtk_signal_connect(GTK_OBJECT(frame),
    "button_press_event", GTK_SIGNAL_FUNC(click_handler), NULL);
  if (include_menubar)
    gnome_app_create_menus(GNOME_APP(frame), mainmenu);
  gnome_app_set_contents(GNOME_APP(frame), h_box);

  /* Create timer events for the drawing and slider widgets. */
  gtk_timeout_add((int)(1000 * chart_interval),
    (GtkFunction)chart_timer_handler, drawing);

  /* FIX THIS: This mess is a failed attempt to handle negative
     position specs.  It fails to accomodate the size of the window
     decorations. And its ugly.  And there's got to be a better way.
     Other than that, it's perfect.  */
  if (geometry_flags & (XNegative | YNegative))
    {
      if (XNegative)
	geometry_x = root_width  + geometry_x - geometry_w - slide_w;
      if (YNegative)
	geometry_y = root_height + geometry_y - geometry_h;
    }
  if (geometry_flags & (XValue | YValue))
    gtk_widget_set_uposition(frame, geometry_x, geometry_y);

  /* Set up session management "save-yourself" signal handler. */
  if ((client = gnome_master_client()) != NULL)
    {
      char cwd[PATH_MAX];
      getcwd(cwd, sizeof(cwd));
      gnome_client_set_current_directory(client, cwd);
      gtk_signal_connect(GTK_OBJECT(client),
        "save_yourself", GTK_SIGNAL_FUNC(save_handler), frame);
    }

  /* Show the top-level window and enter the main event loop. */
  gtk_widget_show(frame);
  gtk_main();
}

#ifdef HAVE_GNOME_APPLET
static void
change_orient_handler(AppletWidget *applet, GNOME_Panel_OrientType orient)
{
  int fs = applet_widget_get_free_space(applet);
  printf("orient: %d (%c); fs=%d\n", orient, "UDLR"[orient], fs);
}

static void
change_size_handler(AppletWidget *applet, GNOME_Panel_SizeType size)
{
  int fs = applet_widget_get_free_space(applet);
  printf("size: %d (%c); fs=%d\n", size, "TNLH"[size], fs);
}

static void
change_pos_handler(AppletWidget *applet, int x, int y)
{
  printf("pos: %d, %d\n", x, y);
}

/*
 * applet -- a display routine like gnome_graph, but in an applet
 * rather than a gnome frame. 
 */
static void
applet(void)
{
  GtkWidget *frame, *h_box, *drawing;
  const int slide_w=10;		/* Set the slider width. */

  /* Create a drawing area, show it, and set its expose event
     handler. */
  drawing = gtk_drawing_area_new();
  //gtk_drawing_area_size(GTK_DRAWING_AREA(drawing), geometry_w, geometry_h);
  gtk_widget_set_events(drawing, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
  gtk_widget_show(drawing);

  h_box = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(h_box), drawing, TRUE, TRUE, 0);

  gtk_signal_connect(GTK_OBJECT(drawing),
    "configure_event", (GtkSignalFunc)config_handler, "draw");
  gtk_signal_connect(GTK_OBJECT(drawing),
    "expose_event", (GtkSignalFunc)chart_expose_handler, NULL);

  if (include_slider)
    {
      GtkWidget *sep = gtk_vseparator_new();
      GtkWidget *slider = gtk_drawing_area_new();

      gtk_box_pack_start(GTK_BOX(h_box), sep, FALSE, TRUE, 0);
      gtk_widget_show(sep);

      gtk_drawing_area_size(GTK_DRAWING_AREA(slider), slide_w, geometry_h);
      gtk_box_pack_start(GTK_BOX(h_box), slider, FALSE, FALSE, 0);
      gtk_widget_show(slider);

      gtk_widget_set_events(slider, GDK_EXPOSURE_MASK);
      gtk_signal_connect(GTK_OBJECT(slider),
        "expose_event", (GtkSignalFunc)slider_expose_handler, NULL);

      gtk_timeout_add((int)(1000 * slider_interval),
        (GtkFunction)slider_timer_handler, slider);
    }
  gtk_widget_show(h_box);

  /* Create a top-level window. Set the title, minimum size (_usize),
     initial size (_default_size), and establish delete and destroy
     event handlers. */
  frame = applet_widget_new(prog_name);
  //gtk_widget_set_usize(frame, 1, 1); /* min_w, min_h */
  //gtk_window_set_default_size(GTK_WINDOW(frame), geometry_w, geometry_h);
  gtk_signal_connect(GTK_OBJECT(frame),
    "destroy", GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
  gtk_signal_connect(GTK_OBJECT(frame),
    "delete_event", GTK_SIGNAL_FUNC(gtk_main_quit), NULL);

  gtk_signal_connect(GTK_OBJECT(frame),
    "change_size", (GtkSignalFunc)change_size_handler, NULL);
  gtk_signal_connect(GTK_OBJECT(frame),
    "change_orient", (GtkSignalFunc)change_orient_handler, NULL);
  gtk_signal_connect(GTK_OBJECT(frame),
    "change_position", (GtkSignalFunc)change_pos_handler, NULL);

  /* Set up the pop-up menu handler.  Pack the whole works into the
     top-level applet. */
  gtk_signal_connect(GTK_OBJECT(frame),
    "button_press_event", GTK_SIGNAL_FUNC(click_handler), NULL);
  applet_widget_add(APPLET_WIDGET(frame), h_box);

  /* Create timer events for the drawing and slider widgets. */
  gtk_timeout_add((int)(1000 * chart_interval),
    (GtkFunction)chart_timer_handler, drawing);

  /* Show the top-level window and enter the main event loop. */
  gtk_widget_show(frame);
  applet_widget_gtk_main();
}
#else
static void applet(void) { gnome_graph(); }
#endif

static int
proc_arg(int opt, const char *arg)
{
  switch (opt)
    {
    case 'f': config_file = strdup(arg); break;
    case 'i': chart_interval = atof(arg); break;
    case 'I': chart_filter = atof(arg); break;
    case 'j': slider_interval = atof(arg); break;
    case 'J': slider_filter = atof(arg); break;
    case 'M': include_menubar = 1; break;
    case 'S': include_slider = 0; break;
    case 'g':
      geometry_flags = XParseGeometry(
	arg, &geometry_x, &geometry_y, &geometry_w, &geometry_h);
      break;
    case 't':
      if (streq("gtk", arg))
	display = gnome_graph;
      else if (streq("applet", arg))
	display = applet;
      else
	{
	  gnome_client_disable_master_connection();
	  if (streq("none", arg))
	    display = no_display;
	  else if (streq("text", arg))
	    display = numeric_with_ident;
	  else if (streq("graph", arg))
	    display = numeric_with_graph;
	  else
	    {
	      fprintf(stderr, _("invalid display type: %s\n"), arg);
	      return -1;
	    }
	}
    }
  return 0;
}

static void
popt_arg_extractor(
  poptContext state, enum poptCallbackReason reason,
  const struct poptOption *opt, const char *arg, void *data )
{
  if (proc_arg(opt->val, arg))
    {
      /* FIX THIS: the program name includes trailing junk, and
       * although the long options aren't shown, all of the Gnome
       * internal options are. */
      poptPrintUsage(state, stderr, 0);
      exit(EXIT_FAILURE);
    }
}

static struct
poptOption arglist[] =
{
  { NULL,             '\0', POPT_ARG_CALLBACK, popt_arg_extractor },
  { "geometry",        'g', POPT_ARG_STRING, NULL, 'g',
    N_("Geometry string: WxH+X+Y"), N_("GEO") },
  { "config-file",     'f', POPT_ARG_STRING, NULL, 'f',
    N_("Configuration file name"), N_("FILE") },
  { "chart-interval",  'i', POPT_ARG_STRING, NULL, 'i',
    N_("Chart update interval"), N_("SECS") },
  { "chart-filter",    'I', POPT_ARG_STRING, NULL, 'I',
    N_("Chart low-pass filter time constant"), N_("SECS"),  },
  { "slider-interval", 'j', POPT_ARG_STRING, NULL, 'j',
    N_("Slider update interval"), N_("SECS") },
  { "slider-filter",   'J', POPT_ARG_STRING, NULL, 'J',
    N_("Slider low-pass filter time constant"), N_("SECS") },
  { "menubar",         'M', POPT_ARG_NONE, NULL, 'M',
    N_("Adds a menubar to the main window"), NULL },
  { "omit-slider",     'S', POPT_ARG_NONE, NULL, 'S',
    N_("Omits slider window"), NULL },
  { "display-type",    't', POPT_ARG_STRING, NULL, 't',
    N_("TYPE is one of gtk, text, graph, or none"), N_("TYPE") },
  { NULL,             '\0', 0, NULL, 0 }
};

/*
 * main -- for the stripchart display program.
 */
int 
main(int argc, char **argv)
{
  int c;
  poptContext popt_context;

  /* Initialize the i18n libraries and the gtop library if it is
     linked in.  The applet_widget_init routine performs applet,
     gnome, and gtk initialization, and makes the first of two passes
     through any command line arguments. */
  bindtextdomain(PACKAGE, GNOMELOCALEDIR);
  textdomain(PACKAGE);
#ifdef HAVE_LIBGTOP
  glibtop_init_r(&glibtop_global_server, 0, 0);
#endif

  /* Make an initial scan of the command line arguments to get the
     configuration file name and the type of display requested. */
  popt_context = poptGetContext(prog_name, argc, argv, arglist, 0);
  while ((c = poptGetNextOpt(popt_context)) >= 0)
    proc_arg(c, poptGetOptArg(popt_context));

  /* Now we know the display type, and do applet initialization for
     applets and standard gnome initialization for anything else.  We
     have to do one or the other so that the color name translation
     won't dump core. */
#ifdef HAVE_GNOME_APPLET
  if (display == applet)
    applet_widget_init(prog_name, VERSION, argc, argv, arglist, 0, NULL);
#endif
  if (display == gnome_graph)
    gnome_init_with_popt_table(
      prog_name, VERSION, argc, argv, arglist, 0, NULL);
  if (display == applet || display == gnome_graph)
    gdk_window_get_geometry(
      NULL, NULL, NULL, &root_width, &root_height, NULL);

  chart_glob.max_val = root_width;
  chart_glob.new_val = chart_glob.num_val = 0;
  chart_glob.params = read_param_defns(&chart_glob);
  chart_glob.lpf_const = exp(-chart_filter / chart_interval);
  update_values(&chart_glob, NULL);

  /* Next, we re-parse the command line options so that they'll
     override any values set in the configuration file. */
  popt_context = poptGetContext(prog_name, argc, argv, arglist, 0);
  while ((c = poptGetNextOpt(popt_context)) >= 0)
    proc_arg(c, poptGetOptArg(popt_context));

  /* Clone chart_param into a new slider_param array, then override
     the next, last, and val arrays and associated sizing values. */
  if (include_slider && (display==gnome_graph || display==applet))
    {
      int p;
      slider_glob = chart_glob;
      slider_glob.max_val = 1;	/* The slider has no history whatsoever. */
      slider_glob.num_val = slider_glob.new_val = 0;
      slider_glob.parray =
	malloc(slider_glob.params * sizeof(*slider_glob.parray));
      for (p = 0; p < slider_glob.params; p++)
	{
	  slider_glob.parray[p] = malloc(sizeof(*slider_glob.parray[p]));
	  *slider_glob.parray[p] = *chart_glob.parray[p];
	  slider_glob.parray[p]->val = 
	    calloc(slider_glob.max_val,
		   sizeof(slider_glob.parray[p]->val[0]));
	  slider_glob.parray[p]->now =
	    calloc(slider_glob.parray[p]->vars,
		   sizeof(slider_glob.parray[p]->now[0]));
	  slider_glob.parray[p]->last =
	    calloc(slider_glob.parray[p]->vars,
		   sizeof(slider_glob.parray[p]->last[0]));
	}
      slider_glob.lpf_const = exp(-slider_filter / slider_interval);
      update_values(&slider_glob, NULL);
    }

  /* This is part of a none-too-satisfactory initialization sequence.
     During the prior update_chart==0 pass, the parameter equations
     are evaluated solely to detect syntax errors and to determine
     which libgtop routines should be called at the beginning of each
     update_values pass.  Any equation evaluation errors detected
     during this pass are fatal. */

  /* During the update_chart==1 pass, a set of parameter variables is
     gathered, but the resulting parameter values are not evaluated.
     This pass serves only to populate the various *_now values so
     that reasonable delta values can be computed in the next pass. */
  update_chart = 1;
  update_values(&chart_glob, NULL);

  /* During the update_chart==2 pass, the normal chart_interval time
     delta has elapsed, and can be used to compute time-dependent rate
     values.  */
  update_chart = 2;
  usleep(1000000);
  update_values(&chart_glob, NULL);

  /* During the update_chart==3 pass, each prev value of each
     parameter is considered valid, and is used in the low-pass
     filtering of this and subsequent parameter values. */
  /* update_chart = 3, set by the display() prior to update_values(). */
  usleep(1000000);
  display();

  return EXIT_SUCCESS;
}
