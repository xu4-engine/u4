/*
 * $Id$
 */

#ifndef VENDOR_H
#define VENDOR_H

#include "context.h"

int vendorInit(void);
char *vendorGetIntro(Conversation *cnv);
char *vendorGetArmsVendorQuestionResponse(Conversation *cnv, const char *response);
char *vendorGetTavernVendorQuestionResponse(Conversation *cnv, const char *response);
char *vendorGetHealerVendorQuestionResponse(Conversation *cnv, const char *response);
char *vendorGetArmsBuyItemResponse(Conversation *cnv, const char *response);
char *vendorGetReagentsBuyItemResponse(Conversation *cnv, const char *response);
char *vendorGetHealerBuyItemResponse(Conversation *cnv, const char *response);
char *vendorGetSellItemResponse(Conversation *cnv, const char *response);
char *vendorGetBuyQuantityResponse(Conversation *cnv, const char *response);
char *vendorGetSellQuantityResponse(Conversation *cnv, const char *response);
char *vendorGetBuyPriceResponse(Conversation *cnv, const char *response);
char *vendorGetContinueQuestionResponse(Conversation *cnv, const char *answer);
char *vendorGetPrompt(const Conversation *cnv);

#endif
