/*
 * This is gstripchart version 1.4.  The gstripchart program produces
 * stripchart-like graphs using a file-based parameter input mechanism
 * and a Gtk-based display mechanism.  It is a part of the Gnome
 * project, http://www.gnome.org/.
 *
 * Copyright (C) 1998 John Kodis <kodis@jagunet.com>
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
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <X11/Xlib.h>	// for ex-pee-arse-geometry routine
#include <X11/Xutil.h>	// for flags returned above

#include <gnome.h>

#ifdef HAVE_LIBGTOP

#include <glibtop.h>
#include <glibtop/union.h>
#include <glibtop/parameter.h>

typedef struct
{
  glibtop_cpu		cpu;
  glibtop_mem		mem;
  glibtop_swap		swap;
  glibtop_uptime	uptime;
  glibtop_loadavg	loadavg;
}
gtop_struct;

#endif

static char *prog_name = "gstripchart";
static char *prog_version = "1.5";
static char *config_file = NULL;
static float chart_interval = 5.0;
static float chart_filter = 0.0;
static float slider_interval = 0.2;
static float slider_filter = 0.0;
static int include_menubar = 0;
static int include_slider = 1;
static int post_init = 0;
static int minor_tick=0, major_tick=0;
static int geometry_flags;
static int geometry_w=160, geometry_h=100;
static int geometry_x, geometry_y;

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
  gtop_struct *gtp;
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
  if (post_init)
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

static int gtop_cpu;
static int gtop_mem;
static int gtop_swap;
static int gtop_uptime;
static int gtop_loadavg;

#define GTOP_OFF(el) offsetof(gtop_struct, el)

Gtop_data gtop_vars[] = 
{
  /* glibtop cpu stats */
  { "cpu_total",        'L', &gtop_cpu,     GTOP_OFF(cpu.total)          },
  { "cpu_user",         'L', &gtop_cpu,     GTOP_OFF(cpu.user)           },
  { "cpu_nice",         'L', &gtop_cpu,     GTOP_OFF(cpu.nice)           },
  { "cpu_sys",          'L', &gtop_cpu,     GTOP_OFF(cpu.sys)            },
  { "cpu_idle",         'L', &gtop_cpu,     GTOP_OFF(cpu.idle)           },
  { "cpu_freq",         'L', &gtop_cpu,     GTOP_OFF(cpu.frequency)      },

  /* glibtop memory stats */
  { "mem_total",        'L', &gtop_mem,     GTOP_OFF(mem.total)          },
  { "mem_used",	        'L', &gtop_mem,     GTOP_OFF(mem.used)           },
  { "mem_free",	        'L', &gtop_mem,     GTOP_OFF(mem.free)           },
  { "mem_shared",       'L', &gtop_mem,     GTOP_OFF(mem.shared)         },
  { "mem_buffer",       'L', &gtop_mem,     GTOP_OFF(mem.buffer)         },
  { "mem_cached",       'L', &gtop_mem,     GTOP_OFF(mem.cached)         },
  { "mem_user",	        'L', &gtop_mem,     GTOP_OFF(mem.user)           },
  { "mem_locked",       'L', &gtop_mem,     GTOP_OFF(mem.locked)         },

  /* glibtop swap stats */
  { "swap_total",       'L', &gtop_swap,    GTOP_OFF(swap.total)         },
  { "swap_used",        'L', &gtop_swap,    GTOP_OFF(swap.used)          },
  { "swap_free",        'L', &gtop_swap,    GTOP_OFF(swap.free)          },
  { "swap_pagein",      'L', &gtop_swap,    GTOP_OFF(swap.pageout)       },
  { "swap_pageout",     'L', &gtop_swap,    GTOP_OFF(swap.pagein)        },

  /* glibtop uptime stats */
  { "uptime",           'D', &gtop_uptime,  GTOP_OFF(uptime.uptime)      },
  { "idletime",         'D', &gtop_uptime,  GTOP_OFF(uptime.idletime)    },

  /* glibtop loadavg stats */
  { "loadavg_running",  'L', &gtop_loadavg, GTOP_OFF(loadavg.nr_running) },
  { "loadavg_tasks",    'L', &gtop_loadavg, GTOP_OFF(loadavg.nr_tasks)   },
  { "loadavg_1m",       'D', &gtop_loadavg, GTOP_OFF(loadavg.loadavg[0]) },
  { "loadavg_5m",       'D', &gtop_loadavg, GTOP_OFF(loadavg.loadavg[1]) },
  { "loadavg_15m",      'D', &gtop_loadavg, GTOP_OFF(loadavg.loadavg[2]) },

  /* end of array marker */
  { NULL,	         0,  NULL,          0 }
};

/*
 * gtop_value -- looks up a glibtop datum name, and returns its value.
 */
static int
gtop_value(char *str, gtop_struct *gtp, double *val)
{
  int i, init = !post_init;
  
  for (i=0; gtop_vars[i].name; i++)
    {
      if (streq(str, gtop_vars[i].name))
	{
	  char *cp = ((char *)gtp) + gtop_vars[i].off;
	  if (init)
	    {
	      (*gtop_vars[i].used)++;
	      *val = 0;
	    }
	  else if (gtop_vars[i].type == 'D')
	    {
	      *val = *((double *)cp);
	    }
	  else // if (gtop_vars[i].type == 'L')
	    {
	      unsigned long ul = *((unsigned long *)cp);
	      *val = ul;
	    }
	  return 1;
	}
    }
  return 0;
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
	eval_error(e, "rparen expected");
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
	    eval_error(e, "no such field: %d", id_num);
	  stripbl(e, 0);
	  val = e->now[id_num-1];
	  if (id_intro == '~')
	    val -= e->last[id_num-1];
	}
      else if (streq(id, "i"))	/* nominal update interval */
	{
	  val = chart_interval;
	  /* if (e->s[-1] == '~') val = 0; */
	}
      else if (streq(id, "t"))	/* time of day, in seconds */
	{
	  val = e->t_diff;
	  /* if (e->s[-1] == '~') val = 0; */
	}
#ifdef HAVE_LIBGTOP
      else if (gtop_value(id, e->gtp, &val))
	  ; /* gtop_value handles the assignment to val */
#endif
      else if (!*id)
	eval_error(e, "missing variable identifer");
      else
	eval_error(e, "invalid variable identifer: %s", id);
    }
  else
    eval_error(e, "number expected");
  return val;
}

/*
 * mul_op -- evaluates multiplication, division, and remaindering.
 */
static double
mul_op(Expr *e)
{
  double val = num_op(e);
  while (*e->s=='*' || *e->s=='/' || *e->s=='%')
    {
      char c = *e->s;
      stripbl(e, 1);
      if (c == '*')
	val *= num_op(e);
      else if (c == '/')
	val /= num_op(e);
      else
	val = fmod(val, num_op(e));
    }
  return val;
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
  gtop_struct *gtp,
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
  e.gtp = gtp;
#endif

  if (setjmp(e.err_jmp))
    return e.val;

  stripbl(&e, 0);
  e.val = add_op(&e);
  if (*e.s)
    eval_error(&e, "extra gunk at end");

  return e.val;
}

/*
 * Param -- struct describing a single parameter.
 */
typedef struct
{
  char *ident;		/* paramater name */
  char id_char;		/* single-char parameter name abbreviation */
  char *color_name;	/* the name of the fg color for this parameter */
  char *filename;	/* the file to read this parameter from */
  char *pattern;	/* marks the line in the file for this paramater */
  char *eqn;		/* equation used to compute this paramater value */
  char *eqn_src;	/* where the eqn was defined, for error reporting */
  float top, bot;	/* highest and lowest values ever expected */
  GdkColor gdk_color;	/* the rgb values for the color */
  GdkGC *gdk_gc;	/* the graphics context entry for the color */
  int vars;		/* how many vars to read from the line in the file */
  double *last, *now;	/* the last and current vars, as enumerated above */
  int max_val, num_val;	/* how many vals are allocated and in use */
  int new_val;		/* index of the newest val */
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
  double lpf_const;
  double t_diff;
  struct timeval t_last, t_now;
#ifdef HAVE_LIBGTOP
  gtop_struct gtop;
#endif
}
Param_glob;

static int params;
static Param **chart_param, **slider_param;
static Param_glob chart_glob, slider_glob;

/*
 * display routines: the display variable will be set to one of these.
 */
static void no_display(void);
static void numeric_with_ident(void);
static void numeric_with_graph(void);
static void gtk_graph(void);

/*
 * The display variable points to the display proccessing routine.
 */
static void (*display)(void) = gtk_graph;

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
    fprintf(stderr, "%s, line %d: ", fn, ln);
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
read_param_defns(Param ***ppp)
{
  int i, j, lineno = 0, params = 0;
  Param **p = NULL;
  char fn[FILENAME_MAX], home_fn[FILENAME_MAX];
#ifdef CONFDIR
  char conf_fn[FILENAME_MAX];
#endif
  FILE *fd;

  *ppp = p;
  if (config_file)
    {
      strcpy(fn, config_file);
      if ((fd=fopen(fn, "r")) == NULL)
	defns_error(NULL, 0, "can't open config file \"%s\"", config_file);
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
	      if ((fd=fopen(fn, "r")) == NULL)
		{
#ifdef CONFDIR
		  /* Try in the conf directory. */
		  sprintf(fn, "%s/%s", CONFDIR, "gstripchart.conf");
		  strcpy(conf_fn, fn);
		  if ((fd=fopen(fn, "r")) == NULL)
#endif
		    {
		      defns_error(
			NULL, 0,
			"can't open config file \"%s\", \"%s\", "
#ifdef CONFDIR
			"\"%s\", "
#endif
			"or \"%s\"", 
			"./gstripchart.conf", home_fn, fn
#ifdef CONFDIR
			, conf_fn
#endif
			);
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
	      else if (streq(key, "minor_ticks"))
		minor_tick = atoi(val);
	      else if (streq(key, "major_ticks"))
		major_tick = atoi(val);
	      else if (streq(key, "type"))
		{
		  if (streq("none", val))
		    display = no_display;
		  else if (streq("text", val))
		    display = numeric_with_ident;
		  else if (streq("graph", val))
		    display = numeric_with_graph;
		  else if (streq("gtk", val))
		    display = gtk_graph;
		  else
		    defns_error(
		      fn, lineno, "invalid display type: %s", val);
		}
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
		  p[params-1]->vars = 0;
		  p[params-1]->id_char = '*';
		  p[params-1]->pattern = NULL;
		  p[params-1]->filename = NULL;
		  p[params-1]->eqn = NULL;
		  p[params-1]->eqn_src = NULL;
		  p[params-1]->color_name = NULL;
		  p[params-1]->max_val = 1024;
		  p[params-1]->new_val = 0;
		  p[params-1]->num_val = 0;
		  p[params-1]->top = 1.0;
		  p[params-1]->bot = 0.0;
		}
	      else if (streq(key, "end"))
		{
		  if (!streq(p[params-1]->ident, val))
		    defns_error(fn, lineno, "found %s when expecting %s",
		      val, p[params-1]->ident);
		}
	      else if (params == 0)
		defns_error(fn, lineno, "identifier or begin must be first");
	      else if (streq(key, "id_char"))
		p[params-1]->id_char = val[0];
	      else if (streq(key, "color"))
		{
		  p[params-1]->color_name = strdup(val);
		  if (!gdk_color_parse(val, &p[params-1]->gdk_color))
		    defns_error(fn, lineno, "unrecognized color: %s", val);
		}
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
		p[params-1]->top = atof(val);
	      else if (streq(key, "minimum"))
		p[params-1]->bot = atof(val);
	      else
		defns_error(fn, lineno, "invalid option: \"%s\"", bp);
	    }
	}
    }
  fclose(fd);

  /* Allocate space after sizes have been established. */
  for (i=0; i<params; i++)
    {
      p[i]->val  = malloc(p[i]->max_val * sizeof(p[i]->val[0]));
      p[i]->last = malloc(p[i]->vars * sizeof(p[i]->last[0]));
      p[i]->now  = malloc(p[i]->vars * sizeof(p[i]->now[0]));
      /* The initial `now' values get used as the first set of `last'
         values.  Set them to zero rather than using whatever random
         cruft gets returned by malloc. */
      for (j = 0; j < p[i]->vars; j ++)
	p[i]->now[j] = 0;
    }

  *ppp = p;
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

static void
update_values(Param_glob *pgp)
{
  int param_num;

  pgp->t_last = pgp->t_now;
  gettimeofday(&pgp->t_now, NULL);
  pgp->t_diff = (pgp->t_now.tv_sec - pgp->t_last.tv_sec) +
    (pgp->t_now.tv_usec - pgp->t_last.tv_usec) / 1e6;
#ifdef HAVE_LIBGTOP
  if (gtop_cpu)
    glibtop_get_cpu(&pgp->gtop.cpu);
  if (gtop_mem)
    glibtop_get_mem(&pgp->gtop.mem);
  if (gtop_swap)
    glibtop_get_swap(&pgp->gtop.swap);
  if (gtop_uptime)
    glibtop_get_uptime(&pgp->gtop.uptime);
  if (gtop_loadavg)
    glibtop_get_loadavg(&pgp->gtop.loadavg);
#endif
  for (param_num = 0; param_num < pgp->params; param_num++)
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
      
      if (pu)
	{
	  fgets(buf, sizeof(buf), pu);
	  if (p->pattern)
	    while (!ferror(pu) && !feof(pu) && !strstr(buf, p->pattern))
	      fgets(buf, sizeof(buf), pu);
	  if (*p->filename == '|') pclose(pu); else fclose(pu);
	}

      /* Copy now vals to last vals, update now vals, and compute a new
	 param value based on the new now vals.  */
      memcpy(p->last, p->now, p->vars * sizeof(*(p->last)));
      split_and_extract(buf, p);
      val = eval(
	p->eqn, p->eqn_src, pgp->t_diff,
#ifdef HAVE_LIBGTOP
	&pgp->gtop,
#endif
	p->vars, p->last, p->now );

      /* Put the new val into the val history. */
      prev = p->val[p->new_val];
      p->new_val = (p->new_val+1) % p->max_val;
      p->val[p->new_val] = prev + pgp->lpf_const * (val - prev);
      if (p->num_val < p->max_val)
	p->num_val++;
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
      usleep((int)(1e6 * chart_interval + 0.5));
      update_values(&chart_glob);
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
      usleep((int)(1e6 * chart_interval + 0.5));
      update_values(&chart_glob);

      for (p=0; p<params; p++)
	fprintf(stdout, " %12.3f",
		chart_param[p]->val[chart_param[p]->new_val]);
      fprintf(stdout, "\n");
      for (p=0; p<params; p++)
	fprintf(stdout, " %12s", chart_param[p]->ident);
      fprintf(stdout, "\r");
      fflush(stdout);
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
      usleep((int)(1e6 * chart_interval + 0.5));
      update_values(&chart_glob);

      for (p=0; p<params; p++)
	{
	  float v = chart_param[p]->val[chart_param[p]->new_val];
	  float t = chart_param[p]->top;
	  float s = (t<=0) ? 0 : v / t;
	  char per[8];
	  int i = (int)((width-1) * s + 0.5);
	  buf[i] = chart_param[p]->id_char;
	  sprintf(per, "%d", (int)(100*s));
	  memcpy(buf+i+1, per, strlen(per));
	}
      trimtb(buf);
      fprintf(stdout, "%s\n", buf);
    }
}

/*
 * Gtk display handler stuff...
 */
static GdkPixmap *pixmap;
static GdkColormap *colormap;

static void
destroy_handler(GtkWidget *widget, gpointer *data)
{
  gtk_main_quit();
}

static gint
cfg_handler(GtkWidget *widget, GdkEventConfigure *e, gpointer whence)
{
  int p, w = widget->allocation.width, h = widget->allocation.height;

  //printf("CFG %s: alloc=(%d,%d), req=(%d,%d)\n",
  //  (char*)whence, w, h,
  //  widget->requisition.width, widget->requisition.height);

  /* On the initial configuration event, get the window colormap and
     allocate a color in it for each parameter color. */
  if (colormap == NULL)
    {
      colormap = gdk_window_get_colormap(widget->window);
      for (p=0; p<params; p++)
	{
	  gdk_color_alloc(colormap, &chart_param[p]->gdk_color);
	  chart_param[p]->gdk_gc = gdk_gc_new(widget->window);
	  if (include_slider)
	    slider_param[p]->gdk_gc = chart_param[p]->gdk_gc;
	  gdk_gc_set_foreground(
	    chart_param[p]->gdk_gc, &chart_param[p]->gdk_color);
	  if (include_slider)
	    gdk_gc_set_foreground(
	      slider_param[p]->gdk_gc, &chart_param[p]->gdk_color);
	}
    }

  /* Free any previous pixmap, create a pixmap of window size and
     depth, and fill it with the window background color. */
  if (pixmap)
    gdk_pixmap_unref(pixmap);
  pixmap = gdk_pixmap_new(widget->window, w, h, -1);
  gdk_draw_rectangle(
    pixmap, widget->style->bg_gc[GTK_WIDGET_STATE(widget)], TRUE, 0,0, w,h);

  return 0;
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
	gdk_draw_line(
	  widget->window, widget->style->black_gc, p, c-d, p, c+d);
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

  /* Plot as much of the value history as is available and as the
     window will hold.  Plot points from newest to oldest until we run
     out of data or the window is full. */
  for (p=params; p--; )
    {
      float top = chart_param[p]->top;
      int n = w > chart_param[p]->num_val ? w : chart_param[p]->num_val;
      int i, j = chart_param[p]->new_val;
      int x0, x1 = w - 1;
      int y0, y1 = val2y(chart_param[p]->val[j], top, h);
      for (i=0; i<n; i++)
	{
          if (--j < 0) j = chart_param[p]->max_val - 1;
	  x0 = x1; y0 = y1;
	  x1 = x0 - 1;
          y1 = val2y(chart_param[p]->val[j], top, h);
	  gdk_draw_line(pixmap, chart_param[p]->gdk_gc, x0,y0, x1,y1);
	}
    }

  /* Draw the exposed portions of the pixmap in its window. */
  gdk_draw_pixmap(
    widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
    pixmap,
    event->area.x, event->area.y,
    event->area.x, event->area.y,
    event->area.width, event->area.height);

  overlay_tick_marks(widget, minor_tick, major_tick);

  return 0;
}

static gint
chart_timer_handler(GtkWidget *widget)
{
  int p, w = widget->allocation.width, h = widget->allocation.height;

  /* Shift the pixmap one pixel left, and clear the RHS  */
  gdk_window_copy_area(
    pixmap, widget->style->fg_gc[GTK_WIDGET_STATE(widget)], 0, 0,
    pixmap, 1, 0, w-1, h);
  gdk_draw_rectangle(
    pixmap, widget->style->bg_gc[GTK_WIDGET_STATE(widget)], TRUE, 
    w-1, 0, 1, h);

  /* Collect new parameter values and plot each in the RHS of the
     pixmap. */
  update_values(&chart_glob);
  for (p=params; p--; )
    {
      float top = chart_param[p]->top;
      int i = chart_param[p]->new_val;
      int m = chart_param[p]->max_val;
      int y1 = val2y(chart_param[p]->val[i], top, h);
      int y0 = val2y(chart_param[p]->val[(i+m-1) % m], top, h);
      gdk_draw_line(pixmap, chart_param[p]->gdk_gc, w-2,y0, w-1,y1);
    }

  gdk_draw_pixmap(
    widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)], 
    pixmap, 0,0, 0,0, w,h);

  overlay_tick_marks(widget, minor_tick, major_tick);

  return 1;
}

static gint
slider_redraw(GtkWidget *widget)
{
  int p, w = widget->allocation.width, h = widget->allocation.height;
  GdkGC *bg = widget->style->bg_gc[GTK_WIDGET_STATE(widget)];
  gdk_draw_rectangle(widget->window, bg, TRUE, 0,0, w-1,h-1);
  for (p=params; p--; )
    {
      GdkPoint tri[3];
      int y = val2y(
	slider_param[p]->val[slider_param[p]->new_val],
	slider_param[p]->top, h);
      tri[0].x = 1; tri[0].y = y;
      tri[1].x = w-1; tri[1].y = y-w/2;
      tri[2].x = w-1; tri[2].y = y+w/2;
      gdk_draw_polygon(widget->window, slider_param[p]->gdk_gc, TRUE, tri, 3);
    }
  //gdk_draw_line(widget->window, widget->style->black_gc, 0,0, 0,h-1);

  return 0;
}

static gint
slider_expose_handler(GtkWidget *widget, GdkEventExpose *event)
{
  slider_redraw(widget);
  return 0;
}

static gint
slider_timer_handler(GtkWidget *widget)
{
  update_values(&slider_glob);
  slider_redraw(widget);
  return 1;
}

static gint
exit_callback(void)
{
  gtk_main_quit();
  return 1;
}

GnomeUIInfo file_menu[] =
{
  {
    GNOME_APP_UI_ITEM, 
    N_("E_xit"), N_("Terminate the stripchart program"), 
    exit_callback, NULL, NULL, 
    GNOME_APP_PIXMAP_NONE, GNOME_STOCK_MENU_EXIT, 'x', GDK_CONTROL_MASK, NULL
  },
  GNOMEUIINFO_END
};

static gint
about_callback(void)
{
  const gchar *authors[] = { "John Kodis, kodis@jagunet.com", NULL };
  GtkWidget *about = gnome_about_new(
    _(prog_name), prog_version,
    _("Copyright 1998 John Kodis"),
    authors,
    "The GNOME stripchart program plots various user-specified parameters "
    "as a function of time.  Its main use is to chart system performance "
    "parameters such as CPU load, CPU utilization, network traffic levels, "
    "and the like.  Other more ingenious uses are left as an exercise for "
    "the interested user.",
    "/usr/local/share/pixmaps/gnoapp-logo.xpm");
  gtk_widget_show(about);
  return 1;
}

GnomeUIInfo help_menu[] =
{
  {
    GNOME_APP_UI_ITEM,
    N_("_About"), N_("Info about the striphart program"),
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
 * click_handler -- activated on any mouse click in the chart window.
 * Creates and pops up a menu in response to the mouse click. 
 */
static void
click_handler(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  static GtkWidget *menu=NULL, *menu_item;

  if (menu == NULL)
    {
      menu = gtk_menu_new();

      menu_item = gtk_menu_item_new_with_label("Help");
      gtk_menu_append(GTK_MENU(menu), menu_item);
      gtk_signal_connect_object(GTK_OBJECT(menu_item), "activate",
	GTK_SIGNAL_FUNC(help_menu_action), GTK_OBJECT(widget));
      gtk_widget_show(menu_item);

      menu_item = gtk_menu_item_new_with_label("About");
      gtk_menu_append(GTK_MENU(menu), menu_item);
      gtk_signal_connect_object(GTK_OBJECT(menu_item), "activate",
        GTK_SIGNAL_FUNC(about_callback), NULL);
      gtk_widget_show(menu_item);

      menu_item = gtk_menu_item_new_with_label("Exit");
      gtk_menu_append(GTK_MENU(menu), menu_item);
      gtk_signal_connect_object(GTK_OBJECT(menu_item), "activate",
        GTK_SIGNAL_FUNC(exit_callback), NULL);
      gtk_widget_show(menu_item);
    }
  // FIX THIS: Should use gnome-popup-menu() instead.
  gtk_menu_popup(
    GTK_MENU(menu), NULL, NULL, NULL, NULL, 
    ((GdkEventButton*)event)->button, ((GdkEventButton*)event)->time);
}

/*
 * gtk_graph -- the display processor for the main, Gtk-based
 * graphical display. 
 */
static void
gtk_graph(void)
{
  GtkWidget *frame, *h_box, *drawing;
  const int slide_w=10;  /* Set the slider width. */

  /* Create a top-level window. Set the title and establish delete and
     destroy event handlers. */
  frame = gnome_app_new("gstripchart", "Gnome stripchart viewer");
  gtk_signal_connect(
    GTK_OBJECT(frame), "destroy",
    GTK_SIGNAL_FUNC(destroy_handler), NULL);
  gtk_signal_connect(
    GTK_OBJECT(frame), "delete_event",
    GTK_SIGNAL_FUNC(destroy_handler), NULL);

  /* Set up the pop-up menu handler.  If a mennubar was requested, set
     that up as well. */
  gtk_signal_connect(
    GTK_OBJECT(frame), "button_press_event", 
    GTK_SIGNAL_FUNC(click_handler), NULL);
  if (include_menubar)
    gnome_app_create_menus(GNOME_APP(frame), mainmenu);

  /* Create a drawing area.  Add it to the window, show it, and
     set its expose event handler. */
  drawing = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(drawing), geometry_w, geometry_h);

  h_box = gtk_hbox_new(FALSE, 0);
  gnome_app_set_contents(GNOME_APP(frame), h_box);
  gtk_box_pack_start(GTK_BOX(h_box), drawing, TRUE, TRUE, 0);
  gtk_widget_show(drawing);

  gtk_signal_connect(GTK_OBJECT(drawing), "configure_event", 
    (GtkSignalFunc)cfg_handler, "draw");
  gtk_signal_connect(GTK_OBJECT(drawing), "expose_event", 
    (GtkSignalFunc)chart_expose_handler, NULL);
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
      gtk_signal_connect(GTK_OBJECT(slider), "expose_event", 
        (GtkSignalFunc)slider_expose_handler, NULL);

      gtk_timeout_add((int)(1000 * slider_interval),
        (GtkFunction)slider_timer_handler, slider);
    }
  gtk_widget_show(h_box);
  gtk_widget_set_events(drawing, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);

  /* Create timer events for the drawing and slider widgets. */
  gtk_timeout_add((int)(1000 * chart_interval),
    (GtkFunction)chart_timer_handler, drawing);

  /* This mess is a failed attempt to handle negative position specs.
     It fails to accomodate the size of the window decorations. And
     its ugly.  And there's got to be a better way.  Other than that,
     it's perfect.  FIX THIS */
  if (geometry_flags & (XNegative | YNegative))
  {
    int x, y, w, h, d;
    gdk_window_get_geometry(NULL, &x, &y, &w, &h, &d);
    if (XNegative)
      geometry_x = w + geometry_x - geometry_w - (include_slider? slide_w: 0);
    if (YNegative)
      geometry_y = h + geometry_y - geometry_h;
  }
  if (geometry_flags & (XValue | YValue))
    gtk_widget_set_uposition(frame, geometry_x, geometry_y);

  /* Show the top-level window and enter the main event loop. */
  gtk_widget_show(frame);
  gtk_main();
}

static int
proc_arg(int opt, const char *arg)
{
  printf("proc_arg: opt=%c, arg=%s\n", opt, arg);
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
      if (streq("none", arg))
	display = no_display;
      else if (streq("text", arg))
	display = numeric_with_ident;
      else if (streq("graph", arg))
	display = numeric_with_graph;
      else if (streq("gtk", arg))
	display = gtk_graph;
      else
	{
	  fprintf(stderr, "invalid display type: %s\n", arg);
	  return -1;
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
      // FIX THIS: the prog name includes trailing junk, and long
      // options aren't shown but all the Gnome internal options are.
      poptPrintUsage(state, stderr, 0);
      exit(EXIT_FAILURE);
    }
}

static struct
poptOption arglist[] =
{
  { NULL,              '\0', POPT_ARG_CALLBACK, popt_arg_extractor, '\0' },
  { "geometry",         'g', POPT_ARG_STRING, NULL, 'g',
    N_("GEO"), N_("geometry") },
  { "config-file",	'f', POPT_ARG_STRING, NULL, 'f',
    N_("FILE"), N_("configuration file")	   },
  { "chart-interval",	'i', POPT_ARG_STRING, NULL, 'i',
    N_("SECS"), N_("chart update interval")   },
  { "chart-filter",	'I', POPT_ARG_STRING, NULL, 'I',
    N_("SECS"), N_("chart LP filter TC")      },
  { "slider-interval",	'j', POPT_ARG_STRING, NULL, 'j',
    N_("SECS"), N_("slider update interval")  },
  { "slider-filter",	'J', POPT_ARG_STRING, NULL, 'J',
    N_("SECS"), N_("slider LP filter TC")     },
  { "menubar",		'M', POPT_ARG_NONE, NULL, 'M',
    NULL,       N_("add menubar")		   },
  { "omit-slider",	'S', POPT_ARG_NONE, NULL, 'S',
    NULL,       N_("omit slider")		   },
  { "display-type",	't', POPT_ARG_STRING, NULL, 't',
    N_("TYPE"), N_("gtk|text|graph|none")	   },
  { NULL,		'\0', 0, NULL, 0 }
};

/*
 * main -- for the stripchart display program.
 */
int 
main(int argc, char **argv)
{
  int c;
  poptContext popt_context;

  /* Initialize the i18n stuff.  If the gtop library is linked in,
     initialize that as well.  Let gnome_init initialize the Gnome and
     Gtk stuff. */
  bindtextdomain(PACKAGE, GNOMELOCALEDIR);
  textdomain(PACKAGE);
#ifdef HAVE_LIBGTOP
  glibtop_init_r(&glibtop_global_server, 0, 0);
#endif

  /* Arrange for gnome_init to parse the command line options.  The
     only option that matters here is the configuration filename.  We
     then process whatever configuration file is set, either by
     default or as specified on the command line. */
  gnome_init_with_popt_table(
    prog_name, prog_version, argc, argv, arglist, 0, &popt_context);
  params = read_param_defns(&chart_param);

  /* Next, we re-parse the command line options so that they'll
     override any values set in the configuration file. */
  popt_context = poptGetContext(prog_name, argc, argv, arglist, 0);
  while ((c = poptGetNextOpt(popt_context)) > 0)
    proc_arg(c, poptGetOptArg(popt_context));

  chart_glob.params = params;
  chart_glob.parray = chart_param;
  chart_glob.lpf_const = exp(-chart_filter / chart_interval);
  update_values(&chart_glob);

  /* Clone chart_param into a new slider_param array, then override
     the next, last, and val arrays and associated sizing values. */
  if (display == gtk_graph && include_slider)
    {
      int p;
      slider_param = malloc(params * sizeof(*slider_param));
      for (p=0; p<params; p++)
	{
	  slider_param[p] = malloc(sizeof(*slider_param[p]));
	  *slider_param[p] = *chart_param[p];
	  slider_param[p]->max_val = 1;
	  slider_param[p]->num_val = 0;
	  slider_param[p]->new_val = 0;
	  slider_param[p]->val = 
	    malloc(slider_param[p]->max_val * sizeof(slider_param[p]->val[0]));
	  slider_param[p]->now =
	    malloc(slider_param[p]->vars * sizeof(slider_param[p]->now[0]));
	  slider_param[p]->last =
	    malloc(slider_param[p]->vars * sizeof(slider_param[p]->last[0]));
	}
      slider_glob.params = params;
      slider_glob.parray = slider_param;
      slider_glob.lpf_const = exp(-slider_filter / slider_interval);
      update_values(&slider_glob);
    }

  post_init = 1;
  if (display)
    (*display)();

  return EXIT_SUCCESS;
}
