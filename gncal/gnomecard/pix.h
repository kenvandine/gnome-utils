#ifndef __GNOMECARD_PIX
#define __GNOMECARD_PIX

#include <gnome.h>

typedef struct 
{
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	
        gint width;
        gint height;
} pix;

extern pix *null_pix, *crd_pix, *ident_pix, *geo_pix, *org_pix, *exp_pix, *sec_pix;
extern pix *phone_pix, *email_pix, *addr_pix, *expl_pix, *org_pix;

extern pix *pix_new(char **xpm);

#endif
