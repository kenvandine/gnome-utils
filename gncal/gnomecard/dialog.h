#ifndef __GNOMECARD_DIALOG
#define __GNOMECARD_DIALOG

#include <gnome.h>


typedef struct
{
	/* Identity */
	GtkWidget *fn;
	GtkWidget *given, *add, *fam, *pre, *suf;
/* not used	GtkWidget *bday;  */

	/* Organization */
	GtkWidget *title;
	GtkWidget *orgn;
/* not used       GtkWidget *role; */
/* not used       GtkWidget *org1, *org2, *org3, *org4; */

        /* Internet Info */
        GtkWidget *email;
        GtkWidget *url;

        /* Address */
        GtkWidget *addrtype[6];
        GtkWidget *street1, *street2, *city, *state, *country, *zip;
        gint      curaddr;
        CardList  postal;

        /* Phone numbers */
        GtkWidget *phonetype[13];
        GtkWidget *phoneentry;
        gint      curphone;
        CardList  phone;
        
	/* Geographical */
	GtkWidget *tzh, *tzm;
	GtkWidget *gplon, *gplat;
	

	/* Explanatory */
	GtkWidget *comment;

	/* Security */
	GtkWidget *key, *keypgp;
	
	GList *l;
} GnomeCardEditor;


extern void gnomecard_add_email_call(GtkWidget *widget, gpointer data);
extern void gnomecard_add_phone_call(GtkWidget *widget, gpointer data);
extern void gnomecard_add_deladdr_call(GtkWidget *widget, gpointer data);
extern void gnomecard_add_dellabel_call(GtkWidget *widget, gpointer data);
extern void gnomecard_find_card_call(GtkWidget *widget, gpointer data);
extern void gnomecard_about(GtkWidget *widget, gpointer data);

extern void gnomecard_edit(GList *node);
extern void gnomecard_edit_card(GtkWidget *widget, gpointer data);
extern void gnomecard_append(GtkWidget *widget, gpointer data);
extern void gnomecard_open(GtkWidget *widget, gpointer data);
extern gboolean gnomecard_open_file(char *fname);
extern void gnomecard_save(void);
extern void gnomecard_save_as(GtkWidget *widget, gpointer data);
extern void gnomecard_setup(GtkWidget *widget, gpointer data);

#endif
