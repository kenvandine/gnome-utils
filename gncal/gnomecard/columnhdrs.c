#include <config.h>
#include <gnome.h>

#include "card.h"
#include "gnomecard.h"
#include "my.h"
#include "columnhdrs.h"
#include "phonelist.h"

static ColumnHeader column_hdrs[] = { 
    {N_("Full Name"), COLTYPE_FULLNAME},
    {N_("Card Name"), COLTYPE_CARDNAME},
    {N_("First Name"), COLTYPE_FIRSTNAME},
    {N_("Middle Name"), COLTYPE_MIDDLENAME},
    {N_("Last Name"), COLTYPE_LASTNAME},
    {N_("Prefix"), COLTYPE_PREFIX},
    {N_("Suffix"), COLTYPE_SUFFIX},
    {N_("Organization"), COLTYPE_ORG},
    {N_("Title"), COLTYPE_TITLE},
    {N_("Email"), COLTYPE_EMAIL},
    {N_("Web Page"), COLTYPE_WEBPAGE},
    {N_("Home Phone"), COLTYPE_HOMEPHONE},
    {N_("Business Phone"), COLTYPE_WORKPHONE},
    {NULL, COLTYPE_END},
};

		      
/* NULL means no match */
gchar *
getColumnNameFromType(ColumnType type)
{
    gint i;

    for (i=0; column_hdrs[i].colname; i++)
	if (column_hdrs[i].coltype == type)
	    break;

    return column_hdrs[i].colname;
}

/* < 0 means no match */
ColumnType
getColumnTypeFromName(gchar *name)
{
    gint i;

    for (i=0; column_hdrs[i].colname; i++)
	if (!strcmp(column_hdrs[i].colname, name))
	    break;

    return column_hdrs[i].coltype;
}

/* NULL means no match */
ColumnHeader *
getColumnHdrFromType(ColumnType type)
{
    gint i;

    for (i=0; column_hdrs[i].colname; i++)
	if (column_hdrs[i].coltype == type)
	    break;

    return (column_hdrs[i].colname) ? &column_hdrs[i] : NULL;
}


gint 
numColumnHeaders(GList *cols)
{
    return g_list_length(cols);
}


GList *
buildColumnHeaders(ColumnType *cols)
{
    GList *l=NULL;
    gint  i;
    ColumnHeader *p;

    i = 0;
    while (cols[i] != COLTYPE_END) {
	p = getColumnHdrFromType(cols[i]);
	if (p)
	    l = g_list_append(l,p);
	i++;
    }
	
    return l;
}



gchar *
getValFromColumnHdr(Card *crd, ColumnHeader *hdr)
{
    gchar *allocstr=NULL;
    gchar *unallocstr=NULL;

    switch (hdr->coltype) {
      case COLTYPE_FULLNAME:
	allocstr = gnomecard_join_name(crd->name.prefix, crd->name.given, 
				  crd->name.additional, crd->name.family, 
				  crd->name.suffix);
	break;

      case COLTYPE_CARDNAME:
	if (crd->fname.str)
	    unallocstr = crd->fname.str;
	break;

      case COLTYPE_FIRSTNAME:
	if (crd->name.given)
	    unallocstr = crd->name.given;
	break;

      case COLTYPE_MIDDLENAME:
	if (crd->name.additional)
	    unallocstr = crd->name.additional;
	break;

      case COLTYPE_LASTNAME:
	if (crd->name.family)
	    unallocstr = crd->name.family;
	break;

      case COLTYPE_PREFIX:
	if (crd->name.prefix)
	    unallocstr = crd->name.prefix;
	break;

      case COLTYPE_SUFFIX:
	if (crd->name.suffix)
	    unallocstr = crd->name.suffix;
	break;

      case COLTYPE_ORG:
	if (crd->org.name)
	    unallocstr = crd->org.name;
	break;

      case COLTYPE_TITLE:
	if (crd->title.str)
	    unallocstr = crd->title.str;
	break;

      case COLTYPE_EMAIL:
	if (crd->email.address)
	    unallocstr = crd->email.address;
	break;

      case COLTYPE_WEBPAGE:
	if (crd->url.str)
	    unallocstr = crd->url.str;
	break;

      case COLTYPE_HOMEPHONE:
	if (crd->phone.l) {
	    GList *l;

	    l = findmatchPhoneType(crd->phone.l, PHONE_HOME);
	    if (l) {
		CardPhone *phone;
		
		phone = (CardPhone *)l->data;
		unallocstr = phone->data;
	    }
	}
	break;

      case COLTYPE_WORKPHONE:
	if (crd->phone.l) {
	    GList *l;

	    l = findmatchPhoneType(crd->phone.l, PHONE_WORK);
	    if (l) {
		CardPhone *phone;
		
		phone = (CardPhone *)l->data;
		unallocstr = phone->data;
	    }
	}
	break;
	
			 
		
      default:
	break;
    }

    if (allocstr)
	return allocstr;
    else if (unallocstr)
	return g_strdup(unallocstr);
    else
	return NULL;
}

    
/* return GList of all column headers */
GList *
getAllColumnHdrs(void)
{
    GList *l=NULL;
    ColumnHeader *p;


    for ( p = column_hdrs; p->coltype != COLTYPE_END; p++ )
	l = g_list_append(l,p);
	
    return l;
}
    
