#ifndef CARD_H
#define CARD_H

#include <time.h>
#include <glib.h>
#include <stdio.h>
#include "cardtypes.h"

#define PO 0
#define EXT 1
#define STREET 2
#define CITY 3
#define REGION 4
#define CODE 5
#define COUNTRY 6

#define DELADDR_MAX 7

typedef struct _group
{
	char *name;
	struct _group *parent;
} CardGroup;

typedef struct
{
	CardGroup *grp;

	int used;
	enum PropertyType type;
	enum EncodType encod;
	enum ValueType value;
	char *charset;
	char *lang;
	
	void *user_data;
} CardProperty;

typedef struct
{
	CardProperty prop;
	
	char *str;
} CardStrProperty;

typedef struct
{
	CardProperty prop;
	
	GList *l;
} CardList;


/* IDENTIFICATION PROPERTIES */


typedef struct
{
	CardProperty prop;
	
	char *family;        /* Public */
	char *given;         /* John */
	char *additional;    /* Quinlan */
	char *prefix;        /* Mr. */
	char *suffix;        /* Esq. */
} CardName;

typedef struct 
{
	CardProperty prop;
	
	enum PhotoType type;
	unsigned int size;
	char *data;
} CardPhoto;

typedef struct
{
	CardProperty prop;
	
	int year;
	int month;
	int day;
} CardBDay;


/* DELIVERY ADDRESSING PROPERTIES */

#if 0
/* NOT USED */
typedef struct
{
	CardProperty prop;
	
	int type;
	char *data[DELADDR_MAX];
} CardDelAddr;            /* Delivery Address */

typedef struct 
{
	CardProperty prop;
	
	int type;
	char *data;
} CardDelLabel;
#else
/* new ones I'm using */
typedef struct
{
    CardProperty prop;

    gint type;
    gchar *street1;
    gchar *street2;
    gchar *city;
    gchar *state;
    gchar *country;
    gchar *zip;
} CardPostAddr;
#endif

/* TELECOMMUNICATIONS ADDRESSING PROPERTIES */


typedef struct
{
	CardProperty prop;
	
	int type;
	char *data;
} CardPhone;

/* NOT USED - We're not keeping a list of emails, just one per person */
#if 0
typedef struct 
{
	CardProperty prop;
	
	enum EMailType type;
	char *data;
} CardEMail;
#else
typedef struct 
{
	CardProperty prop;
	
	enum EMailType type;
	char *address;
} CardEMail;
#endif

typedef struct
{
	CardProperty prop;
	
	int sign;      /* 1 or -1 */
	int hours;     /* Mexico General is at -6:00 UTC */
	int mins;      /* sign -1, hours 6, mins 0 */
} CardTimeZone;

typedef struct
{
	CardProperty prop;
	
	float lon;
	float lat;
} CardGeoPos;


/* ORGANIZATIONAL PROPERTIES */


typedef struct
{
	CardProperty prop;
	
	char *name;
	char *unit1;
	char *unit2;
	char *unit3;
	char *unit4;
} CardOrg;


/* EXPLANATORY PROPERTIES */


typedef struct
{
	CardProperty prop;
	
	int utc;
	struct tm tm;
} CardRev;

typedef struct
{
	CardProperty prop;
	
	enum SoundType type;
	unsigned int size;
	char *data;
} CardSound;

typedef struct
{
	CardProperty prop;
	
	enum KeyType type;
	char *data;
} CardKey;

typedef struct _Card
{
	CardProperty prop;
	
	CardStrProperty fname;
	CardName        name;
	CardPhoto       photo;
	CardBDay        bday;

/* NOT USED
        CardList        deladdr;
	CardList        dellabel;
*/
        CardList        postal;
	
	CardList        phone;
	CardEMail       email;
	CardStrProperty mailer;
	
	CardTimeZone    timezn;
	CardGeoPos      geopos;
	
	CardStrProperty title;
	CardStrProperty role;
	CardPhoto       logo;
	struct _Card   *agent;
	CardOrg         org;
	
	CardStrProperty comment;
	CardRev         rev;
	CardSound       sound;
	CardStrProperty url;
	CardStrProperty uid;
	CardKey         key;
	
	int flag;
} Card;

Card *card_new(void);
void card_free(Card *crd);
void card_prop_free(CardProperty prop);
CardProperty empty_CardProperty(void);
GList *card_load (GList *crdlist, char *fname);
void card_save(Card *crd, FILE *fp);

char *card_bday_str(CardBDay bday);
char *card_timezn_str(CardTimeZone timezn);
char *card_geopos_str(CardGeoPos geopos);

#endif
