/*
 * $Id$
 */

#ifndef VENDOR_H
#define VENDOR_H

#include "context.h"

int vendorInit(void);
char *vendorGetIntro(Conversation *cnv);
char *vendorGetBuySellResponse(Conversation *cnv, const char *response);
char *vendorGetBuyResponse(Conversation *cnv, const char *response);
char *vendorGetQuantityResponse(Conversation *cnv, const char *response);
char *vendorGetPrompt(const Conversation *cnv);

#endif
