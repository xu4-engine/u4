/*
 * $Id$
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/valid.h>
#include <libxml/xmlIO.h>

#include "xml.h"
#include "error.h"
#include "list.h"
#include "settings.h"
#include "u4file.h"

#if defined(_WIN32)
    #define vsnprintf _vsnprintf
#endif

void xmlAccumError(void *l, const char *fmt, ...);

extern int verbose;

int ioRegistered = 0;

void *xmlXu4FileOpen (const char *filename) {
    void *result;
    char *pathname = u4find_conf(filename);

    if (!pathname)
        return NULL;
    result = xmlFileOpen(pathname);


    if (verbose)
        printf("xml parser opened %s: %s\n", pathname, result ? "success" : "failed");

    free(pathname);

    return result;
}

void xmlRegisterIO() {
    xmlRegisterInputCallbacks(&xmlFileMatch, &xmlXu4FileOpen, xmlFileRead, xmlFileClose);
}

/**
 * Parse an XML document, and optionally validate it.  An error is
 * triggered if the parsing or validation fail.
 */
xmlDocPtr xmlParse(const char *filename) {
    xmlDocPtr doc;

    if (!ioRegistered)
        xmlRegisterIO();

    doc = xmlParseFile(filename);
    if (!doc)
        errorFatal("error parsing %s", filename);

    if (settings->validateXml && doc->intSubset) {
        ListNode *errorMessages = NULL, *node;
        char *errorMessage;
        xmlValidCtxt cvp;

        if (verbose)
            printf("validating %s\n", filename);

        cvp.userData = &errorMessages;
        cvp.error = &xmlAccumError;

        if (!xmlValidateDocument(&cvp, doc)) {
            errorMessage = strdup("");
            for (node = errorMessages; node; node = node->next) {
                errorMessage = realloc(errorMessage, strlen(errorMessage) + strlen((char *)node->data) + 1);
                strcat(errorMessage, (char *)node->data);
                free(node->data);
            }
            listDelete(errorMessages);
            errorFatal("xml parse error:\n%s", errorMessage);
        }
    }

    return doc;
}

void xmlAccumError(void *l, const char *fmt, ...) {
    ListNode **errlist = (ListNode **) l;
    char buffer[1000];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    *errlist = listAppend(*errlist, strdup(buffer));
}

/**
 * Get an XML property and convert it to a boolean value.  The value
 * should be "true" or "false", case sensitive.  If it is neither,
 * false is returned.
 */
int xmlGetPropAsBool(xmlNodePtr node, const xmlChar *name) {
    int result;
    xmlChar *prop;

    prop = xmlGetProp(node, name);
    if (!prop)
        return 0;

    if (xmlStrcmp(prop, (const xmlChar *) "true") == 0)
        result = 1;
    else if (xmlStrcmp(prop, (const xmlChar *) "true") == 0)
        result = 0;
    else
        result = 0;
    xmlFree(prop);

    return result;
}

/**
 * Get an XML property and convert it to an integer value.  Returns
 * zero if the property is not set.
 */
int xmlGetPropAsInt(xmlNodePtr node, const xmlChar *name) {
    long result;
    xmlChar *prop;

    prop = xmlGetProp(node, name);
    if (!prop)
        return 0;

    result = strtol(prop, NULL, 0);
    xmlFree(prop);

    return (int) result;
}

/**
 * Compare an XML property to another string.  The return value is as
 * strcmp.
 */
int xmlPropCmp(xmlNodePtr node, const xmlChar *name, const char *s) {
    int result;
    xmlChar *prop;
    
    prop = xmlGetProp(node, name);
    result = xmlStrcmp(prop, (const xmlChar *) s);
    xmlFree(prop);
    
    return result;
}

/**
 * Compare an XML property to another string, case insensitively.  The
 * return value is as str[case]cmp.
 */
int xmlPropCaseCmp(xmlNodePtr node, const xmlChar *name, const char *s) {
    int result;
    xmlChar *prop;
    
    prop = xmlGetProp(node, name);
    result = xmlStrcasecmp(prop, (const xmlChar *) s);
    xmlFree(prop);
    
    return result;
}
