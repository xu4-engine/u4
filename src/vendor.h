/*
 * $Id$
 */

#ifndef VENDOR_H
#define VENDOR_H

#include "context.h"

typedef void (*InnHandlerCallback)(void);

void vendorSetInnHandlerCallback(InnHandlerCallback callback);

int vendorInit(void);
void vendorGetConversationText(Conversation *cnv, const char *inquiry, char **response);
char *vendorGetPrompt(const Conversation *cnv);
const char *vendorGetVendorQuestionChoices(const Conversation *cnv);
char *vendorGetIntro(Conversation *cnv);
char *vendorGetArmsVendorQuestionResponse(Conversation *cnv, const char *response);
char *vendorGetTavernVendorQuestionResponse(Conversation *cnv, const char *response);
char *vendorGetInnVendorQuestionResponse(Conversation *cnv, const char *response);
char *vendorGetArmsBuyItemResponse(Conversation *cnv, const char *response);
char *vendorGetReagentsBuyItemResponse(Conversation *cnv, const char *response);
char *vendorGetHealerBuyItemResponse(Conversation *cnv, const char *response);
char *vendorGetGuildBuyItemResponse(Conversation *cnv, const char *response);
char *vendorGetSellItemResponse(Conversation *cnv, const char *response);
char *vendorGetBuyQuantityResponse(Conversation *cnv, const char *response);
char *vendorGetSellQuantityResponse(Conversation *cnv, const char *response);
char *vendorGetTavernBuyPriceResponse(Conversation *cnv, const char *response);
char *vendorGetReagentsBuyPriceResponse(Conversation *cnv, const char *response);
char *vendorGetArmsConfirmationResponse(Conversation *cnv, const char *response);
char *vendorGetHealerConfirmationResponse(Conversation *cnv, const char *response);
char *vendorGetInnConfirmationResponse(Conversation *cnv, const char *response);
char *vendorGetGuildConfirmationResponse(Conversation *cnv, const char *response);
char *vendorGetStableConfirmationResponse(Conversation *cnv, const char *response);
char *vendorGetContinueQuestionResponse(Conversation *cnv, const char *answer);
char *vendorGetTavernTopicResponse(Conversation *cnv, const char *response);
char *vendorGetHealerPlayerResponse(Conversation *cnv, const char *response);

#endif
