#ifndef CARD_H
#define CARD_H

#include <time.h>
#include <glib.h>

typedef struct _group
{
	char *name;
	struct _group *parent;
} CardGroup;

enum EncodType
{
	ENC_BASE64,
	ENC_QUOTED_PRINTABLE,
	ENC_8BIT,
	ENC_7BIT
};

enum ValueType 
{
	VAL_INLINE,
	VAL_CID,
	VAL_URL
};

typedef struct
{
	CardGroup *grp;
	
	int used;
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

enum PhotoType
{
	PHOTO_GIF , PHOTO_CGM  , PHOTO_WMF , PHOTO_BMP, PHOTO_MET, PHOTO_PMB ,
	PHOTO_DIB , PHOTO_PICT , PHOTO_TIFF, PHOTO_PS , PHOTO_PDF, PHOTO_JPEG,
	PHOTO_MPEG, PHOTO_MPEG2, PHOTO_AVI , PHOTO_QTIME
};

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


enum AddrType
{
	ADDR_DOM    = 1, 
	ADDR_INTL   = 2, 
	ADDR_POSTAL = 4, 
	ADDR_PARCEL = 8, 
	ADDR_HOME   = 16, 
	ADDR_WORK   = 32
};

typedef struct
{
	CardProperty prop;
	
	int type;
	char *po;         /* Post Office Address */
	char *ext;        /* Extended Address */
	char *street;
	char *locality;
	char *region;
	char *code;
	char *country;
} CardDelAddr;            /* Delivery Address */

typedef struct 
{
	CardProperty prop;
	
	int type;
	char *data;
} CardDelLabel;


/* TELECOMMUNICATIONS ADDRESSING PROPERTIES */


enum TelephoneType
{
	PHONE_PREF  = 1,
	PHONE_WORK  = 2,
	PHONE_HOME  = 4,
	PHONE_VOICE = 8,
	PHONE_FAX   = 16,
	PHONE_MSG   = 32,
	PHONE_CELL  = 64,
	PHONE_PAGER = 128,
	PHONE_BBS   = 256,
	PHONE_MODEM = 512,
	PHONE_CAR   = 1024,
	PHONE_ISDN  = 2048,
	PHONE_VIDEO = 4096
};

typedef struct
{
	CardProperty prop;
	
	int type;
	char *data;
} CardPhone;

enum EMailType
{
	EMAIL_AOL,
	EMAIL_APPLE_LINK,
	EMAIL_ATT,
	EMAIL_CIS,
	EMAIL_EWORLD,
	EMAIL_INET,
	EMAIL_IBM,
	EMAIL_MCI,
	EMAIL_POWERSHARE,
	EMAIL_PRODIGY,
	EMAIL_TLX,
	EMAIL_X400
};

typedef struct 
{
	CardProperty prop;
	
	enum EMailType type;
	char *data;
} CardEMail;

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

enum SoundType
{
	SOUND_AIFF,
	SOUND_PCM, 
	SOUND_WAVE, 
	SOUND_PHONETIC
};

typedef struct
{
	CardProperty prop;
	
	enum SoundType type;
	unsigned int size;
	char *data;
} CardSound;

enum KeyType
{
	KEY_X509, 
	KEY_PGP
};

typedef struct
{
	CardProperty prop;
	
	enum KeyType type;
	char *data;
} CardKey;

typedef struct _Card
{
	CardStrProperty fname;
	CardName        name;
	CardPhoto       photo;
	CardBDay        bday;
	
	GList          *deladdr;
	GList          *dellabel;
	
	GList          *phone;
	GList          *email;
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
	
	void *user_data;
	int flag;
} Card;

Card *card_new(void);
void card_free(Card *crd);
GList *card_load (GList *crdlist, char *fname);
void card_save(Card *crd, FILE *fp);
VObject *card_convert_to_vobject(Card *crd);

#endif
