/*
 * $Id$
 */

#ifndef XML_H
#define XML_H

#include <libxml/xmlmemory.h>

xmlDocPtr xmlParse(const char *filename);
int xmlPropExists(xmlNodePtr node, const char *name);
char *xmlGetPropAsStr(xmlNodePtr node, const char *name);
int xmlGetPropAsBool(xmlNodePtr node, const char *name);
int xmlGetPropAsInt(xmlNodePtr node, const char *name);
int xmlPropCmp(xmlNodePtr node, const char *name, const char *s);
int xmlPropCaseCmp(xmlNodePtr node, const char *name, const char *s);

#endif /* XML_H */
