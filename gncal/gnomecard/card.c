#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pairs.h"
#include "card.h"

#define is_a_prop_of(obj,prop) isAPropertyOf (obj,prop)
#define str_val(obj) the_str = fakeCString (vObjectUStringZValue (obj))
#define has(obj,prop) (vo = isAPropertyOf (obj, prop))

typedef struct
{
	char c;
	int id;
	
	GList *sons;
} tree;

static CardProperty 
empty_CardProperty(void)
{
	CardProperty prop;
	
	prop.used = 0;
	prop.encod = ENC_7BIT;
	prop.value = VAL_INLINE;
	prop.charset = NULL;
	prop.lang = NULL;
	prop.grp = NULL;
	
	return prop;
}

static CardStrProperty 
empty_CardStrProperty(void)
{
	CardStrProperty strprop;
	
	strprop.prop = empty_CardProperty();
	strprop.str = NULL;
	
	return strprop;
}

#define _FREE(a) if (a) free (a)

static void
free_CardProperty(CardProperty prop)
{
	CardGroup *node, *node2;
	
	_FREE(prop.charset);
	_FREE(prop.lang);
	
	for (node = prop.grp; node; node = node2) {
		node2 = node->parent;
		free(node);
	}
}
	
Card *
card_new(void)
{
	Card *c;
	
	c = malloc(sizeof(Card));

	c->name.family = c->name.given  = c->name.additional = NULL;
	c->name.prefix = c->name.suffix = NULL;
	c->photo.type = PHOTO_JPEG;
	c->photo.size = 0;
	c->photo.data = NULL;
	c->bday.year = c->bday.month = c->bday.day = 0;
	c->deladdr  = NULL;
	c->dellabel = NULL;
	c->phone    = NULL;
	c->email    = NULL;
	c->timezn.sign  = 1;
	c->timezn.hours = c->timezn.mins = 0;
	c->geopos.lon   = c->geopos.lat  = 0;
	c->logo.type = PHOTO_JPEG;
	c->logo.size = 0;
	c->logo.data = NULL;
	c->agent = NULL;
	c->org.name = c->org.unit1 = c->org.unit2 = 
	              c->org.unit3 = c->org.unit4 = NULL;
	c->rev.utc = -1;
	c->sound.type = SOUND_PHONETIC;
	c->sound.size = 0;
	c->sound.data = NULL;
	c->key.type = KEY_PGP;
	c->key.data = NULL;
	
	c->fname = c->mailer  = c->title = c->role  = c->comment = 
	c->url   = c->uid     = empty_CardStrProperty();
	
	c->name.prop    = c->photo.prop    = c->bday.prop   = c->timezn.prop = 
	c->geopos.prop  = c->logo.prop     = c->org.prop    = c->rev.prop    =
	c->sound.prop   = c->key.prop = empty_CardProperty();
	
	return c;
}

void 
card_free(Card *crd)
{
	g_return_if_fail (crd != NULL);
	  
	_FREE(crd->name.family); free_CardProperty(crd->name.prop);
	_FREE(crd->name.given);
	_FREE(crd->name.additional);
	_FREE(crd->name.prefix);
	_FREE(crd->name.suffix);
	_FREE(crd->photo.data); free_CardProperty(crd->photo.prop);
	_FREE(crd->logo.data); free_CardProperty(crd->logo.prop);
	_FREE(crd->org.name); free_CardProperty(crd->org.prop);
	_FREE(crd->org.unit1);
	_FREE(crd->org.unit2);
	_FREE(crd->org.unit3);
	_FREE(crd->org.unit4);
	_FREE(crd->key.data); free_CardProperty(crd->key.prop);
	_FREE(crd->sound.data); free_CardProperty(crd->sound.prop);
	_FREE(crd->fname.str); free_CardProperty(crd->fname.prop);
	_FREE(crd->mailer.str); free_CardProperty(crd->mailer.prop);
	_FREE(crd->title.str); free_CardProperty(crd->title.prop);
	_FREE(crd->role.str); free_CardProperty(crd->role.prop);
	_FREE(crd->comment.str); free_CardProperty(crd->comment.prop);
	_FREE(crd->url.str); free_CardProperty(crd->url.prop);
	_FREE(crd->uid.str); free_CardProperty(crd->uid.prop);
	
	free(crd);
}

static tree *
new_tree(char c, int id)
{
	tree *t;
	
	t = malloc(sizeof(tree));
	t->c = c;
	t->id = id;
	t->sons = NULL;
	
	return t;
}

static void 
add_branch(tree *t, char *str, int id)
{
	tree *tmp;
	char *end;
	
	end = str + strlen(str) + 1;

	while (str != end) {
		tmp = new_tree(*str, id);
		t->sons = g_list_append(t->sons, (gpointer) tmp);
		t = tmp;
		
		str ++;
	}
}

static tree *
add_to_tree(tree *t, struct pair p)
{
	GList *node;
	char *c, *end;
	tree *tmp;
	
	  c = p.str;
	end = c + strlen(c) + 1;
	tmp = t;
	
	while (c != end) {
		for (node = tmp->sons; node; node = node->next)
		  if (((tree *) node->data)->c == *c) {
			  break;
		  }
		
		if (node) {
			tmp = (tree *) node->data;
			tmp->id = 0;
			c++;
		}
		else {
			add_branch(tmp, c, p.id);
			break;
		}
	}
	
	return t;
}
		
static tree *
create_search_tree(void)
{
	tree *t;
	int i;

	t = new_tree(0, 0);
	for (i = 0; prop_lookup[i].str; i++)
	  t = add_to_tree(t, prop_lookup[i]);
	
	return t;
}
		
static int 
card_lookup_name (const char *c)
{
	static tree *search_tree = NULL;
	GList *node;
	tree *tmp;
	const char *end;
	
	if (!search_tree)
	  search_tree = create_search_tree();
	
	tmp = search_tree;
	end = c + strlen(c) + 1;
	
	while (tmp->id == 0 && c != end) {
		for (node = tmp->sons; node; node = node->next)
		  if (((tree *) node->data)->c == *c) {
			  break;
		  }
	
		if (node) {
			tmp = (tree *) node->data;
			c++;
		}
		else
		  return 0;
	}
	
	return tmp->id;
}

static enum PhotoType 
get_photo_type(VObject *o)
{
	VObject *vo;
	int i;
	
	for (i = 0; photo_pairs[i].str; i++)
	  if (has (o, photo_pairs[i].str))
	    return photo_pairs[i].id;

	g_warning("? < No PhotoType for Photo property. Falling back to JPEG.");
	return PHOTO_JPEG;
}

static int 
get_addr_type(VObject *o)
{
	VObject *vo;
	int ret = 0;
	int i;
	
	for (i = 0; addr_pairs[i].str; i++)
	  if (has (o, addr_pairs[i].str))
	    ret |= addr_pairs[i].id;
	
	return ret;
}

static int 
get_phone_type(VObject *o)
{
	VObject *vo;
	int ret = 0;
	int i;
	
	for (i = 0; phone_pairs[i].str; i++)
	  if (has (o, phone_pairs[i].str))
	    ret |= phone_pairs[i].id;
	
	return ret;
}

static enum EMailType 
get_email_type(VObject *o)
{
	VObject *vo;
	int i;

	for (i = 0; email_pairs[i].str; i++)
	  if (has (o, email_pairs[i].str))
	    return email_pairs[i].id;

	g_warning("? < No EMailType for EMail property. Falling back to INET.");
	return EMAIL_INET;
}

static CardProperty 
get_CardProperty(VObject *o)
{
	VObject *vo;
	char *the_str;
	CardProperty prop;

	prop = empty_CardProperty();
	prop.used = 1;
	
	if (has (o, VCValueProp)) {
		if (has (vo, VCContentIDProp))
		  prop.value = VAL_CID;
		else if (has (vo, VCURLValueProp))
		  prop.value = VAL_URL;
	}
	
	if (has (o, VCEncodingProp)) {
		if (has (vo, VCQuotedPrintableProp))
		  prop.encod = ENC_QUOTED_PRINTABLE;
		else if (has (vo, VC8bitProp))
		  prop.encod = ENC_8BIT;
		else if (has (vo, VCBase64Prop))
		  prop.encod = ENC_BASE64;
	} else if (has (o, VCQuotedPrintableProp))
		  prop.encod = ENC_QUOTED_PRINTABLE;
	else if (has (o, VC8bitProp))
	  prop.encod = ENC_8BIT;
	else if (has (o, VCBase64Prop))
	  prop.encod = ENC_BASE64;
	
	if (has (o, VCLanguageProp)) {
		prop.lang = g_strdup(str_val(vo));
		free(the_str);
	}
	
	if (has (o, VCCharSetProp)) {
		prop.charset = g_strdup(str_val(vo));
		free(the_str);
	}

	return prop;
}

static CardName 
get_CardName(VObject *o)
{
	VObject *vo;
	char *the_str;
	CardName name;

	name.family = NULL;
	name.given = NULL;
	name.additional = NULL;
	name.prefix = NULL;
	name.suffix = NULL;
	
	if (has (o, VCFamilyNameProp)) {
		name.family = g_strdup(str_val(vo));
		free(the_str);
	}
	if (has (o, VCGivenNameProp)) {
		name.given = g_strdup(str_val(vo));
		free(the_str);
	}
	if (has (o, VCAdditionalNamesProp)) {
		name.additional = g_strdup(str_val(vo));
		free(the_str);
	}
	if (has (o, VCNamePrefixesProp)) {
		name.prefix = g_strdup(str_val(vo));
		free(the_str);
	}
	if (has (o, VCNameSuffixesProp)) {
		name.suffix = g_strdup(str_val(vo));
		free(the_str);
	}
	
	return name;
}

static CardBDay 
strtoCardBDay(char *str)
{
	char *s;
	int i;
	CardBDay bday;

	bday.year = 0;
	bday.month = 0;
	bday.day = 0;
	
	if (strchr(str, '-')) {
		for (s = strtok (str, "-"), i = 0; s; 
		     s = strtok (NULL, "-"), i++)
		  switch (i) {
		   case 0:
			  bday.year = atoi(s);
			  break;
		   case 1:
			  bday.month = atoi(s);
			  break;
		   case 2:
			  bday.day = atoi(s);
			  break;
		   default:
			  g_warning("? < Too many values for BDay property.");
		  }
		
		if (i < 2)
		  g_warning ("? < Too few values for BDay property.");
	} else {
		if (strlen(str) >= 8) {
			bday.day = atoi(str + 6);
			str[6] = 0;
			bday.month = atoi(str + 4);
			str[4] = 0;
			bday.year = atoi(str);
		} else
		  g_warning("? < Bad format for BDay property.");
	}
	
	return bday;
}

static CardDelAddr *
get_CardDelAddr(VObject *o)
{
	VObject *vo;
	char *the_str;
	CardDelAddr *addr;
	
	addr = malloc(sizeof(CardDelAddr));
	
	addr->po = NULL;
	addr->ext = NULL;
	addr->street = NULL;
	addr->locality = NULL;
	addr->region = NULL;
	addr->code = NULL;
	addr->country = NULL;
	addr->type = get_addr_type(o);
	addr->prop = get_CardProperty(o);
	
	if (has (o, VCPostalBoxProp)) {
		addr->po = g_strdup(str_val(vo));
		free(the_str);
	}
	if (has (o, VCExtAddressProp)) {
		addr->ext = g_strdup(str_val(vo));
		free(the_str);
	}
	if (has (o, VCStreetAddressProp)) {
		addr->street = g_strdup(str_val(vo));
		free(the_str);
	}
	if (has (o, VCLocationProp)) {
		addr->locality = g_strdup(str_val(vo));
		free(the_str);
	}
	if (has (o, VCRegionProp)) {
		addr->region = g_strdup(str_val(vo));
		free(the_str);
	}
	if (has (o, VCPostalCodeProp)) {
		addr->code = g_strdup(str_val(vo));
		free(the_str);
	}
	if (has (o, VCCountryNameProp)) {
		addr->country = g_strdup(str_val(vo));
		free(the_str);
	}
	
	return addr;
}
	
static CardDelLabel *
get_CardDelLabel(VObject *o)
{
	CardDelLabel *dellabel;
	char *the_str;
	
	dellabel = malloc(sizeof(CardDelLabel));
	
	dellabel->type = get_addr_type(o);
	dellabel->data = g_strdup(str_val(o));
	dellabel->prop = get_CardProperty(o);
	
	return dellabel;
}


static CardPhone *
get_CardPhone(VObject *o)
{
	CardPhone *ret;
	char *the_str;
	
	ret = malloc(sizeof(CardPhone));
	ret->type = get_phone_type(o);
	ret->data = g_strdup(str_val(o));
	ret->prop = get_CardProperty(o);
	
	free(the_str);

	return ret;
}

static CardEMail *
get_CardEMail(VObject *o)
{
	CardEMail *ret;
	char *the_str;
	
	ret = malloc(sizeof(CardEMail));
	ret->type = get_email_type(o);
	ret->data = g_strdup(str_val(o));
	ret->prop = get_CardProperty(o);
	
	free(the_str);
	
	return ret;
}
	
static CardTimeZone 
strtoCardTimeZone(char *str)
{
	char s[3];
	CardTimeZone tz;
	
	if (*str == '-') {
		tz.sign = -1;
		str++;
	} else
	  tz.sign = 1;
	
	tz.hours = 0;
	tz.mins = 0;
	
	s[3] = 0;
	if (strlen(str) > 2) {
		s[0] = str[0];
		s[1] = str[1];
		tz.hours = atoi(s);
	} else {
		g_warning("? < TimeZone value is too short.");
		return tz;
	}
	
	str += 2;
	if (*str == ':')
	  str++;
	
	if (strlen(str) >= 2) {
		s[0] = str[0];
		s[1] = str[1];
		tz.mins = atoi(s);
	} else {
		g_warning("? < TimeZone value is too short.");
		return tz;
	}
	
	if (strlen(str) > 3)
		g_warning("? < TimeZone value is too long.");

	return tz;
}

static CardGeoPos 
strtoCardGeoPos(char *str)
{
	CardGeoPos gp;
	char *s;
	
	gp.lon = 0;
	gp.lat = 0;
	  
	s = strchr(str, ',');
	
	if (! s) {
		g_warning("? < Bad format for GeoPos property.");
		return gp;
	}
	
	*s = 0;
	s++;
	
	gp.lon = atof(str);
	gp.lat = atof(s);
	
	return gp;
}
	
static CardOrg 
get_CardOrg(VObject *o)
{
	VObject *vo;
	char *the_str;
	CardOrg org;

	org.name = NULL;
	org.unit1 = NULL;
	org.unit2 = NULL;
	org.unit3 = NULL;
	org.unit4 = NULL;

	if (has (o, VCOrgNameProp)) {
		org.name = g_strdup(str_val(vo));
		free(the_str);
	}
	if (has (o, VCOrgUnitProp)) {
		org.unit1 = g_strdup(str_val(vo));
		free(the_str);
	}
	if (has (o, VCOrgUnit2Prop)) {
		org.unit2 = g_strdup(str_val(vo));
		free(the_str);
	}
	if (has (o, VCOrgUnit3Prop)) {
		org.unit3 = g_strdup(str_val(vo));
		free(the_str);
	}
	if (has (o, VCOrgUnit4Prop)) {
		org.unit4 = g_strdup(str_val(vo));
		free(the_str);
	}
	
	return org;
}

static CardRev 
strtoCardRev(char *str)
{
	char s[3], *t, *ss;
	int len, i;
	CardRev rev;
	
	rev.utc = 0;
	len = strlen(str);
	
	if (str[len] == 'Z') {              /* Is it UTC? */
		rev.utc = 1;
		str[len] = 0;
	}
	  
	s[3] = 0;
	t = strchr(str, 'T');
	if (t) {                            /* Take the Time */
		*t = 0;
		t++;
		if (strlen(t) > 2) {
			s[0] = t[0];
			s[1] = t[1];
			rev.tm.tm_hour = atoi(s);
		} else {
			g_warning("? < Rev value is too short.");
			return rev;
		}
		
		t += 2;
		if (*t == ':')             /* Ignore ':' separator */
		  t++;
		
		if (strlen(t) > 2) {
			s[0] = t[0];
			s[1] = t[1];
			rev.tm.tm_min = atoi(s);
		} else {
			g_warning("? < Rev value is too short.");
			return rev;
		}
		
		t += 2;
		if (*t == ':')
		  t++;
		
		if (strlen(t) > 2) {
			s[0] = t[0];
			s[1] = t[1];
			rev.tm.tm_sec = atoi(s);
		} else {
			g_warning("? < Rev value is too short.");
			return rev;
		}

		if (strlen(str) > 3)
		  g_warning("? < Rev value is too long.");
		
	} else {
		g_warning("? < No time value for Rev property.");
	}

	/* Now the date (the part before the T) */
	
	if (strchr(str, '-')) {                        /* extended iso 8601 */
		for (ss = strtok (str, "-"), i = 0; ss;
		     ss = strtok (NULL, "-"), i++)
		  switch (i) {
		   case 0:
			  rev.tm.tm_year = atoi(ss);
			  break;
		   case 1:
			  rev.tm.tm_mon = atoi(ss);
			  break;
		   case 2:
			  rev.tm.tm_mday = atoi(ss);
			  break;
		   default:
			  g_warning("? < Too many values for Rev property.");
		  }
		
		if (i < 2)
		  g_warning ("? < Too few values for Rev property.");
	} else {
		if (strlen(str) >= 8) {             /* short representation */
			rev.tm.tm_mday = atoi(str + 6);
			str[6] = 0;
			rev.tm.tm_mon = atoi(str + 4);
			str[4] = 0;
			rev.tm.tm_year = atoi(str);
		} else
		  g_warning("? < Bad format for Rev property.");
	}
	
	return rev;
}
		
static enum KeyType 
get_key_type(VObject *o)
{
	VObject *vo;
	int i;
	
	for (i = 0; key_pairs[i].str; i++)
	  if (has (o, key_pairs[i].str))
	    return key_pairs[i].id;

	g_warning ("? < No KeyType for Key property. Falling back to PGP.");
	return KEY_PGP;
}

static CardPhoto 
get_CardPhoto(VObject *o)
{
	VObject *vo;
	char *the_str;
	CardPhoto photo;

	photo.type = get_photo_type(o);
	
	if (has (o, VCDataSizeProp)) {
		photo.size = vObjectIntegerValue(vo);
		photo.data = malloc(photo.size);
		memcpy (photo.data, vObjectAnyValue(o), photo.size);
	} else {
		photo.size = strlen(str_val(o)) + 1;
		photo.data = g_strdup(the_str);
		free(the_str);
	}
	
	return photo;
}

static enum SoundType 
get_sound_type(VObject *o)
{
	VObject *vo;
	int i;
	
	for (i = 0; sound_pairs[i].str; i++)
	  if (has (o, sound_pairs[i].str))
	    return sound_pairs[i].id;

	return SOUND_PHONETIC;
}
	
static CardSound 
get_CardSound(VObject *o)
{
	VObject *vo;
	char *the_str;
	CardSound sound;

	sound.type = get_sound_type(o);
	
	if (has (o, VCDataSizeProp)) {
		sound.size = vObjectIntegerValue(vo);
		sound.data = malloc(sound.size);
		memcpy (sound.data, vObjectAnyValue(o), sound.size);
	} else {
		sound.size = strlen(str_val(o));
		sound.data = g_strdup(the_str);
		free(the_str);
	}
	
	return sound;
}

/* Loads our card contents from a VObject */
static Card *
card_create_from_vobject (VObject *vcrd)
{
	VObjectIterator i;
	Card *crd;
	char *the_str;

	initPropIterator (&i, vcrd);
	crd = card_new();

	while (moreIteration (&i)) {
		VObject *o = nextVObject (&i);
		const char *n = vObjectName (o);
		int propid;
		CardProperty *prop = NULL;

		propid = card_lookup_name(n);
		
		switch (propid) {
		 case FNAME:
			prop = &crd->fname.prop;
			crd->fname.str = g_strdup(str_val(o));
			free(the_str);
			break;
		 case NAME:
			prop = &crd->name.prop;
			crd->name = get_CardName(o);
			break;
		 case PHOTO:
			prop = &crd->photo.prop;
			crd->photo = get_CardPhoto(o);
			break;
		 case BDAY:
			prop = &crd->bday.prop;
			crd->bday = strtoCardBDay(str_val(o));
			free(the_str);
			break;
		 case DELADDR:
			crd->deladdr = g_list_append(crd->deladdr,
						     get_CardDelAddr(o));
			break;
		 case DELLABEL:
			crd->dellabel = g_list_append(crd->dellabel,
						     get_CardDelLabel(o));
			break;
		 case PHONE:
			crd->phone = g_list_append(crd->phone,
						   get_CardPhone(o));
			break;
		 case EMAIL:
			crd->email = g_list_append(crd->email,
						   get_CardEMail(o));
			break;
		 case MAILER:
			prop = &crd->mailer.prop;
			crd->mailer.str = g_strdup(str_val(o));
			free(the_str);
			break;
		 case TIMEZN:
			prop = &crd->timezn.prop;
			crd->timezn = strtoCardTimeZone(str_val(o));
			break;
		 case GEOPOS:
			prop = &crd->geopos.prop;
			crd->geopos = strtoCardGeoPos(str_val(o));
			break;
		 case TITLE:
			prop = &crd->title.prop;
			crd->title.str = g_strdup(str_val(o));
			free(the_str);
			break;
		 case ROLE:
			prop = &crd->role.prop;
			crd->role.str = g_strdup(str_val(o));
			free(the_str);
			break;
		 case LOGO:
			prop = &crd->logo.prop;
			crd->logo = get_CardPhoto(o);
			break;
		 case AGENT:
			crd->agent = card_create_from_vobject(o);
			break;
		 case ORG:
			prop = &crd->org.prop;
			crd->org = get_CardOrg(o);
			break;
		 case COMMENT:
			prop = &crd->comment.prop;
			crd->comment.str = g_strdup(str_val(o));
			free(the_str);
			break;
		 case REV:
			prop = &crd->rev.prop;
			crd->rev = strtoCardRev(str_val(o));
			free(the_str);
			break;
		 case SOUND:
			prop = &crd->sound.prop;
			crd->sound = get_CardSound(o);
			break;
		 case URL:
			prop = &crd->url.prop;
			crd->url.str = g_strdup(str_val(o));
			free(the_str);
			break;
		 case UID:
			prop = &crd->uid.prop;
			crd->uid.str = g_strdup(str_val(o));
			free(the_str);
			break;
		 case VERSION:
				{
					char *str;
					str = str_val(o);
					if (strcmp(str, "2.1"))
					  g_warning ("? < Version doesn't match.");
					free(the_str);
				}
			break;
		 case KEY:
			prop = &crd->key.prop;
			crd->key.type = get_key_type(o);
			crd->key.data = g_strdup(str_val(o));
			free(the_str);
			break;
		 default:
			g_warning("? < Property lookup returned invalid value.");
		}
		
		if (prop)
		  *prop = get_CardProperty(o);
	}
	
	return crd;
}
		
/* Loads a card from a file */
GList *
card_load (GList *crdlist, char *fname)
{
	VObject *vobj, *tmp;
	
	vobj = Parse_MIME_FromFileName (fname);
	if (!vobj) {
		g_warning("Could not load the cardfile");
		return NULL;
	}

	while (vobj) {
		const char *n = vObjectName(vobj);
		
		if (strcmp(n,VCCardProp) == 0) {
			crdlist = g_list_append(crdlist, (gpointer)
					    card_create_from_vobject (vobj));
		}
		tmp = vobj;
		vobj = nextVObjectInList(vobj);
		cleanVObject(tmp);
	}
		
	cleanStrTbl ();
	return crdlist;
}

static VObject *
add_strProp(VObject *o, const char *id, char *val)
{
	VObject *vo = NULL;
	
	if (val)
	  vo = addPropValue(o, id, val);

	return vo;
}

static VObject *
add_CardProperty(VObject *o, CardProperty *prop)
{
	switch (prop->encod) {
	 case ENC_BASE64:
		addProp(o, VCBase64Prop);
		break;
	 case ENC_QUOTED_PRINTABLE:
		addProp(o, VCQuotedPrintableProp);
		break;
	 case ENC_8BIT:
		addProp(o, VC8bitProp);
		break;
	 case ENC_7BIT:
		/* Do nothing: 7BIT is the default. Avoids file clutter. */
		break;
	 default:
		g_warning("? < Card had invalid encoding type.");
	}
	
	switch (prop->value) {
	 case VAL_CID:
		addProp(o, VCContentIDProp);
		break;
	 case VAL_URL:
		addProp(o, VCURLValueProp);
		break;
	 case VAL_INLINE:
		/* Do nothing: INLINE is the default. Avoids file clutter. */
		break;
	 default:
		g_warning("? < Card had invalid value type.");
	}

	add_strProp(o, VCCharSetProp, prop->charset);
	add_strProp(o, VCLanguageProp, prop->lang);
	
	return o;
}

static VObject *
add_CardStrProperty(VObject *vobj, const char *id, CardStrProperty *strprop)
{
	VObject *vprop;
	
	if (strprop->prop.used) {
		vprop = add_strProp(vobj, id, strprop->str);
		add_CardProperty(vprop, &strprop->prop);
	}
	
	return vobj;
}

static VObject *
add_PhotoType(VObject *o, enum PhotoType photo_type)
{
	int i;
	
	for (i = 0; photo_pairs[i].str; i++)
	  if (photo_type == photo_pairs[i].id) {
		  addProp(o, photo_pairs[i].str);
		  return o;
	  }

	g_warning("? > No PhotoType for Photo property. Falling back to JPEG.");
	addProp(o, VCJPEGProp);
	
	return o;
}

static VObject *
add_AddrType(VObject *o, int addr_type)
{
	int i;
	
	for (i = 0; addr_pairs[i].str; i++)
	  if (addr_type & addr_pairs[i].id)
	    addProp(o, addr_pairs[i].str);
	
	return o;
}

static VObject *
add_PhoneType(VObject *o, int phone_type)
{
	int i;
	
	for (i = 0; phone_pairs[i].str; i++)
	  if (phone_type & phone_pairs[i].id)
	    addProp(o, phone_pairs[i].str);
	
	return o;
}

static VObject *
add_EMailType(VObject *o, enum EMailType email_type)
{
	int i;
	
	for (i = 0; email_pairs[i].str; i++)
	  if (email_type == email_pairs[i].id) {
		  addProp(o, email_pairs[i].str);
		  return o;
	  }

	g_warning("? > No EMailType for EMail property. Falling back to INET.");
	addProp(o, VCInternetProp);
	
	return o;
}

static VObject *
add_KeyType(VObject *o, enum KeyType key_type)
{
	int i;
	
	for (i = 0; key_pairs[i].str; i++)
	  if (key_type == key_pairs[i].id) {
		  addProp(o, key_pairs[i].str);
		  return o;
	  }

	g_warning ("? > No KeyType for Key property. Falling back to PGP.");
	addProp(o, VCPGPProp);
	
	return o;
}

static VObject *
add_SoundType(VObject *o, enum SoundType sound_type)
{
	int i;
	
	for (i = 0; sound_pairs[i].str; i++)
	  if (sound_type == sound_pairs[i].id) {
		  addProp(o, sound_pairs[i].str);
		  return o;
	  }

	return o;
}

VObject *
card_convert_to_vobject(Card *crd)
{
	VObject *vobj, *vprop;
	
	vobj = newVObject(VCCardProp);

	add_CardStrProperty(vobj, VCFullNameProp, &crd->fname);
	if (crd->name.prop.used) {
		vprop = addProp(vobj, VCNameProp);
		add_strProp(vprop, VCFamilyNameProp, crd->name.family);
		add_strProp(vprop, VCGivenNameProp, crd->name.given);
		add_strProp(vprop, VCAdditionalNamesProp, crd->name.additional);
		add_strProp(vprop, VCNamePrefixesProp, crd->name.prefix);
		add_strProp(vprop, VCNameSuffixesProp, crd->name.suffix);
		add_CardProperty(vprop, &crd->name.prop);
	}
	if (crd->photo.prop.used) {
		vprop = addPropSizedValue(vobj, VCPhotoProp, 
					  crd->photo.data, crd->photo.size);
		add_PhotoType(vprop, crd->photo.type);
		add_CardProperty(vprop, &crd->photo.prop);
	}
	if (crd->bday.prop.used) {
		char date_str[12];
		
		snprintf(date_str, 12, "%04d-%02d-%02d", crd->bday.year,
			                                 crd->bday.month,
			                                 crd->bday.day);
		vprop = addPropValue(vobj, VCBirthDateProp, date_str);
		add_CardProperty(vprop, &crd->bday.prop);
	}
	if (crd->deladdr) {
		GList *node;
		
		for (node = crd->deladdr; node; node = node->next) {
			CardDelAddr *deladdr = (CardDelAddr *) node->data;
			
			vprop = addProp(vobj, VCAdrProp);
			add_AddrType(vprop, deladdr->type);
			add_strProp(vprop, VCPostalBoxProp, deladdr->po);
			add_strProp(vprop, VCExtAddressProp, deladdr->ext);
			add_strProp(vprop, VCStreetAddressProp, deladdr->street);
			add_strProp(vprop, VCLocationProp, deladdr->locality);
			add_strProp(vprop, VCRegionProp, deladdr->region);
			add_strProp(vprop, VCPostalCodeProp, deladdr->code);
			add_strProp(vprop, VCCountryNameProp, deladdr->country);
			add_CardProperty(vprop, &deladdr->prop);
		}
	}
	if (crd->dellabel) {
		GList *node;
		
		for (node = crd->dellabel; node; node = node->next) {
			CardDelLabel *dellabel = (CardDelLabel *) node->data;
			
			vprop = add_strProp(vobj, VCDeliveryLabelProp, 
					    dellabel->data);
			add_AddrType(vprop, dellabel->type);
			add_CardProperty(vprop, &dellabel->prop);
		}
	}
	if (crd->phone) {
		GList *node;
		
		for (node = crd->phone; node; node = node->next) {
			CardPhone *phone = (CardPhone *) node->data;
			
			vprop = add_strProp(vobj, VCTelephoneProp, 
					    phone->data);
			add_PhoneType(vprop, phone->type);
			add_CardProperty(vprop, &phone->prop);
		}
	}
	if (crd->email) {
		GList *node;
		
		for (node = crd->email; node; node = node->next) {
			CardEMail *email = (CardEMail *) node->data;
			
			vprop = add_strProp(vobj, VCEmailAddressProp, 
					    email->data);
			add_EMailType(vprop, email->type);
			add_CardProperty(vprop, &email->prop);
		}
	}
	add_CardStrProperty(vobj, VCMailerProp, &crd->mailer);
	if (crd->timezn.prop.used) {
		char str[7];
		
		snprintf(str, 7, (crd->timezn.sign == -1)? "-%02d:%02d" :
			                                   "%02d:%02d", 
			 crd->timezn.hours, crd->timezn.mins);
		vprop = addPropValue(vobj, VCTimeZoneProp, str);
		add_CardProperty(vprop, &crd->timezn.prop);
	}
	if (crd->geopos.prop.used) {
		char str[15];
		
		snprintf(str, 15, "%03.02f,%03.02f", 
			 crd->geopos.lon, crd->geopos.lat);
		vprop = addPropValue(vobj, VCGeoLocationProp, str);
		add_CardProperty(vprop, &crd->geopos.prop);
	}
        add_CardStrProperty(vobj, VCTitleProp, &crd->title);
        add_CardStrProperty(vobj, VCBusinessRoleProp, &crd->role);
	if (crd->logo.prop.used) {
		vprop = addPropSizedValue(vobj, VCLogoProp, 
					  crd->logo.data, crd->logo.size);
		add_PhotoType(vprop, crd->logo.type);
		add_CardProperty(vprop, &crd->logo.prop);
	}
	if (crd->agent)
	  addVObjectProp(vobj, card_convert_to_vobject(crd->agent));
	if (crd->org.prop.used) {
		vprop = addProp(vobj, VCOrgProp);
		add_strProp(vprop, VCOrgNameProp, crd->org.name);
		add_strProp(vprop, VCOrgUnitProp, crd->org.unit1);
		add_strProp(vprop, VCOrgUnit2Prop, crd->org.unit2);
		add_strProp(vprop, VCOrgUnit3Prop, crd->org.unit3);
		add_strProp(vprop, VCOrgUnit4Prop, crd->org.unit4);
		add_CardProperty(vprop, &crd->org.prop);
	}
        add_CardStrProperty(vobj, VCCommentProp, &crd->comment);
	if (crd->sound.prop.used) {
		if (crd->sound.type != SOUND_PHONETIC)
		  vprop = addPropSizedValue(vobj, VCPronunciationProp,
					    crd->sound.data, crd->sound.size);
		else
		  vprop = addPropValue(vobj, VCPronunciationProp, 
				       crd->sound.data);
		
		add_SoundType(vprop, crd->sound.type);
		add_CardProperty(vprop, &crd->sound.prop);
	}
        add_CardStrProperty(vobj, VCURLProp, &crd->url);
        add_CardStrProperty(vobj, VCUniqueStringProp, &crd->uid);
	if (crd->key.prop.used) {
		vprop = addPropValue(vobj, VCPublicKeyProp, crd->key.data);
		add_KeyType(vprop, crd->key.type);
		add_CardProperty(vprop, &crd->key.prop);
	}
	
	return vobj;
}

void 
card_save(Card *crd, FILE *fp)
{
	writeVObject(fp, card_convert_to_vobject(crd));
}
