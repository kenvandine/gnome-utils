#ifndef GNOMECARD_COLEDIT_H
#define GNOMECARD_COLEDIT_H

#include "card.h"

typedef enum {
    COLTYPE_END = -1,
    COLTYPE_FULLNAME = 0,
    COLTYPE_CARDNAME,
    COLTYPE_FIRSTNAME,
    COLTYPE_MIDDLENAME,
    COLTYPE_LASTNAME,
    COLTYPE_PREFIX,
    COLTYPE_SUFFIX,
    COLTYPE_ORG,
    COLTYPE_TITLE,
    COLTYPE_EMAIL,
    COLTYPE_WEBPAGE,
    COLTYPE_HOMEPHONE,
    COLTYPE_WORKPHONE
} ColumnType;

typedef struct {
    gchar      *colname;
    ColumnType  coltype;
} ColumnHeader;


gchar *getColumnNameFromType(ColumnType type);
ColumnType getColumnTypeFromName(gchar *name);
ColumnHeader *getColumnHdrFromType(ColumnType type);
gint numColumnHeaders(GList *cols);
GList *buildColumnHeaders(ColumnType *cols);
gchar *getValFromColumnHdr(Card *crd, ColumnHeader *hdr);
GList *getAllColumnHdrs(void);

#endif
