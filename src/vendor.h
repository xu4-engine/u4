/*
 * $Id$
 */

#ifndef VENDOR_H
#define VENDOR_H

#include "context.h"

int vendorInit(void);
char *vendorGetIntro(Conversation *cnv);
char *vendorGetBuySellResponse(Conversation *cnv, const char *response);
char *vendorGetBuyItemResponse(Conversation *cnv, const char *response);
char *vendorGetSellItemResponse(Conversation *cnv, const char *response);
char *vendorGetBuyQuantityResponse(Conversation *cnv, const char *response);
char *vendorGetSellQuantityResponse(Conversation *cnv, const char *response);
char *vendorGetPrompt(const Conversation *cnv);

#endif
