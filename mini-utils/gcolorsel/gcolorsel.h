#ifndef GCOLORSEL_H
#define GCOLORSEL_H

#include "gnome.h"

#include "mdi-color-generic.h"

extern GnomeMDI *mdi;

typedef GtkWidget *(*views_new) (MDIColorGeneric *mcg);

typedef struct views_t {
  char *name;
  int scroll_h_policy;
  int scroll_v_policy;
  views_new new;
  GtkType (*type) (void);
  char *description;
} views_t;

typedef struct docs_t {
  char *name;
  GtkType (*type) (void);
  gboolean can_create;
  gboolean connect;
  char *description;
} docs_t;

extern views_t views_tab[];
extern docs_t  docs_tab[];

views_t *get_views_from_type (GtkType type);

#endif
