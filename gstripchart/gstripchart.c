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
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include <ctype.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <config.h>
#include <gnome.h>

static char *prog_name = "gstripchart";
static char *prog_version = "1.4";
static char *config_file = NULL;
static float chart_interval = 5.0;
static float slider_interval = 0.2;
static int include_menu = 1;
static int include_slider = 1;
static int post_init = 0;

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
  double *last, *now;

  char *s;
  jmp_buf err_jmp;
  float val;
}
Expr;

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

static char *
trimtb(char *s)
{
  char *e = s + strlen(s) - 1;
  while (e >= s && isspace(*e))
    *e-- = '\0';
  return s;
}

static char *
skipbl(char *s)
{
  while (isspace(*s))
    s++;
  return s;
}

static void
stripbl(Expr *e, int skip)
{
  e->s = skipbl(e->s + skip);
}

static double
add_op(Expr *e);

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
      if (isdigit(e->s[1]))
	{
	  int i = 0, c = *e->s;
	  for (e->s++; isdigit(*e->s); e->s++)
	    i = i * 10 + *e->s-'0';
	  if (i > e->vars)
	    eval_error(e, "no such field: %d", i);
	  stripbl(e, 0);
	  val = e->now[i-1];
	  if (c == '~')
	    val -= e->last[i-1];
	}
      else
	{
	  switch (*++e->s)
	    {
	    case 'i':	/* interval, in seconds */
	      val = chart_interval;
	      /* if (e->s[-1] == '~') val = 0; */
	      e->s++;
	      break;
	    case 't':	/* time of day, in seconds */
	      val = e->t_diff;
	      /* if (e->s[-1] == '~') val = 0; */
	      e->s++;
	      break;
	    case '\0':
	      eval_error(e, "missing variable identifer");
	      break;
	    default:
	      eval_error(e, "invalid variable identifer");
	    }
	}
    }
  else
    eval_error(e, "number expected");
  return val;
}

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

static double
eval(char *eqn, char *src, double t_diff, int vars, double *last, double *now)
{
  Expr e;
  e.eqn_base = e.s = eqn;
  e.eqn_src = src;
  e.vars = vars;
  e.last = last;
  e.now  = now;
  e.t_diff = t_diff;

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

typedef struct
{
  int params;
  Param **parray;
  double t_diff;
  struct timeval t_last, t_now;
}
Param_glob;

static int params;
static Param **chart_param, **slider_param;
static Param_glob chart_glob, slider_glob;

static void (*display)(void); /* routine called to redisplay new parameters */

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
 * read_param_defns -- reads parameter definitions from the
 * configuration file.  Allocates space and performs other param
 * initialization.
 */
static int
read_param_defns(Param ***ppp)
{
  int i, lineno = 0, params = 0;
  Param **p = NULL;
  char fn[FILENAME_MAX], home_fn[FILENAME_MAX];
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
		defns_error(NULL, 0,
		  "can't open config file \"%s\", \"%s\", or \"%s\"", 
		  "gstripchart.conf", home_fn, fn);
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
	      /* An "identifier" keyword introduces a new parameter.
                 We bump the params count, and allocate and initialize
                 a new Param struct with default values. */
	      if (streq(key, "identifier"))
		{
		  params ++;
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
	      else if (params == 0)
		defns_error(fn, lineno, "ident must be first");
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
      p[i]->now  = malloc(p[i]->vars * sizeof(p[i]->now[0]));
      p[i]->last = malloc(p[i]->vars * sizeof(p[i]->last[0]));
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
  char *t = strtok(str, " \t");
  while (t && i < p->vars)
    {
      p->now[i] = atof(t);
      t = strtok(NULL, " \t");
      i++;
    }
  return i;
}

static float
get_value(double dt, Param *p)
{
  double val = 0;
  FILE *pu = NULL;
  char buf[1000];

  if (p->filename)
    if (*p->filename == '|')
      pu = popen(p->filename+1, "r");
    else
      pu = fopen(p->filename, "r");
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
  val = eval(p->eqn, p->eqn_src, dt, p->vars, p->last, p->now);

  /* Put the new val into the val history. */
  p->new_val = (p->new_val+1) % p->max_val;
  p->val[p->new_val] = val;
  if (p->num_val < p->max_val)
    p->num_val++;

  return val;
}

static void
update_values(Param_glob *pgp)
{
  int p;

  pgp->t_last = pgp->t_now;
  gettimeofday(&pgp->t_now, NULL);
  pgp->t_diff = (pgp->t_now.tv_sec - pgp->t_last.tv_sec) +
    (pgp->t_now.tv_usec - pgp->t_last.tv_usec) / 1e6;

  for (p=0; p<pgp->params; p++)
    get_value(pgp->t_diff, pgp->parray[p]);
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
    N_("Exit"), N_("Terminate the stripchart program"), 
    exit_callback, NULL, NULL, 
    GNOME_APP_PIXMAP_NONE, GNOME_STOCK_MENU_EXIT, 0, 0, NULL
  },
  GNOMEUIINFO_END
};

static gint
about_callback(void)
{
  gchar *authors[] = { "John Kodis, kodis@jagunet.com", NULL };
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
    N_("About"), N_("Info about the striphart program"),
    about_callback, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL
  },
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_HELP("gstripchart"),
  GNOMEUIINFO_END
};

GnomeUIInfo mainmenu[] =
{
  GNOMEUIINFO_SUBTREE(N_("File"), file_menu),
  GNOMEUIINFO_SUBTREE(N_("Help"), help_menu),
  GNOMEUIINFO_END
};

static void
gtk_graph(void)
{
  GtkWidget *frame, *h_box, *drawing;
  /* Set the nominal drawing window and slider sizes. */
  const int nom_w=160, nom_h=100, slide_w=10;

  /* Create a top-level window. Set the title and establish delete and
     destroy event handlers. */
  frame = gnome_app_new("gstripchart", "Gnome stripchart viewer");
  gtk_signal_connect(
    GTK_OBJECT(frame), "destroy",
    GTK_SIGNAL_FUNC(destroy_handler), NULL);
  gtk_signal_connect(
    GTK_OBJECT(frame), "delete_event",
    GTK_SIGNAL_FUNC(destroy_handler), NULL);
  if (include_menu)
    gnome_app_create_menus(GNOME_APP(frame), mainmenu);

  /* Create a drawing area.  Add it to the window, show it, and
     set its expose event handler. */
  drawing = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(drawing), nom_w, nom_h);

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

      gtk_drawing_area_size(GTK_DRAWING_AREA(slider), slide_w, nom_h);
      gtk_box_pack_start(GTK_BOX(h_box), slider, FALSE, FALSE, 0);
      gtk_widget_show(slider);

      gtk_widget_set_events(slider, GDK_EXPOSURE_MASK);
      gtk_signal_connect(GTK_OBJECT(slider), "expose_event", 
        (GtkSignalFunc)slider_expose_handler, NULL);

      gtk_timeout_add((int)(1000 * slider_interval),
        (GtkFunction)slider_timer_handler, slider);
    }
  gtk_widget_show(h_box);
  gtk_widget_set_events(drawing, GDK_EXPOSURE_MASK);

  /* Create timer events for the drawing and slider widgets. */
  gtk_timeout_add((int)(1000 * chart_interval),
    (GtkFunction)chart_timer_handler, drawing);

  /* Show the top-level window, set its minimum size, and enter the
     main event loop. */
  //gtk_window_set_policy(GTK_WINDOW(frame), TRUE, TRUE, TRUE);
  //gtk_widget_set_usize(frame, 300,200);
  gtk_widget_show(frame);
  //gdk_window_set_hints(
  //  frame->window, 0,0,  nom_w/2,nom_h/2, 0,0, GDK_HINT_MIN_SIZE);
  gtk_main();
}

static error_t
arg_proc(int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'f':
      config_file = arg;      
      return 0;
    case 'i':
      chart_interval = atof(arg);
      return 0;
    case 'j':
      slider_interval = atof(arg);
      return 0;
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
	  return ARGP_ERR_UNKNOWN; /* FIX THIS */
	}
      return 0;
    case 'M':
      include_menu = 0;
      return 0;
    case 'S':
      include_slider = 0;
      return 0;
    default:
      return ARGP_ERR_UNKNOWN;
    }
}

static struct argp_option arglist[] =
{
  { "config-file",     'f', N_("FILE"), 0, N_("configuration file"),     1 },
  { "chart-interval",  'i', N_("SECS"), 0, N_("chart update interval"),  1 },
  { "slider-interval", 'j', N_("SECS"), 0, N_("slider update interval"), 1 },
  { "display-type",    't', N_("TYPE"), 0, N_("type of display"),        1 },
  { "omit-menu",       'M', NULL,       0, N_("omit menu"),              0 },
  { "omit-slider",     'S', NULL,       0, N_("omit slider"),            0 },
  { NULL,               0,  NULL,       0, NULL,                         0 }
};

static struct argp argp_opts =
{
  arglist, arg_proc, NULL, NULL, NULL, NULL, NULL
};

/*
 * main -- for the stripchart display program.
 */
int 
main(int argc, char **argv)
{
  /* Initialize the Gtk stuff first, whether Gtk is used or not, since
     failure to do so will cause the color name to rgb translation
     done in read_param_defns to dump core. */

  /* Get program options and read in the parameter definition file. */
  display = gtk_graph;
  argp_program_version = prog_version;
  gnome_init(prog_name, &argp_opts, argc, argv, 0, NULL);

  params = read_param_defns(&chart_param);
  chart_glob.params = params;
  chart_glob.parray = chart_param;
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
      update_values(&slider_glob);
    }

  post_init = 1;
  if (display)
    (*display)();

  return EXIT_SUCCESS;
}
