#ifndef PAIRS_H
#define PAIRS_H

#include "../versit/vcc.h"
#include "card.h"

#define FNAME 1
#define MAILER 2
#define TITLE 3
#define ROLE 4
#define COMMENT 5
#define URL 6
#define UID 7
#define NAME 8
#define PHOTO 9
#define BDAY 10
#define DELADDR 11
#define DELLABEL 12
#define PHONE 13
#define EMAIL 21
#define TIMEZN 14
#define GEOPOS 15
#define LOGO 16
#define AGENT 17
#define ORG 18
#define REV 19
#define SOUND 20
#define KEY 22
#define VERSION 23

struct pair
{
	char *str;
	int id;
};

struct pair prop_lookup[] = {
		{ VCFullNameProp, FNAME },
		{ VCNameProp, NAME },
		{ VCPhotoProp, PHOTO },
		{ VCBirthDateProp, BDAY },
		{ VCAdrProp, DELADDR },
		{ VCDeliveryLabelProp, DELLABEL },
		{ VCTelephoneProp, PHONE },
		{ VCEmailAddressProp, EMAIL },
		{ VCMailerProp, MAILER },
		{ VCTimeZoneProp, TIMEZN },
		{ VCGeoProp, GEOPOS },
		{ VCTitleProp, TITLE },
		{ VCBusinessRoleProp, ROLE },
		{ VCLogoProp, LOGO },
		{ VCAgentProp, AGENT },
		{ VCOrgProp, ORG },
		{ VCCommentProp, COMMENT },
		{ VCLastRevisedProp, REV },
		{ VCPronunciationProp, SOUND },
		{ VCURLProp, URL },
		{ VCUniqueStringProp, UID },
		{ VCVersionProp, VERSION },
		{ VCPublicKeyProp, KEY },
		{ NULL, 0} };

struct pair addr_pairs[] = {
		{ VCDomesticProp, ADDR_DOM },
		{ VCInternationalProp, ADDR_INTL },
		{ VCPostalProp, ADDR_POSTAL },
		{ VCParcelProp, ADDR_PARCEL },
		{ VCHomeProp, ADDR_HOME },
		{ VCWorkProp, ADDR_WORK },
		{ NULL, 0} };
	  
struct pair photo_pairs[] = {
		{ VCGIFProp, PHOTO_GIF },
		{ VCCGMProp, PHOTO_CGM },
		{ VCWMFProp, PHOTO_WMF },
		{ VCBMPProp, PHOTO_BMP },
		{ VCMETProp, PHOTO_MET },
		{ VCPMBProp, PHOTO_PMB },
		{ VCDIBProp, PHOTO_DIB },
		{ VCPICTProp, PHOTO_PICT },
		{ VCTIFFProp, PHOTO_TIFF },
		{ VCPDFProp, PHOTO_PDF },
		{ VCPSProp, PHOTO_PS },
		{ VCJPEGProp, PHOTO_JPEG },
		{ VCMPEGProp, PHOTO_MPEG },
		{ VCMPEG2Prop, PHOTO_MPEG2 },
		{ VCAVIProp, PHOTO_AVI },
		{ VCQuickTimeProp, PHOTO_QTIME },
		{ NULL, 0 } };

struct pair phone_pairs[] = {
		{ VCPreferredProp, PHONE_PREF },
		{ VCWorkProp, PHONE_WORK },
		{ VCHomeProp, PHONE_HOME },
		{ VCVoiceProp, PHONE_VOICE },
		{ VCFaxProp, PHONE_FAX },
		{ VCMessageProp, PHONE_MSG },
		{ VCCellularProp, PHONE_CELL },
		{ VCPagerProp, PHONE_PAGER },
		{ VCBBSProp, PHONE_BBS },
		{ VCModemProp, PHONE_MODEM },
		{ VCCarProp, PHONE_CAR },
		{ VCISDNProp, PHONE_ISDN },
		{ VCVideoProp, PHONE_VIDEO },
		{ NULL, 0 } };

struct pair email_pairs[] = {
		{ VCAOLProp, EMAIL_AOL },
		{ VCAppleLinkProp, EMAIL_APPLE_LINK },
		{ VCATTMailProp, EMAIL_ATT },
		{ VCCISProp, EMAIL_CIS },
		{ VCEWorldProp, EMAIL_EWORLD },
		{ VCInternetProp, EMAIL_INET },
		{ VCIBMMailProp, EMAIL_IBM },
		{ VCMCIMailProp, EMAIL_MCI },
		{ VCPowerShareProp, EMAIL_POWERSHARE },
		{ VCProdigyProp, EMAIL_PRODIGY },
		{ VCTLXProp, EMAIL_TLX },
		{ VCX400Prop, EMAIL_X400 },
		{ NULL, 0 } };

struct pair sound_pairs[] = {
		{ VCAIFFProp, SOUND_AIFF },
		{ VCPCMProp, SOUND_PCM },
		{ VCWAVEProp, SOUND_WAVE },
		{ NULL, 0 } };

struct pair key_pairs[] = {
		{ VCX509Prop, KEY_X509 },
		{ VCPGPProp, KEY_PGP },
		{ NULL, 0 } };
	  

#endif
