#include <config.h>
#include <fnmatch.h>
#include <gnome.h>

#include "card.h"
#include "canvas.h"
#include "dialog.h"
#include "gnomecard.h"
#include "my.h"
#include "sort.h"
#include "list.h"
#include "world.xpm"



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

	/* Geographical */
	GtkWidget *tzh, *tzm;
	GtkWidget *gplon, *gplat;
	

	/* Explanatory */
	GtkWidget *comment;

	/* Security */
	GtkWidget *key, *keypgp;
	
	GList *l;
} GnomeCardEditor;

typedef struct
{
	GtkWidget *entry, *sens, *back;
} GnomeCardFind;

typedef struct
{
	GtkWidget *def_phone, *def_email;
} GnomeCardSetup;

/* NOT USED
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

typedef struct
{
	GtkWidget *type[6], *data;
	
	GList *l;
} GnomeCardDelLabel;
*/

static void gnomecard_prop_close(GtkWidget *widget, gpointer node);
static void gnomecard_take_from_name(GtkWidget *widget, gpointer data);
static void gnomecard_cancel(GtkWidget *widget, gpointer data);
static void gnomecard_setup_apply(GtkWidget *widget, int page);
static void gnomecard_find_card(GtkWidget *w, gpointer data);
static void gnomecard_save_call(GtkWidget *widget, gpointer data);
static int gnomecard_match_pattern(char *pattern, char *str, int sens);

static void addrtypeclicked(GtkWidget *widget, gpointer data);
static void deleteAddrList(CardList src);
static void copyAddrList(CardList src, CardList *dest);
static void copyGUIToCurAddr(GnomeCardEditor *ce);
static void copyCurAddrToGUI(GnomeCardEditor *ce);
static void gnomecard_postaddr_entry_change(GtkWidget *w, gpointer data);
static void gnomecard_addrprop_used(GtkWidget *w, gpointer data);
static void postaddr_connect(gpointer widget, char *sig, gpointer box, 
		 CardProperty *prop, enum PropertyType type);




char *email_type_name[] = 
                 { N_("America On-Line"), N_("Apple Link"), N_("AT&T"),
		               N_("CIS"), N_("e-World"), N_("Internet"), N_("IBM"),
		               N_("MCI"), N_("Power Share"), N_("Prodigy"), N_("TLX"),
		               N_("X400"), NULL };

char *phone_type_name[] =
                 { N_("Preferred"), N_("Work"), N_("Home"), N_("Voice"),
			 N_("Fax"), N_("Message Recorder"), N_("Cellular"),
			 N_("Pager"), N_("Bulletin Board"), N_("Modem"),
			 N_("Car"), N_("ISDN"), N_("Video"), NULL };

char *addr_type_name[] =
                 { N_("Home"), N_("Work"), N_("Postal Box"), N_("Parcel"),
		   N_("Domestic"), N_("International"), NULL };

static ignore_postaddr_changes = FALSE;


/* avoid getting sending changes to property box when we are manually */
/* manipulating address entries                                       */
static void
gnomecard_postaddr_entry_change(GtkWidget *w, gpointer data) {
    
    if (!ignore_postaddr_changes)
	gnome_property_box_changed(data);
}

static void
gnomecard_addrprop_used(GtkWidget *w, gpointer data)
{
    CardProperty *prop;
    
    prop = (CardProperty *) data;
    prop->type = (int) gtk_object_get_user_data(GTK_OBJECT(w));
    prop->used = TRUE;
}

static void
postaddr_connect(gpointer widget, char *sig, gpointer box, 
		 CardProperty *prop, enum PropertyType type)
{
 	gtk_signal_connect_object(GTK_OBJECT(widget), sig,
				  GTK_SIGNAL_FUNC(gnomecard_postaddr_entry_change),
				  GTK_OBJECT(box));
	gtk_signal_connect(GTK_OBJECT(widget), sig,
			   GTK_SIGNAL_FUNC(gnomecard_addrprop_used),
			   prop);
	gtk_object_set_user_data(GTK_OBJECT(widget), (gpointer) type);
}

static void gnomecard_prop_apply(GtkWidget *widget, int page)
{
	GnomeCardEditor *ce;
	GList *node;
	Card *crd;
	struct tm *tm;
	time_t tt;

	if (page != -1)
	  return;             /* ignore partial applies */
	
	ce = (GnomeCardEditor *) gtk_object_get_user_data(GTK_OBJECT(widget));
	crd = (Card *) ce->l->data;
	
	MY_FREE(crd->fname.str);
	MY_FREE(crd->name.family);
	MY_FREE(crd->name.given);
	MY_FREE(crd->name.additional);
	MY_FREE(crd->name.prefix);
	MY_FREE(crd->name.suffix);
	
	crd->fname.str       = MY_STRDUP(gtk_entry_get_text(GTK_ENTRY(ce->fn)));
	crd->name.family     = MY_STRDUP(gtk_entry_get_text(GTK_ENTRY(ce->fam)));
	crd->name.given      = MY_STRDUP(gtk_entry_get_text(GTK_ENTRY(ce->given)));
	crd->name.additional = MY_STRDUP(gtk_entry_get_text(GTK_ENTRY(ce->add)));
	crd->name.prefix     = MY_STRDUP(gtk_entry_get_text(GTK_ENTRY(ce->pre)));
	crd->name.suffix     = MY_STRDUP(gtk_entry_get_text(GTK_ENTRY(ce->suf)));

/* NOT USED (yet)	
	tt = gnome_date_edit_get_date(GNOME_DATE_EDIT(ce->bday)); 
	tm = localtime(&tt);
	
	crd->bday.year       = tm->tm_year + 1900;
	crd->bday.month      = tm->tm_mon + 1;
	crd->bday.day        = tm->tm_mday;
*/
	crd->timezn.sign  = (ce->tzh < 0)? -1: 1;
	crd->timezn.hours = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ce->tzh));
	crd->timezn.mins  = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(ce->tzm));
	
	crd->geopos.lon = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(ce->gplon));
	crd->geopos.lat = gtk_spin_button_get_value_as_float(GTK_SPIN_BUTTON(ce->gplat));
	
	MY_FREE(crd->title.str);
	MY_FREE(crd->role.str);
	MY_FREE(crd->org.name);
	MY_FREE(crd->org.unit1);
	MY_FREE(crd->org.unit2);
	MY_FREE(crd->org.unit3);
	MY_FREE(crd->org.unit4);

	crd->title.str = MY_STRDUP(gtk_entry_get_text(GTK_ENTRY(ce->title)));
	crd->org.name  = MY_STRDUP(gtk_entry_get_text(GTK_ENTRY(ce->orgn)));
#if 0
	/* NOT USED */
	crd->role.str  = MY_STRDUP(gtk_entry_get_text(GTK_ENTRY(ce->role)));
	crd->org.unit1 = MY_STRDUP(gtk_entry_get_text(GTK_ENTRY(ce->org1)));
	crd->org.unit2 = MY_STRDUP(gtk_entry_get_text(GTK_ENTRY(ce->org2)));
	crd->org.unit3 = MY_STRDUP(gtk_entry_get_text(GTK_ENTRY(ce->org3)));
	crd->org.unit4 = MY_STRDUP(gtk_entry_get_text(GTK_ENTRY(ce->org4)));
#else
	crd->role.str  = MY_STRDUP("");
	crd->org.unit1 = MY_STRDUP("");
        crd->org.unit2 = MY_STRDUP("");
        crd->org.unit3 = MY_STRDUP("");
        crd->org.unit4 = MY_STRDUP("");
#endif	
	MY_FREE(crd->comment.str);
	MY_FREE(crd->url.str);
	MY_FREE(crd->email.address);
	
	crd->comment.str = gtk_editable_get_chars(GTK_EDITABLE(ce->comment), 
			 0, gtk_text_get_length(GTK_TEXT(ce->comment)));
	crd->url.str     = MY_STRDUP(gtk_entry_get_text(GTK_ENTRY(ce->url)));
	crd->email.address   = MY_STRDUP(gtk_entry_get_text(GTK_ENTRY(ce->email)));

	/* postal addresses */
	/* store results from current displayed address type, since */
	/* we may not have stored changes yet                       */
	copyGUIToCurAddr(ce);

	/* remove old address list */
	deleteAddrList(crd->postal);

	/* link to new address list */
	crd->postal = ce->postal;

        MY_FREE(crd->key.data);
	
	crd->key.data = gtk_editable_get_chars(GTK_EDITABLE(ce->key), 
						   0, gtk_text_get_length(GTK_TEXT(ce->key)));
	if (GTK_TOGGLE_BUTTON(ce->keypgp)->active)
	  crd->key.type = KEY_PGP;
	else
	  crd->key.type = KEY_X509;

	gnomecard_update_list(crd);
	gnomecard_update_canvas(crd);
	gnomecard_scroll_list(ce->l);
	gnomecard_set_changed(TRUE);
}

static void gnomecard_prop_close(GtkWidget *widget, gpointer node)
{
	((Card *) ((GList *) node)->data)->flag = FALSE;
	
	if ((GList *) node == gnomecard_curr_crd)
	  gnomecard_set_edit_del(TRUE);
}

static void gnomecard_take_from_name(GtkWidget *widget, gpointer data)
{
        GnomeCardEditor *ce;
	char *name;
	
	ce = (GnomeCardEditor *) data;
	
	name = gnomecard_join_name(gtk_entry_get_text(GTK_ENTRY(ce->pre)),
			 gtk_entry_get_text(GTK_ENTRY(ce->given)),
			 gtk_entry_get_text(GTK_ENTRY(ce->add)),
			 gtk_entry_get_text(GTK_ENTRY(ce->fam)),
			 gtk_entry_get_text(GTK_ENTRY(ce->suf)));
	
	gtk_entry_set_text(GTK_ENTRY(ce->fn), name);
	
	g_free(name);
}
	
extern void gnomecard_edit(GList *node)
{
	GnomePropertyBox *box;
	GnomeCardEditor *ce;
	GtkWidget *hbox, *hbox2, *vbox, *frame, *table;
	GtkWidget *label, *entry, *align, *align2, *pix;
	GtkWidget *addrhbox, *addrvbox, *addrtypebox;
	GtkWidget *addrtypeframe;
	GtkWidget *radio1, *radio2, *button;
	GtkObject *adj;
	
	GtkWidget *nametable;


	GSList    *addrtypegroup=NULL;
	Card *crd;
	time_t tmp_time;
	gint i;
	
	ce = g_new0(GnomeCardEditor, 1);
	ce->l = node;
	crd = (Card *) node->data;
	
	/* Set flag and disable Delete and Edit. */
	crd->flag = TRUE;
	gnomecard_set_edit_del(FALSE);
	/*gnomecard_set_add(TRUE);*/
	
	box = GNOME_PROPERTY_BOX(gnome_property_box_new());
	gtk_object_set_user_data(GTK_OBJECT(box), ce);
	gtk_window_set_wmclass(GTK_WINDOW(box), "GnomeCard",
			       "GnomeCard");
	gtk_signal_connect(GTK_OBJECT(box), "apply",
			   (GtkSignalFunc)gnomecard_prop_apply, NULL);
	gtk_signal_connect(GTK_OBJECT(box), "destroy",
			   (GtkSignalFunc)gnomecard_prop_close, node);
	
	/* Identity notebook page*/
	vbox = my_gtk_vbox_new();
	label = gtk_label_new(_("Identity"));
	gtk_notebook_append_page(GTK_NOTEBOOK(box->notebook), vbox, label);
	
	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	label = gtk_label_new(_("File As:"));
	ce->fn = entry = my_gtk_entry_new(0, crd->fname.str);
	my_connect(entry, "changed", box, &crd->fname.prop, PROP_FNAME);
	button = gtk_button_new_with_label(_("Take from Name"));
 	gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed),
				  GTK_OBJECT(box));
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(gnomecard_take_from_name),
			   ce);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/* create name frame */
	frame = gtk_frame_new(_("Name"));	
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, FALSE, 0);

	/* pack all name fields into a table */
	nametable = my_gtk_table_new(1, 4);
	gtk_container_add(GTK_CONTAINER(frame), nametable);

	/* first name */
	label = gtk_label_new(_("First:"));
	ce->given = entry = my_gtk_entry_new(0, crd->name.given);
	my_connect(entry, "changed", box, &crd->name.prop, PROP_NAME);
	align = gtk_alignment_new(0.0, 0.0, 0, 0);
        gtk_container_add (GTK_CONTAINER (align), entry);
	gtk_table_attach(GTK_TABLE(nametable), label, 0, 1, 0, 1,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 0, 0);
	gtk_table_attach(GTK_TABLE(nametable), align, 1, 2, 0, 1, 
			 GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_SHRINK, 
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);

	label = gtk_label_new(_("Middle:"));
	ce->add = entry = my_gtk_entry_new(0, crd->name.additional);
	my_connect(entry, "changed", box, &crd->name.prop, PROP_NAME);
	gtk_table_attach(GTK_TABLE(nametable), label, 2, 3, 0, 1,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 0, 0);
	gtk_table_attach(GTK_TABLE(nametable), entry, 3, 4, 0, 1, 
			 GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_SHRINK, 
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);

	label = gtk_label_new(_("Last:"));
	ce->fam = entry = my_gtk_entry_new(0, crd->name.family);
	align = gtk_alignment_new(0.0, 0.0, 0, 0);
        gtk_container_add (GTK_CONTAINER (align), entry);
	my_connect(entry, "changed", box, &crd->name.prop, PROP_NAME);
	gtk_table_attach(GTK_TABLE(nametable), label, /*4, 5, 0, 1, */
			                              0, 1, 1, 2, 
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 0, 0);
	gtk_table_attach(GTK_TABLE(nametable), align, /*5, 6, 0, 1,*/
			                              1, 4 , 1, 2,
			 GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);

	label = gtk_label_new(_("Prefix:"));
	ce->pre = entry = my_gtk_entry_new(5, crd->name.prefix);
	align = gtk_alignment_new(0.0, 0.0, 0, 0);
        gtk_container_add (GTK_CONTAINER (align), entry);
	my_connect(entry, "changed", box, &crd->name.prop, PROP_NAME);
	gtk_table_attach(GTK_TABLE(nametable), label, 0, 1, 2, 3,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	gtk_table_attach(GTK_TABLE(nametable), align, 1, 2, 2, 3, 
			 GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);

	label = gtk_label_new(_("Suffix:"));
	ce->suf = entry = my_gtk_entry_new(5, crd->name.suffix);
	align = gtk_alignment_new(0.0, 0.0, 0, 0); 
        gtk_container_add (GTK_CONTAINER (align), entry);
	my_connect(entry, "changed", box, &crd->name.prop, PROP_NAME);
        gtk_table_attach(GTK_TABLE(nametable), label, 2, 3, 2, 3,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	gtk_table_attach(GTK_TABLE(nametable), align, 3, 4, 2, 3, 
			 GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);

	/* organization and internet info share same line */
	/* add organization to identity notetab */
	hbox2 = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

	frame = gtk_frame_new(_("Organization"));
	gtk_box_pack_start(GTK_BOX(hbox2), frame, TRUE, TRUE, 0);

	table = my_gtk_table_new(2, 2);
	gtk_container_add(GTK_CONTAINER(frame), table);

	label = gtk_label_new(_("Name:"));
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	ce->orgn = entry = my_gtk_entry_new(0, crd->org.name);
	my_connect(entry, "changed", box, &crd->org.prop, PROP_ORG);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1,
			 GTK_FILL | GTK_SHRINK | GTK_EXPAND, GTK_FILL | GTK_SHRINK,
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);

	label = gtk_label_new(_("Title:"));
	ce->title = entry = my_gtk_entry_new(0, crd->title.str);
	my_connect(entry, "changed", box, &crd->title.prop, PROP_TITLE);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 1, 2,
			 GTK_FILL | GTK_SHRINK | GTK_EXPAND, GTK_FILL | GTK_SHRINK, 
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);


	/* internet related information */
	frame = gtk_frame_new(_("Internet Info"));
	gtk_box_pack_start(GTK_BOX(hbox2), frame, TRUE, TRUE, 0);

	table = my_gtk_table_new(2, 2);
	gtk_container_add(GTK_CONTAINER(frame), table);

	label = gtk_label_new(_("Email Address:"));
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	ce->email = entry = my_gtk_entry_new(0, crd->email.address);
	my_connect(entry, "changed", box, &crd->email.prop, PROP_ORG);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1,
			 GTK_FILL | GTK_SHRINK | GTK_EXPAND, GTK_FILL | GTK_SHRINK,
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);

	label = gtk_label_new(_("Homepage URL:"));
	ce->url = entry = my_gtk_entry_new(0, crd->url.str);
	my_connect(entry, "changed", box, &crd->url.prop, PROP_URL);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 1, 2,
			 GTK_FILL | GTK_SHRINK | GTK_EXPAND, GTK_FILL | GTK_SHRINK, 
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);

	/* add address notetab */
/*	align = gtk_alignment_new(0.0, 0.0, 0, 0); */
	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
/*        gtk_container_add (GTK_CONTAINER (align), hbox); */
	label = gtk_label_new(_("Addresses"));
/*	gtk_notebook_append_page(GTK_NOTEBOOK(box->notebook), align, label); */
	gtk_notebook_append_page(GTK_NOTEBOOK(box->notebook), hbox, label);

	/* make a frame for the address entry area */
	frame = gtk_frame_new(_("Address"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);

	/* make a hbox for entire address entry area */
	addrhbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_container_add(GTK_CONTAINER(frame), addrhbox);

	/* first have the address type entry area */
	addrtypebox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_end(GTK_BOX(addrhbox), addrtypebox, FALSE, FALSE, 
			   GNOME_PAD_SMALL);


	addrtypeframe = gtk_frame_new(_("Select address:"));
	gtk_frame_set_label_align(GTK_FRAME(addrtypeframe), 0.5, 0.5);
	gtk_box_pack_end(GTK_BOX(addrtypebox), addrtypeframe,FALSE,FALSE, 0);

	/* enter address types (based on Vcal standard I guess) */
	addrvbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_container_add(GTK_CONTAINER(addrtypeframe), addrvbox);

	/* default to the 1st addr type */
	ce->curaddr = 1;
	for (i = 0; i < 6; i++) {
		ce->addrtype[i]=gtk_radio_button_new_with_label(addrtypegroup,
							addr_type_name[i]);
		addrtypegroup = gtk_radio_button_group(GTK_RADIO_BUTTON(ce->addrtype[i]));
		gtk_object_set_user_data(GTK_OBJECT(ce->addrtype[i]),
					 (gpointer) (1 << i));
		gtk_box_pack_start(GTK_BOX(addrvbox), ce->addrtype[i],
				   FALSE, FALSE, 0);

		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(ce->addrtype[i]),
					    (i == 0) );

	}

	/* make the actual entry boxes for entering the address */
	table = my_gtk_table_new(6, 2);
	gtk_box_pack_end(GTK_BOX(addrhbox), table, TRUE, TRUE, 0);

	label = gtk_label_new(_("Street 1:"));
	align = gtk_alignment_new(1.0, 0.5, 0, 0);
        gtk_container_add (GTK_CONTAINER (align), label);
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	ce->street1 = entry = my_gtk_entry_new(0, "");
	gtk_table_attach(GTK_TABLE(table), align, 0, 1, 0, 1,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);

	label = gtk_label_new(_("Street 2:"));
	align = gtk_alignment_new(1.0, 0.5, 0, 0);
        gtk_container_add (GTK_CONTAINER (align), label);
	ce->street2 = entry = my_gtk_entry_new(0, "");
/*	my_connect(entry, "changed", box, &crd->title.prop, PROP_TITLE); */
	gtk_table_attach(GTK_TABLE(table), align, 0, 1, 1, 2,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 1, 2,
			 GTK_FILL | GTK_SHRINK | GTK_EXPAND, GTK_FILL | GTK_SHRINK, 
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);

	label = gtk_label_new(_("City:"));
	align = gtk_alignment_new(1.0, 0.5, 0, 0);
        gtk_container_add (GTK_CONTAINER (align), label);
	ce->city = entry = my_gtk_entry_new(0, "");
/*	my_connect(entry, "changed", box, &crd->title.prop, PROP_TITLE); */
	gtk_table_attach(GTK_TABLE(table), align, 0, 1, 2, 3,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 2, 3,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);

	label = gtk_label_new(_("State:"));
	align = gtk_alignment_new(1.0, 0.5, 0, 0);
        gtk_container_add (GTK_CONTAINER (align), label);
	ce->state = entry = my_gtk_entry_new(0, "");
/*	my_connect(entry, "changed", box, &crd->title.prop, PROP_TITLE); */
	gtk_table_attach(GTK_TABLE(table), align, 0, 1, 3, 4,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 3, 4,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);

	label = gtk_label_new(_("Zip Code:"));
	align = gtk_alignment_new(1.0, 0.5, 0, 0);
        gtk_container_add (GTK_CONTAINER (align), label);
	ce->zip = entry = my_gtk_entry_new(0, "");
/*	my_connect(entry, "changed", box, &crd->title.prop, PROP_TITLE); */
	gtk_table_attach(GTK_TABLE(table), align, 0, 1, 4, 5,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 4, 5,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);

	label = gtk_label_new(_("Country:"));
	align = gtk_alignment_new(1.0, 0.5, 0, 0);
        gtk_container_add (GTK_CONTAINER (align), label);
	ce->country = entry = my_gtk_entry_new(0, "");
/*	my_connect(entry, "changed", box, &crd->title.prop, PROP_TITLE); */
	gtk_table_attach(GTK_TABLE(table), align, 0, 1, 5, 6,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 5, 6,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
			 GNOME_PAD_SMALL, GNOME_PAD_SMALL);


	/* fill in data structures for the addresses */
	copyAddrList(crd->postal, &ce->postal);

	/* prime the GUI */
	copyCurAddrToGUI(ce);

	/* attach signals now we're finished */
	for (i=0; i < 6; i++)
	    gtk_signal_connect(GTK_OBJECT(ce->addrtype[i]), "clicked",
			       GTK_SIGNAL_FUNC(addrtypeclicked),
			       ce);

	postaddr_connect(ce->street1, "changed", box, &crd->postal.prop, PROP_POSTADDR); 
	postaddr_connect(ce->street2, "changed", box, &crd->postal.prop, PROP_POSTADDR); 
	postaddr_connect(ce->city, "changed", box, &crd->postal.prop, PROP_POSTADDR); 
	postaddr_connect(ce->state, "changed", box, &crd->postal.prop, PROP_POSTADDR); 
	postaddr_connect(ce->zip, "changed", box, &crd->postal.prop, PROP_POSTADDR); 
	postaddr_connect(ce->country, "changed", box, &crd->postal.prop, PROP_POSTADDR); 
	

/* LOSE BIRTHDAY FOR NOW	
	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	label = gtk_label_new(_("Birthdate:"));
	
	if (crd->bday.prop.used) {
		struct tm tm = {0, 0, 0, 0, 0, 0, 0, 0, 0};
		
		tm.tm_year = crd->bday.year - 1900;
		tm.tm_mon  = crd->bday.month - 1;
		tm.tm_mday = crd->bday.day;
		
		tmp_time = mktime(&tm);
	} else
	  tmp_time = time(NULL);
	
	ce->bday = entry = gnome_date_edit_new(tmp_time, FALSE, FALSE);
	my_connect(GNOME_DATE_EDIT(entry)->calendar, "day_selected",
		   box, &crd->bday.prop, PROP_BDAY);
	my_connect(GNOME_DATE_EDIT(entry)->date_entry, "changed",
		   box, &crd->bday.prop, PROP_BDAY);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
*/

	/* Geographical */
	align2 = gtk_alignment_new(0.5, 0.5, 0, 0);
	hbox2 = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
        gtk_container_add (GTK_CONTAINER (align2), hbox2);
	label = gtk_label_new(_("Geographical"));
	gtk_notebook_append_page(GTK_NOTEBOOK(box->notebook), align2, label);
	
	pix = gnome_pixmap_new_from_xpm_d (world_xpm);
	gtk_box_pack_start(GTK_BOX(hbox2), pix, FALSE, FALSE, 0);
	
	vbox = my_gtk_vbox_new();
	gtk_box_pack_start(GTK_BOX(hbox2), vbox, FALSE, FALSE, 0);
	frame = gtk_frame_new(_("Time Zone"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_container_border_width(GTK_CONTAINER(hbox), GNOME_PAD_SMALL);
	align = gtk_alignment_new(0.5, 0.5, 0, 0);
        gtk_container_add (GTK_CONTAINER (align), hbox);
	gtk_container_add(GTK_CONTAINER(frame), align);
	label = gtk_label_new(_("hrs."));
	adj = gtk_adjustment_new(crd->timezn.prop.used? 
				 crd->timezn.sign * crd->timezn.hours : 0,
				 -12, 12, 1, 1, 3);
	ce->tzh = entry = my_gtk_spin_button_new(GTK_ADJUSTMENT(adj), 3);
	my_connect(adj, "value_changed", box, &crd->timezn.prop, PROP_TIMEZN);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	label = gtk_label_new(_("mins."));
	adj = gtk_adjustment_new(crd->timezn.prop.used? crd->timezn.mins : 0,
				 0, 59, 1, 1, 10);
	ce->tzm = entry = my_gtk_spin_button_new(GTK_ADJUSTMENT(adj), 2);
	my_connect(adj, "value_changed", box, &crd->timezn.prop, PROP_TIMEZN);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
		
	frame = gtk_frame_new(_("Geographic Position"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_container_border_width(GTK_CONTAINER(hbox), GNOME_PAD_SMALL);
	align = gtk_alignment_new(0.5, 0.5, 0, 0);
        gtk_container_add (GTK_CONTAINER (align), hbox);
	gtk_container_add(GTK_CONTAINER(frame), align);
	label = gtk_label_new(_("lat, "));
	adj = gtk_adjustment_new(crd->geopos.prop.used? crd->geopos.lat : 0,
				 -90, 90, .01, 1, 1);
	ce->gplat = entry = my_gtk_spin_button_new(GTK_ADJUSTMENT(adj), 5);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(entry), 2);
	my_connect(adj, "value_changed", box, &crd->geopos.prop, PROP_GEOPOS);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	label = gtk_label_new(_("lon."));
	adj = gtk_adjustment_new(crd->geopos.prop.used? crd->geopos.lon : 0,
				 -180, 180, .01, 1, 1);
	ce->gplon = entry = my_gtk_spin_button_new(GTK_ADJUSTMENT(adj), 6);
	gtk_spin_button_set_digits(GTK_SPIN_BUTTON(entry), 2);
	my_connect(adj, "value_changed", box, &crd->geopos.prop, PROP_GEOPOS);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

	/* Explanatory */
	
	vbox = my_gtk_vbox_new();
	label = gtk_label_new(_("Explanatory"));
	gtk_notebook_append_page(GTK_NOTEBOOK(box->notebook), vbox, label);
	
	label = gtk_label_new(_("Comment:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	ce->comment = entry = gtk_text_new(NULL, NULL);
	gtk_text_set_editable(GTK_TEXT(entry), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
	gtk_widget_realize(entry);
	if (crd->comment.prop.used) {
		int pos = 0;
		gtk_editable_insert_text(GTK_EDITABLE(entry), crd->comment.str,
					 strlen(crd->comment.str), &pos);
	}
	gtk_widget_set_usize (entry, 0, (entry->style->font->ascent +
					 entry->style->font->descent) * 4);
	my_connect(entry, "changed", box, &crd->comment.prop, PROP_COMMENT);
	
/* URL is above now 
	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	label = gtk_label_new(_("URL:"));
	ce->url = entry = my_gtk_entry_new(0, crd->url.str);
	my_connect(entry, "changed", box, &crd->url.prop, PROP_URL);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
*/

	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, GNOME_PAD);
	label = gtk_label_new(_("Last Revision:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	label = gtk_label_new(_("The last revision goes here."));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	
	/* Security */
	
	vbox = my_gtk_vbox_new();
	label = gtk_label_new(_("Security"));
	gtk_notebook_append_page(GTK_NOTEBOOK(box->notebook), vbox, label);

	label = gtk_label_new(_("Public Key:"));
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	ce->key = entry = gtk_text_new(NULL, NULL);
	gtk_text_set_editable(GTK_TEXT(entry), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);
	gtk_widget_realize(entry);
	if (crd->key.prop.used) {
		int pos = 0;
		gtk_editable_insert_text(GTK_EDITABLE(entry), crd->key.data,
					 strlen(crd->key.data), &pos);
	}
	gtk_widget_set_usize (entry, 0, (entry->style->font->ascent +
					 entry->style->font->descent) * 6);
	my_connect(entry, "changed", box, &crd->key.prop, PROP_KEY);

	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	ce->keypgp = radio1 = gtk_radio_button_new_with_label(NULL, _("PGP"));
	if (crd->key.prop.used && crd->key.type != KEY_PGP)
	  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(radio1), 0);
	my_connect(radio1, "toggled", box, &crd->key.prop, PROP_KEY);
	gtk_box_pack_start(GTK_BOX(hbox), radio1, FALSE, FALSE, 0);
	radio2 = gtk_radio_button_new_with_label(
		gtk_radio_button_group(GTK_RADIO_BUTTON(radio1)),
		_("X509"));
	if (crd->key.prop.used && crd->key.type == KEY_X509)
	  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(radio2), 1);
	my_connect(radio2, "toggled", box, &crd->key.prop, PROP_KEY);
	gtk_box_pack_start(GTK_BOX(hbox), radio2, FALSE, FALSE, 0);

	gtk_widget_show_all(GTK_WIDGET(box));
}

static void
gnomecard_cancel(GtkWidget *widget, gpointer data)
{
	void *p;

	if ((p = gtk_object_get_user_data(GTK_OBJECT(widget))) != NULL)
		g_free(p);
	
	gtk_widget_destroy(widget);
}

static void
gnomecard_setup_apply(GtkWidget *widget, int page)
{
	GnomeCardSetup *setup;
	int old_def_data;

	if (page != -1)
	  return;             /* ignore partial applies */
	
	setup = (GnomeCardSetup *) gtk_object_get_user_data(GTK_OBJECT(widget));
	
	old_def_data = gnomecard_def_data;
	gnomecard_def_data = 0;
	
	if (GTK_TOGGLE_BUTTON(setup->def_phone)->active)
		gnomecard_def_data |= PHONE;
	
	if (GTK_TOGGLE_BUTTON(setup->def_email)->active)
		gnomecard_def_data |= EMAIL;

	gnome_config_set_int("/GnomeCard/layout/def_data",  gnomecard_def_data);
}
			
void
gnomecard_setup(GtkWidget *widget, gpointer data)
{
	GnomePropertyBox *box;
	GnomeCardSetup *setup;
	GtkWidget *vbox, *vbox2, *frame;
	GtkWidget *label, *check;
	
	setup = g_malloc(sizeof(GnomeCardSetup));
	box = GNOME_PROPERTY_BOX(gnome_property_box_new());
	gtk_object_set_user_data(GTK_OBJECT(box), setup);
	gtk_window_set_wmclass(GTK_WINDOW(box), "GnomeCard",
			       "GnomeCard");
	gtk_signal_connect(GTK_OBJECT(box), "apply",
			   (GtkSignalFunc)gnomecard_setup_apply, NULL);

	vbox = my_gtk_vbox_new();
	label = gtk_label_new(_("Layout"));
	gtk_notebook_append_page(GTK_NOTEBOOK(box->notebook), vbox, label);
	
	frame = gtk_frame_new(_("Default data"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 0);
	
	vbox2 = my_gtk_vbox_new();
	gtk_container_add(GTK_CONTAINER(frame), vbox2);
	/* falta conectar con el apply... checa my_connect. */
	check = setup->def_phone = gtk_check_button_new_with_label("Phone");
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(check), gnomecard_def_data & PHONE);
 	gtk_signal_connect_object(GTK_OBJECT(check), "clicked",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed),
				  GTK_OBJECT(box));
	gtk_box_pack_start(GTK_BOX(vbox2), check, FALSE, FALSE, 0);
	check = setup->def_email = gtk_check_button_new_with_label("E-mail");
 	gtk_signal_connect_object(GTK_OBJECT(check), "clicked",
				  GTK_SIGNAL_FUNC(gnome_property_box_changed),
				  GTK_OBJECT(box));
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(check), gnomecard_def_data & EMAIL);
	gtk_box_pack_start(GTK_BOX(vbox2), check, FALSE, FALSE, 0);
	
	gtk_widget_show_all(GTK_WIDGET(box));
}

#if 0
/* NOT USED */
void gnomecard_add_email(GtkWidget *widget, gpointer data)
{
	GnomeCardEMail *e;
	CardEMail *email;
	char *text;

	g_message("gnomecard_add_email not used/implemented");
#if 0	
	e = (GnomeCardEMail *) data;
	
	text = gtk_entry_get_text(GTK_ENTRY(e->data));
	if (*text == 0)
		return;
	
	email = g_malloc(sizeof(CardEMail));
	email->data = g_strdup(text);
	email->prop = empty_CardProperty();
	email->prop.used = TRUE;
	email->prop.type = PROP_EMAIL;
	email->type = (int) gtk_object_get_user_data(GTK_OBJECT(gtk_menu_get_active(GTK_MENU(e->type))));
	
	((Card *) e->l->data)->email.l = g_list_append(((Card *) e->l->data)->email.l, email);

	gnomecard_update_list(e->l->data);
	
	gtk_editable_delete_text(GTK_EDITABLE(e->data), 0, strlen(text));
	gnomecard_set_changed(TRUE);
#endif
}

extern void gnomecard_add_email_call(GtkWidget *widget, gpointer data)
{
	GtkWidget *w, *hbox, *label, *omenu, *item;
	GnomeCardEMail *e;
	char *title;
	Card *crd;
	int i;

	g_message("gnomecard_add_email_call not used/implemented");
#if 0	
	crd = gnomecard_curr_crd->data;

	/* + 7 for case crd->fname == 0, which sprintf's to "(null)" */
	title = g_malloc(strlen(_("Add E-mail for ")) + MY_STRLEN(crd->fname.str) + 7);
	
	sprintf(title, _("Add E-mail for %s"), crd->fname.str);
	
	e = g_malloc(sizeof(GnomeCardEMail));
	e->l = gnomecard_curr_crd;
	w = gnome_dialog_new(title, "Add", GNOME_STOCK_BUTTON_CLOSE, NULL);
	gtk_object_set_user_data(GTK_OBJECT(w), e);
	gnome_dialog_button_connect(GNOME_DIALOG(w), 0,
															GTK_SIGNAL_FUNC(gnomecard_add_email),	e);
	gnome_dialog_button_connect_object(GNOME_DIALOG(w),	1,
																		 GTK_SIGNAL_FUNC(gnomecard_cancel),
																		 GTK_OBJECT(w));

	e->data = my_hbox_entry(GNOME_DIALOG(w)->vbox, "E-mail address:", NULL);

	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(w)->vbox), hbox, FALSE, FALSE, 0);
	label = gtk_label_new(_("E-mail type:"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	omenu = gtk_option_menu_new();
	gtk_box_pack_start(GTK_BOX(hbox), omenu, FALSE, FALSE, 0);
	
	e->type = gtk_menu_new();
	
	for (i = 0; email_type_name[i]; i++) {
		item = gtk_menu_item_new_with_label(email_type_name[i]);
		gtk_object_set_user_data(GTK_OBJECT(item), (gpointer) i);
		gtk_menu_append(GTK_MENU(e->type), item);
		gtk_widget_show(item);
	}
	
	gtk_menu_set_active(GTK_MENU(e->type), EMAIL_INET);
	gtk_option_menu_set_menu(GTK_OPTION_MENU(omenu), e->type);
	
	gtk_widget_show_all(GNOME_DIALOG(w)->vbox);
	gtk_widget_show(w);
#endif
}


void gnomecard_add_phone(GtkWidget *widget, gpointer data)
{
	GnomeCardPhone *p;
	CardPhone *phone;
	char *text;
	int i;
	
	p = (GnomeCardPhone *) data;
	
	text = gtk_entry_get_text(GTK_ENTRY(p->data));
	if (*text == 0)
		return;
	
	phone = g_malloc(sizeof(CardPhone));
	phone->data = g_strdup(text);
	phone->prop = empty_CardProperty();
	phone->prop.used = TRUE;
	phone->type = 0;
	for (i = 0; i < 13; i++)
		if (GTK_TOGGLE_BUTTON(p->type[i])->active)
			phone->type |= (int) gtk_object_get_user_data(GTK_OBJECT(p->type[i]));
	
	((Card *) p->l->data)->phone.l = g_list_append(((Card *) p->l->data)->phone.l, phone);

	gnomecard_update_list(p->l->data);
	
	gtk_editable_delete_text(GTK_EDITABLE(p->data), 0, strlen(text));
	gnomecard_set_changed(TRUE);
}

extern void gnomecard_add_phone_call(GtkWidget *widget, gpointer data)
{
	GtkWidget *w, *hbox, *frame, *vbox;
	GnomeCardPhone *p;
	char *title;
	Card *crd;
	int i;
	
	crd = gnomecard_curr_crd->data;
	
	/* + 7 for case crd->fname == 0, which sprintf's to "(null)" */
	title = g_malloc(strlen(_("Add Telephone Number for ")) + MY_STRLEN(crd->fname.str) + 7);
	
	sprintf(title, _("Add Telephone Number for %s"), crd->fname.str);
	
	p = g_malloc(sizeof(GnomeCardPhone));
	p->l = gnomecard_curr_crd;
	w = gnome_dialog_new(title, "Add", GNOME_STOCK_BUTTON_CLOSE, NULL);
	gtk_object_set_user_data(GTK_OBJECT(w), p);
	gnome_dialog_button_connect(GNOME_DIALOG(w), 0,
															GTK_SIGNAL_FUNC(gnomecard_add_phone),	p);
	gnome_dialog_button_connect_object(GNOME_DIALOG(w),	1,
																		 GTK_SIGNAL_FUNC(gnomecard_cancel),
																		 GTK_OBJECT(w));

	p->data = my_hbox_entry(GNOME_DIALOG(w)->vbox, "Telephone Number:", NULL);
	
	frame = gtk_frame_new(_("Phone type:"));
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(w)->vbox), frame, FALSE, FALSE, 0);
	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_container_add(GTK_CONTAINER(frame), hbox);
	
	vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	for (i = 0; i < 7; i++) {
		p->type[i] = gtk_check_button_new_with_label(phone_type_name[i]);
		gtk_object_set_user_data(GTK_OBJECT(p->type[i]), (gpointer) (1 << i));
		gtk_box_pack_start(GTK_BOX(vbox), p->type[i], FALSE, FALSE, 0);
	}
		
	vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 0);

	for (i = 7; i < 13; i++) {
		p->type[i] = gtk_check_button_new_with_label(phone_type_name[i]);
		gtk_object_set_user_data(GTK_OBJECT(p->type[i]), (gpointer) (1 << i));
		gtk_box_pack_start(GTK_BOX(vbox), p->type[i], FALSE, FALSE, 0);
	}
	
	gtk_widget_show_all(GNOME_DIALOG(w)->vbox);
	gtk_widget_show(w);
}
/* END OF NOT USED */
#endif

#if 0
/* NO LONGER IN USE */
void gnomecard_add_deladdr(GtkWidget *widget, gpointer data)
{
	GnomeCardDelAddr *p;
	CardDelAddr *addr;
	char *text[7];
	int i, flag;
	
	p = (GnomeCardDelAddr *) data;
	
	flag = 0;
	for (i = 0; i < 7; i++) {
		text[i] = gtk_entry_get_text(GTK_ENTRY(p->data[i]));
		if (*text[i])
			flag = 1;
	}
	
	if (!flag)
		return;
	
	addr = g_malloc(sizeof(CardDelAddr));
	
	for (i = 0; i < DELADDR_MAX; i++)
		addr->data[i] = MY_STRDUP(text[i]);
	
	addr->prop = empty_CardProperty();
	addr->prop.used = TRUE;

	addr->type = 0;
	for (i = 0; i < 6; i++)
		if (GTK_TOGGLE_BUTTON(p->type[i])->active)
			addr->type |= (int) gtk_object_get_user_data(GTK_OBJECT(p->type[i]));
	
	((Card *) p->l->data)->deladdr.l = g_list_append(((Card *) p->l->data)->deladdr.l, addr);

	gnomecard_update_list(p->l->data);

	for (i = 0; i < DELADDR_MAX; i++)
		gtk_editable_delete_text(GTK_EDITABLE(p->data[i]), 0, strlen(text[i]));
	gnomecard_set_changed(TRUE);
}

extern void gnomecard_add_deladdr_call(GtkWidget *widget, gpointer data)
{
	GtkWidget *w, *hbox, *frame, *vbox, *table, *label;
	GnomeCardDelAddr *p;
	char *title;
	Card *crd;
	int i;
	
	crd = gnomecard_curr_crd->data;
	
	/* + 7 for case crd->fname == 0, which sprintf's to "(null)" */
	title = g_malloc(strlen(_("Add Delivery Address for ")) + MY_STRLEN(crd->fname.str) + 7);
	
	sprintf(title, _("Add Delivery Address for %s"), crd->fname.str);
	
	p = g_malloc(sizeof(GnomeCardDelAddr));
	p->l = gnomecard_curr_crd;
	w = gnome_dialog_new(title, "Add", GNOME_STOCK_BUTTON_CLOSE, NULL);
	gtk_object_set_user_data(GTK_OBJECT(w), p);
	gnome_dialog_button_connect(GNOME_DIALOG(w), 0,
															GTK_SIGNAL_FUNC(gnomecard_add_deladdr),	p);
	gnome_dialog_button_connect_object(GNOME_DIALOG(w),	1,
																		 GTK_SIGNAL_FUNC(gnomecard_cancel),
																		 GTK_OBJECT(w));

	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(w)->vbox), hbox, FALSE, FALSE, 0);
	
	frame = gtk_frame_new(_("Address type:"));
	gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, FALSE, 0);
	vbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	for (i = 0; i < 6; i++) {
		p->type[i] = gtk_check_button_new_with_label(addr_type_name[i]);
		gtk_object_set_user_data(GTK_OBJECT(p->type[i]), (gpointer) (1 << i));
		gtk_box_pack_start(GTK_BOX(vbox), p->type[i], FALSE, FALSE, 0);
	}
	
	table = my_gtk_table_new(2, DELADDR_MAX);
	gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 0);
	
	label = gtk_label_new("Post Office:");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	p->data[0] = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), p->data[0], 1, 2, 0, 1,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	label = gtk_label_new("Extended:");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	p->data[1] = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), p->data[1], 1, 2, 1, 2,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	label = gtk_label_new("Street:");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	p->data[2] = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), p->data[2], 1, 2, 2, 3,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	label = gtk_label_new("City:");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 3, 4,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	p->data[3] = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), p->data[3], 1, 2, 3, 4,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	label = gtk_label_new("Region:");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 4, 5,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	p->data[4] = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), p->data[4], 1, 2, 4, 5,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	label = gtk_label_new("Code:");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 5, 6,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	p->data[5] = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), p->data[5], 1, 2, 5, 6,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	label = gtk_label_new("Country:");
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 6, 7,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	p->data[6] = gtk_entry_new();
	gtk_table_attach(GTK_TABLE(table), p->data[6], 1, 2, 6, 7,
			 GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 0, 0);
	
	gtk_widget_show_all(GNOME_DIALOG(w)->vbox);
	gtk_widget_show(w);
}

void gnomecard_add_dellabel(GtkWidget *widget, gpointer data)
{
	GnomeCardDelLabel *p;
	CardDelLabel *addr;
	char *text;
	int i;
	
	p = (GnomeCardDelLabel *) data;
	
	text = gtk_editable_get_chars(GTK_EDITABLE(p->data), 0,
				      gtk_text_get_length(GTK_TEXT(p->data)));
	if (*text == 0)
	  return;
	
	addr = g_malloc(sizeof(CardDelLabel));
	addr->data = MY_STRDUP(text);
	addr->prop = empty_CardProperty();
	addr->prop.encod = ENC_QUOTED_PRINTABLE;
	addr->prop.used = TRUE;

	addr->type = 0;
	for (i = 0; i < 6; i++)
	  if (GTK_TOGGLE_BUTTON(p->type[i])->active)
	    addr->type |= (int) gtk_object_get_user_data(GTK_OBJECT(p->type[i]));
	
	((Card *) p->l->data)->dellabel.l = g_list_append(((Card *) p->l->data)->dellabel.l, addr);

	gnomecard_update_list(p->l->data);

	gtk_editable_delete_text(GTK_EDITABLE(p->data), 0, strlen(text));
	gnomecard_set_changed(TRUE);
}

extern void gnomecard_add_dellabel_call(GtkWidget *widget, gpointer data)
{
}
#endif


/* delete card list of addresses from src, freeing as we go */
static void
deleteAddrList(CardList src)
{
    GList *l;
    CardPostAddr *p;

    for (l=src.l; l; l=l->next) {
	p = (CardPostAddr *)l->data;
	MY_FREE(p->street1);
	MY_FREE(p->street2);
	MY_FREE(p->city);
	MY_FREE(p->state);
 	MY_FREE(p->zip);
	MY_FREE(p->country);
	MY_FREE(p);
    }
    
    g_list_free(src.l);
}

/* copy card list of addresses from src into dest, allocating as we go */
static void
copyAddrList(CardList src, CardList *dest)
{
    GList *l;

    for (l=src.l; l; l=l->next) {
	CardPostAddr *p, *srcp;

	srcp = (CardPostAddr *)l->data;
	p = g_new0(CardPostAddr, 1);
	p->prop = empty_CardProperty();
	p->prop = srcp->prop;

	p->type    = srcp->type;
	p->street1 = g_strdup(srcp->street1);
	p->street2 = g_strdup(srcp->street2);
	p->city    = g_strdup(srcp->city);
	p->state   = g_strdup(srcp->state);
 	p->zip     = g_strdup(srcp->zip);
	p->country = g_strdup(srcp->country);
	
	dest->l = g_list_append(dest->l, p);
   }
}

/* try to find specific address type in a list of addresses */
/* returns ptr to list item if found, otherwise return NULL */
static GList *
findmatchAddrType(GList *l, gint type)
{
    GList *k;

    for (k = l; k; k = k->next) {
	CardPostAddr *p = ((CardPostAddr *)k->data);
	
	if ( p->type == type )
	    break;
    }

    return k;
}


/* set address edit fields according to the current address type */
/* assumes ce is a valid structure from call to gnomecard_edit() */
static void 
copyCurAddrToGUI(GnomeCardEditor *ce) {
    CardPostAddr *p;
    GList *l;

    l = findmatchAddrType(ce->postal.l, ce->curaddr);
    if (!l) {
	CardPostAddr *addr;

	/* create new address type */
	g_message("Creating addrtype %d in copyCurAddrToGUI()",ce->curaddr);
	addr = g_new0(CardPostAddr, 1);
	addr->street1 = g_strdup("");
	addr->street2 = g_strdup("");
	addr->city    = g_strdup("");
	addr->state   = g_strdup("");
	addr->country = g_strdup("");
	addr->zip     = g_strdup("");
	addr->type    = ce->curaddr;
	addr->prop    = empty_CardProperty();
	addr->prop.used = TRUE;

	ce->postal.l = g_list_append(ce->postal.l, addr);
	l = findmatchAddrType(ce->postal.l, ce->curaddr);
    }

    /* copy address into GUI */
    ignore_postaddr_changes = TRUE;
    p = ((CardPostAddr *)l->data);
    gtk_entry_set_text(GTK_ENTRY(ce->street1), (p->street1) ? p->street1 : "");
    gtk_entry_set_text(GTK_ENTRY(ce->street2), (p->street2) ? p->street2 : "");
    gtk_entry_set_text(GTK_ENTRY(ce->city), (p->city) ? p->city : "");
    gtk_entry_set_text(GTK_ENTRY(ce->state), (p->state) ? p->state : "");
    gtk_entry_set_text(GTK_ENTRY(ce->zip), (p->zip) ? p->zip : "");
    gtk_entry_set_text(GTK_ENTRY(ce->country), (p->country) ? p->country : "");
    ignore_postaddr_changes = FALSE;
	
}

/* copies GUI of address entry into current data for address type */
static void 
copyGUIToCurAddr(GnomeCardEditor *ce) {
    CardPostAddr *p;
    GList *l;

    l = findmatchAddrType(ce->postal.l, ce->curaddr);
    if (!l) {
	CardPostAddr *addr;

	/* create new address type */
	g_message("Creating addrtype %d in copyGUIToCurAddr()",ce->curaddr);
	addr = g_new0(CardPostAddr, 1);
	addr->street1 = addr->street2 = addr->city = addr->state = NULL;
	addr->country = addr->zip = NULL;
	addr->type    = ce->curaddr;
	addr->prop    = empty_CardProperty();
	addr->prop.used = TRUE;

	ce->postal.l = g_list_append(ce->postal.l, addr);
	l = findmatchAddrType(ce->postal.l, ce->curaddr);
    }

    /* copy GUI into GUI */
    p = ((CardPostAddr *)l->data);
    MY_FREE(p->street1);
    p->street1 = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->street1)));
    MY_FREE(p->street2);
    p->street2 = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->street2)));
    MY_FREE(p->city);
    p->city = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->city)));
    MY_FREE(p->state);
    p->state = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->state)));
    MY_FREE(p->zip);
    p->zip = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->zip)));
    MY_FREE(p->country);
    p->country = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->country)));
}

static void
addrtypeclicked(GtkWidget *widget, gpointer data)
{
    gint num;
    GnomeCardEditor *ce;

    /* what is the address type of button causing this event */
    num = GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(widget)));
    ce  = (GnomeCardEditor *)data;

    /* if we are turning off a button, copy entry data into state vars */
    if (!GTK_TOGGLE_BUTTON(widget)->active) {
	copyGUIToCurAddr(ce);
	return;
    } else {
	ce->curaddr = num;
	copyCurAddrToGUI(ce);
    }
}


extern void gnomecard_edit_card(GtkWidget *widget, gpointer data)
{
	if (gnomecard_curr_crd)
	  gnomecard_edit(gnomecard_curr_crd);
}

static gboolean gnomecard_append_file(char *fname)
{
	GtkWidget *w;
	GList *c, *crds;

	if (!(crds = card_load(gnomecard_crds, fname))) {
		char *tmp;
		
		tmp = g_malloc(strlen(_("Wrong file format.")) + strlen(fname) + 3);
		sprintf(tmp, "%s: %s", fname, _("Wrong file format."));
		w = gnome_message_box_new(tmp,
					  GNOME_MESSAGE_BOX_ERROR,
					  GNOME_STOCK_BUTTON_OK, NULL);
		GTK_WINDOW(w)->position = GTK_WIN_POS_MOUSE;
		gtk_widget_show(w);
		g_free(tmp);
		
		return FALSE;
	}
	
	gnomecard_crds = crds;
	gnomecard_sort_card_list(gnomecard_sort_criteria);

	gtk_clist_freeze(gnomecard_list);
	gtk_clist_clear(gnomecard_list);
	for (c = gnomecard_crds; c; c = c->next)
	    gnomecard_add_card_to_list((Card *) c->data);
	gtk_clist_thaw(gnomecard_list);
	gnomecard_scroll_list(gnomecard_crds);
	gnomecard_set_curr(gnomecard_crds);
	return TRUE;
}

static void gnomecard_append_call(GtkWidget *widget, gpointer data)
{
	char *fname;
	
	fname = gtk_file_selection_get_filename(GTK_FILE_SELECTION(widget));
	
	if (gnomecard_append_file(fname))
		gtk_widget_destroy(widget);
}

extern void gnomecard_append(GtkWidget *widget, gpointer data)
{
	GtkWidget *fsel;
	
	fsel = gtk_file_selection_new(_("Append GnomeCard File"));
	gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(fsel));
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fsel)->ok_button),
			   "clicked", GTK_SIGNAL_FUNC(gnomecard_append_call),
			   GTK_OBJECT(fsel));
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fsel)->cancel_button),
			   "clicked", GTK_SIGNAL_FUNC(gnomecard_cancel),
			   GTK_OBJECT(fsel));
	gtk_widget_show(fsel);
}

extern gboolean gnomecard_open_file(char *fname)
{
	if (! gnomecard_destroy_cards())
	  return FALSE;
	
	return gnomecard_append_file(fname);
}

static void gnomecard_open_call(GtkWidget *widget, gpointer data)
{
	char *fname;
	
	fname = gtk_file_selection_get_filename(GTK_FILE_SELECTION(widget));
	
	if (gnomecard_open_file(fname)) {
		g_free(gnomecard_fname);
		gnomecard_fname = g_strdup(fname);
		gtk_widget_destroy(widget);
		gnome_config_set_string("/GnomeCard/file/open",  gnomecard_fname);
	}
}

extern void gnomecard_open(GtkWidget *widget, gpointer data)
{
	GtkWidget *fsel;
	
	fsel = gtk_file_selection_new(_("Open GnomeCard File"));
	gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(fsel));
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fsel)->ok_button),
			   "clicked", GTK_SIGNAL_FUNC(gnomecard_open_call),
			   GTK_OBJECT(fsel));
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fsel)->cancel_button),
			   "clicked", GTK_SIGNAL_FUNC(gnomecard_cancel),
			   GTK_OBJECT(fsel));
	gtk_widget_show(fsel);
}

static void gnomecard_save_call(GtkWidget *widget, gpointer data)
{
	g_free(gnomecard_fname);
	gnomecard_fname = g_strdup(gtk_file_selection_get_filename(GTK_FILE_SELECTION(widget)));
	gtk_widget_destroy(widget);
	
	gnomecard_save();
}

extern void gnomecard_save_as(GtkWidget *widget, gpointer data)
{
	GtkWidget *fsel;
	
	fsel = gtk_file_selection_new(_("Save GnomeCard File As..."));
	gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(fsel));
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fsel)->ok_button),
			   "clicked", GTK_SIGNAL_FUNC(gnomecard_save_call),
			   GTK_OBJECT(fsel));
	gtk_signal_connect_object(GTK_OBJECT(GTK_FILE_SELECTION(fsel)->cancel_button),
			   "clicked", GTK_SIGNAL_FUNC(gnomecard_cancel),
			   GTK_OBJECT(fsel));
	gtk_widget_show(fsel);
}

extern void gnomecard_about(GtkWidget *widget, gpointer data)
{
	GtkWidget *about;
	const gchar *authors[] = { "arturo@nuclecu.unam.mx", 
				   "drmike@redhat.com",
				   NULL };
	
	about = gnome_about_new (_("GnomeCard"), NULL,
				 "(C) 1997-1998 the Free Software Fundation",
				 authors,
				 _("Electronic Business Card Manager"),
				 NULL);
	gtk_widget_show (about);
}

static int gnomecard_match_pattern(char *pattern, char *str, int sens)
{
	char *txt;
	int found = 0;
	
	if (! str)
		return 0;
	
	if (sens)
		txt = str;
	else
		g_strup(txt = g_strdup(str));
	
	if (fnmatch(pattern, txt, 0) == 0) {
		found = 1;
	}
	
	if (! sens)
		g_free(txt);
	
	return found;
}

static void gnomecard_find_card(GtkWidget *w, gpointer data)
{
	GnomeCardFind *p;
	GList *l, *k;
	Card *crd;
	char *pattern, *crd_text[15];
	int i, wrapped, found, sens, back;
	
	p = (GnomeCardFind *) data;

	found = 0;
	wrapped = 0;

	if (GTK_TOGGLE_BUTTON(p->back)->active)
		gnomecard_find_back = back = 1;
	else
		gnomecard_find_back = back = 0;
	
	MY_FREE(gnomecard_find_str);
	gnomecard_find_str = g_strdup(gtk_entry_get_text(GTK_ENTRY(p->entry)));
	pattern = g_malloc(strlen(gnomecard_find_str) + 3);
	sprintf(pattern, "*%s*", gnomecard_find_str);

	if (GTK_TOGGLE_BUTTON(p->sens)->active)
		gnomecard_find_sens = sens = 1;
	else {
		gnomecard_find_sens = sens = 0;
		g_strup(pattern);
	}
	
	l = gnomecard_curr_crd;
	
	while (l) {
	    if (wrapped != 1)
		l = (back)? l->prev : l->next;
	    else
		wrapped ++;
	    
	    if (l) {
		crd = l->data;
		crd_text[0] = crd->fname.str;
		crd_text[1] = crd->name.family;
		crd_text[2] = crd->name.given;
		crd_text[3] = crd->name.additional;
		crd_text[4] = crd->name.prefix;
		crd_text[5] = crd->name.suffix;
		crd_text[6] = crd->title.str;
		crd_text[7] = crd->role.str;
		crd_text[8] = crd->comment.str;
		crd_text[9] = crd->url.str;
		crd_text[10] = crd->org.name;
		crd_text[11] = crd->org.unit1;
		crd_text[12] = crd->org.unit2;
		crd_text[13] = crd->org.unit3;
		crd_text[14] = crd->org.unit4;
		
		for (i = 0; i < 15 && !found; i++)
		    if (gnomecard_match_pattern(pattern, crd_text[i], sens))
			found = 1;
		
		for (k = crd->phone.l; k && !found; k = k->next)
		    if (gnomecard_match_pattern(pattern, 
						((CardPhone *) k->data)->data,
						sens))
			found = 1;
		
#if 0			
		for (k = crd->email.l; k && !found; k = k->next)
		    if (gnomecard_match_pattern(pattern, 
						((CardEMail *) k->data)->data,
						sens))
			found = 1;
#else
		g_message("Did not search of email (yet)");
#endif
		
/* NOT IMPLEMENTED
		for (k = crd->dellabel.l; k && !found; k = k->next)
		    if (gnomecard_match_pattern(pattern, 
						((CardDelLabel *)k->data)->data, sens))
			found = 1;
*/		

/* NOT USED
		for (k = crd->deladdr.l; k && !found; k = k->next)
		    for (i = 0; i < DELADDR_MAX; i++)
			if (gnomecard_match_pattern(pattern, 
						    ((CardDelAddr *) k->data)->data[i], sens))
			    found = 1;
*/
		for (k = crd->postal.l; k && !found; k = k->next) {
		    CardPostAddr *p = ((CardPostAddr *)k->data);

		    found = gnomecard_match_pattern(pattern,p->street1,sens) ||
			gnomecard_match_pattern(pattern,p->street2,sens) ||
			gnomecard_match_pattern(pattern,p->city,sens) ||
			gnomecard_match_pattern(pattern,p->state,sens) ||
			gnomecard_match_pattern(pattern,p->zip,sens) ||
			gnomecard_match_pattern(pattern,p->country,sens);
		}
		
		if (found) {
		    gnomecard_scroll_list(l);
		    break;
		}
		
	    }	else if (wrapped) {
		GtkWidget *w;
		
		w = gnome_message_box_new(_("No matching record found."),
					  GNOME_MESSAGE_BOX_ERROR,
					  GNOME_STOCK_BUTTON_OK, NULL);
		GTK_WINDOW(w)->position = GTK_WIN_POS_MOUSE;
		gnome_dialog_button_connect_object(GNOME_DIALOG(w),	0,
						   GTK_SIGNAL_FUNC(gnomecard_cancel),
						   GTK_OBJECT(w));	
		gtk_widget_show(w);
		break;
	    } else {
		GtkWidget *w;
		char msg[128], *str1, *str2;
		
		str1 = (back)? _("first") : _("last");
		str2 = (back)? _("last") : _("first");
		snprintf(msg, 128, _("Reached %s record.\nContinue from the %s one?"),
			 str1, str2);
		w = gnome_message_box_new(msg,
					  GNOME_MESSAGE_BOX_QUESTION,
					  GNOME_STOCK_BUTTON_OK,
					  GNOME_STOCK_BUTTON_CANCEL, NULL);
		GTK_WINDOW(w)->position = GTK_WIN_POS_MOUSE;
		gtk_widget_show(w);
		
		switch(gnome_dialog_run_modal(GNOME_DIALOG(w))) {
		  case -1:
		  case 1:
		    l = NULL;
		    break;
		  case 0:
		    l = (back)? g_list_last(gnomecard_crds) : gnomecard_crds;
		    wrapped = 1;
		}
	    }
	}
	
	g_free(pattern);
	
	gnome_config_set_bool("/GnomeCard/find/sens",  gnomecard_find_sens);
	gnome_config_set_bool("/GnomeCard/find/back",  gnomecard_find_back);
	gnome_config_set_string("/GnomeCard/find/str",  gnomecard_find_str);
}

extern void gnomecard_find_card_call(GtkWidget *widget, gpointer data)
{
	GtkWidget *w, *hbox, *check;
	GnomeCardFind *p;

	p = g_malloc(sizeof(GnomeCardFind));
	w = gnome_dialog_new(_("Find Card"), _("Find"),
			     GNOME_STOCK_BUTTON_CLOSE, NULL);
	gtk_object_set_user_data(GTK_OBJECT(w), p);
	gnome_dialog_button_connect(GNOME_DIALOG(w), 0,
				    GTK_SIGNAL_FUNC(gnomecard_find_card), p);
	gnome_dialog_button_connect_object(GNOME_DIALOG(w),	1,
					   GTK_SIGNAL_FUNC(gnomecard_cancel),
					   GTK_OBJECT(w));	

	p->entry = my_hbox_entry(GNOME_DIALOG(w)->vbox, _("Find:"), gnomecard_find_str);
	hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
	gtk_box_pack_start(GTK_BOX(GNOME_DIALOG(w)->vbox), hbox, FALSE, FALSE, 0);
	p->sens = check = gtk_check_button_new_with_label(_("Case Sensitive"));
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(check), gnomecard_find_sens);
	gtk_box_pack_start(GTK_BOX(hbox), check, FALSE, FALSE, 0);
	p->back = check = gtk_check_button_new_with_label(_("Find Backwards"));
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(check), gnomecard_find_back);
	gtk_box_pack_end(GTK_BOX(hbox), check, FALSE, FALSE, 0);
	
	gtk_widget_show_all(GNOME_DIALOG(w)->vbox);
	gtk_widget_show(w);
}
