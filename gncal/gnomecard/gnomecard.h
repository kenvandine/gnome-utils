#include <gnome.h>
#include <glib.h>

#include "card.h"

typedef struct 
{
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	
        gint width;
        gint height;
} pix;

typedef struct
{
	/* Identity */
	GtkWidget *fn;
	GtkWidget *given, *add, *fam, *pre, *suf;
	GtkWidget *bday;
	
	/* Geographical */
	GtkWidget *tzh, *tzm;
	GtkWidget *gplon, *gplat;
	
	/* Organization */
	GtkWidget *title, *role;
	GtkWidget *orgn, *org1, *org2, *org3, *org4;

	/* Explanatory */
	GtkWidget *comment, *url;

	/* Security */
	GtkWidget *key, *keypgp;
	
	GList *l;
} GnomeCardEditor;

typedef struct
{
	GtkWidget *data, *type;
	
	GList *l;
} GnomeCardEMail;

typedef struct
{
	GtkWidget *type[13], *data;
	
	GList *l;
} GnomeCardPhone;

typedef struct
{
	GtkWidget *type[6], *data[7];
	
	GList *l;
} GnomeCardDelAddr;
