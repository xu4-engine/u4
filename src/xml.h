/*
 * $Id$
 */

#ifndef XML_H
#define XML_H

#include <libxml/xmlmemory.h>

#ifdef __cplusplus
extern "C" {
#endif

xmlDocPtr xmlParse(const char *filename);
int xmlPropExists(xmlNodePtr node, const char *name);
char *xmlGetPropAsStr(xmlNodePtr node, const char *name);
int xmlGetPropAsBool(xmlNodePtr node, const char *name);
int xmlGetPropAsInt(xmlNodePtr node, const char *name);
int xmlGetPropAsEnum(xmlNodePtr node, const char *name, const char *enumValues[]);
int xmlPropCmp(xmlNodePtr node, const char *name, const char *s);
int xmlPropCaseCmp(xmlNodePtr node, const char *name, const char *s);

#ifdef __cplusplus
}
#endif

#endif /* XML_H */
