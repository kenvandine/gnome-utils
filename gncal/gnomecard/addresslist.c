#include <config.h>
#include <gnome.h>

#include "card.h"
#include "dialog.h"
#include "my.h"
#include "addresslist.h"

static void addrtypeclicked(GtkWidget *widget, gpointer data);
static void gnomecard_postaddr_entry_change(GtkWidget *w, gpointer data);
static void gnomecard_addrprop_used(GtkWidget *w, gpointer data);
static void postaddr_connect(gpointer widget, char *sig, gpointer box, 
		 CardProperty *prop, enum PropertyType type);


gchar *addr_type_name[] = { N_("Home"), N_("Work"), N_("Postal Box"), 
			    N_("Parcel"), N_("Domestic"), N_("International"),
			    NULL };

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


/* delete card list of addresses from src, freeing as we go */
void
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
void
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
void 
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
void 
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

    /* copy GUI into card record */
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

GtkWidget *
gnomecard_create_address_page(Card *crd, GnomeCardEditor *ce, 
			      GnomePropertyBox *box)
{
    GtkWidget *label, *frame, *align, *table, *hbox, *entry;
    GtkWidget *addrhbox, *addrvbox, *addrtypebox;
    GtkWidget *addrtypeframe;
    gint      i;
    GSList    *addrtypegroup=NULL;


    hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
    /* make a frame for the address entry area */
    frame = gtk_frame_new(_("Address"));
    gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 0);
    
    /* make a hbox for entire address entry area */
    addrhbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);
    gtk_container_add(GTK_CONTAINER(frame), addrhbox);
    
    /* the address type entry area */
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


    return hbox;
}
