#include <config.h>
#include <gnome.h>

#include "card.h"
#include "dialog.h"
#include "my.h"
#include "phonelist.h"


static void phonetypeclicked(GtkWidget *widget, gpointer data);
static void gnomecard_phone_entry_change(GtkWidget *w, gpointer data);
static void gnomecard_phoneprop_used(GtkWidget *w, gpointer data);
static void phone_connect(gpointer widget, char *sig, gpointer box, 
		 CardProperty *prop, enum PropertyType type);



gchar *phone_type_name[]={ N_("Preferred"), N_("Work"), N_("Home"), 
			   N_("Voice"), N_("Fax"), N_("Message Recorder"),
			   N_("Cellular"), N_("Pager"), N_("Bulletin Board"),
			   N_("Modem"), N_("Car"), N_("ISDN"), N_("Video"), 
			   NULL };

static ignore_phone_changes = FALSE;

/* avoid getting sending changes to property box when we are manually */
/* manipulating phone entries                                       */
static void
gnomecard_phone_entry_change(GtkWidget *w, gpointer data) {
    
    if (!ignore_phone_changes)
	gnome_property_box_changed(data);
}

static void
gnomecard_phoneprop_used(GtkWidget *w, gpointer data)
{
    CardProperty *prop;
    
    prop = (CardProperty *) data;
    prop->type = (int) gtk_object_get_user_data(GTK_OBJECT(w));
    prop->used = TRUE;
}

static void
phone_connect(gpointer widget, char *sig, gpointer box, 
		 CardProperty *prop, enum PropertyType type)
{
 	gtk_signal_connect_object(GTK_OBJECT(widget), sig,
				  GTK_SIGNAL_FUNC(gnomecard_phone_entry_change),
				  GTK_OBJECT(box));
	gtk_signal_connect(GTK_OBJECT(widget), sig,
			   GTK_SIGNAL_FUNC(gnomecard_phoneprop_used),
			   prop);
	gtk_object_set_user_data(GTK_OBJECT(widget), (gpointer) type);
}


static void
phonetypeclicked(GtkWidget *widget, gpointer data)
{
    gint num;
    GnomeCardEditor *ce;

    /* what is the phone type of button causing this event */
    num = GPOINTER_TO_INT(gtk_object_get_user_data(GTK_OBJECT(widget)));
    ce  = (GnomeCardEditor *)data;

    /* if we are turning off a button, copy entry data into state vars */
    if (!GTK_TOGGLE_BUTTON(widget)->active) {
	copyGUIToCurPhone(ce);
	return;
    } else {
	ce->curphone = num;
	copyCurPhoneToGUI(ce);
    }
}

/* Functions for handling phone numbers */
/* delete card list of phone numbers from src, freeing as we go */
void
deletePhoneList(CardList *src)
{
    GList *l;
    CardPhone *p;

    for (l=src->l; l; l=l->next) {
	p = (CardPhone *)l->data;
	MY_FREE(p->data);
    }
    
    g_list_free(src->l);
    src->l = NULL;
}

/* copy card list of phone from src into dest, allocating as we go */
void
copyPhoneList(CardList src, CardList *dest)
{
    GList *l;

    for (l=src.l; l; l=l->next) {
	CardPhone *p, *srcp;

	srcp = (CardPhone *)l->data;
	p = g_new0(CardPhone, 1);
	p->prop = empty_CardProperty();
	p->prop = srcp->prop;

	p->type    = srcp->type;
	p->data    = g_strdup(srcp->data);
	
	dest->l = g_list_append(dest->l, p);
   }
}

/* try to find specific phone type in a list of phone numbers */
/* returns ptr to list item if found, otherwise return NULL   */
GList *
findmatchPhoneType(GList *l, gint type)
{
    GList *k;

    for (k = l; k; k = k->next) {
	CardPhone *p = ((CardPhone *)k->data);
	
	if ( p->type == type )
	    break;
    }

    return k;
}


/* set address edit fields according to the current address type */
/* assumes ce is a valid structure from call to gnomecard_edit() */
void 
copyCurPhoneToGUI(GnomeCardEditor *ce) {
    CardPhone *p;
    GList *l;

    l = findmatchPhoneType(ce->phone.l, ce->curphone);
    if (!l) {
	CardPhone *phone;

	/* create new phone type */
	g_message("Creating phonetype %d in copyCurPhoneToGUI()",ce->curphone);
	phone = g_new0(CardPhone, 1);
	phone->data = g_strdup("");
	phone->type    = ce->curphone;
	phone->prop    = empty_CardProperty();
	phone->prop.used = TRUE;

	ce->phone.l = g_list_append(ce->phone.l, phone);
	l = findmatchPhoneType(ce->phone.l, ce->curphone);
    }

    /* copy phone into GUI */
    ignore_phone_changes = TRUE;
    p = ((CardPhone *)l->data);
    gtk_entry_set_text(GTK_ENTRY(ce->phoneentry), (p->data) ? p->data : "");
    ignore_phone_changes = FALSE;
}

/* copies GUI of phone entry into current data for phone type */
void 
copyGUIToCurPhone(GnomeCardEditor *ce) {
    CardPhone *p;
    GList *l;

    l = findmatchPhoneType(ce->phone.l, ce->curphone);
    if (!l) {
	CardPhone *phone;

	/* create new phone type */
	g_message("Creating phone type %d in copyGUIToCurPhone()",ce->curphone);
	phone = g_new0(CardPhone, 1);
	phone->data = NULL;
	phone->prop    = empty_CardProperty();
	phone->prop.used = TRUE;

	ce->phone.l = g_list_append(ce->phone.l, phone);
	l = findmatchPhoneType(ce->phone.l, ce->curphone);
    }

    /* copy GUI into card record */
    p = ((CardPhone *)l->data);
    MY_FREE(p->data);
    p->data = g_strdup(gtk_entry_get_text(GTK_ENTRY(ce->phoneentry)));
}



GtkWidget *
gnomecard_create_phone_page(Card *crd, GnomeCardEditor *ce,
			    GnomePropertyBox *box)
{
    GtkWidget *label, *frame, *align, *table, *thebox, *entry;
    GtkWidget *phonehbox, *phonetypebox, *phonetypeframe, *phonevbox;
    gint      i;
    GSList    *phonetypegroup=NULL;

    /* Phone number page */
    thebox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
    
    /* make a frame for the phone entry area */
    frame = gtk_frame_new(_("Phone"));
    gtk_box_pack_start(GTK_BOX(thebox), frame, TRUE, TRUE, 0);
    
    /* make a hbox for entire phone entry area */
    phonehbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
    gtk_container_add(GTK_CONTAINER(frame), phonehbox);
    
    /* the phone type entry area */
    phonetypebox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
    gtk_box_pack_end(GTK_BOX(phonehbox), phonetypebox, FALSE, FALSE, 
		     GNOME_PAD_SMALL);
    
    phonetypeframe = gtk_frame_new(_("Phone # Type:"));
    gtk_frame_set_label_align(GTK_FRAME(phonetypeframe), 0.5, 0.5);
    gtk_box_pack_end(GTK_BOX(phonetypebox), phonetypeframe,FALSE,FALSE, 0);
    
    /* enter phone # types (based on Vcal standard I guess) */
    phonevbox = gtk_vbox_new(FALSE, GNOME_PAD_SMALL);
    gtk_container_add(GTK_CONTAINER(phonetypeframe), phonevbox);
    
    /* default to the 1st phone type */
    ce->curphone = 1;
    for (i = 0; i < 6; i++) {
	ce->phonetype[i]=gtk_radio_button_new_with_label(phonetypegroup,
							 phone_type_name[i]);
	phonetypegroup = gtk_radio_button_group(GTK_RADIO_BUTTON(ce->phonetype[i]));
	gtk_object_set_user_data(GTK_OBJECT(ce->phonetype[i]),
				 (gpointer) (1 << i));
	gtk_box_pack_start(GTK_BOX(phonevbox), ce->phonetype[i],
			   FALSE, FALSE, 0);
	
	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(ce->phonetype[i]),
				    (i == 0) );
	
    }
    
    /* make the actual entry boxes for entering the phone number */
    table = my_gtk_table_new(6, 2);
    gtk_box_pack_end(GTK_BOX(phonehbox), table, TRUE, TRUE, 0);
    
    label = gtk_label_new(_("Phone Number:"));
    align = gtk_alignment_new(1.0, 0.5, 0, 0);
    gtk_container_add (GTK_CONTAINER (align), label);
    gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
    ce->phoneentry = entry = my_gtk_entry_new(0, "");
    gtk_table_attach(GTK_TABLE(table), align, 0, 1, 0, 1,
		     GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK, 
		     0, 0);
    gtk_table_attach(GTK_TABLE(table), entry, 1, 2, 0, 1,
		     GTK_FILL | GTK_SHRINK, GTK_FILL | GTK_SHRINK,
		     GNOME_PAD_SMALL, GNOME_PAD_SMALL);
    
    /* fill in data structures for the phone */
    copyPhoneList(crd->phone, &ce->phone);
    
    /* prime the GUI */
    copyCurPhoneToGUI(ce);
    
    /* attach signals now we're finished */
    for (i=0; i < 6; i++)
	gtk_signal_connect(GTK_OBJECT(ce->phonetype[i]), "clicked",
			   GTK_SIGNAL_FUNC(phonetypeclicked),
			   ce);
    
    phone_connect(ce->phoneentry, "changed", box, &crd->phone.prop, PROP_PHONE);

    return thebox;
}

