#ifndef __GNOMECARD
#define __GNOMECARD

#include <gnome.h>

#define PHONE 1
#define EMAIL 2

#define TREE_SPACING 16

extern GtkCTree  *gnomecard_tree;

extern GList *gnomecard_crds;
extern GList *gnomecard_curr_crd;

extern char *gnomecard_fname;
extern char *gnomecard_find_str;
extern gboolean gnomecard_find_sens;
extern gboolean gnomecard_find_back;
extern gint gnomecard_def_data;

extern char *gnomecard_join_name (char *pre, char *given, char *add, 
				  char *fam, char *suf);
extern void gnomecard_set_add(gboolean state);
extern void gnomecard_set_changed(gboolean val);
extern void gnomecard_set_edit_del(gboolean state);
extern int  gnomecard_destroy_cards(void);

#endif
