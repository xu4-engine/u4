/*
 * $Id$
 */

#include <stdio.h>
#include <string.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

void addAsText(xmlDocPtr doc, xmlNodePtr node, const char *in) {
    const char *begin, *end;
    char buffer[300];

    begin = in;
    while (*begin) {
        end = strchr(begin, '\n');
        if (!end)
            end = begin + strlen(begin);
        strncpy(buffer, begin, end - begin);
        buffer[end - begin] = '\0';

        if (buffer[0]) {
            xmlAddChild(node, xmlNewText((const xmlChar *)buffer));
        }
            
        begin = end;

        while (*begin == '\n') {
            xmlAddChild(node, xmlNewCharRef(doc, (const xmlChar *)"&#xA;"));
            begin++;
        }
    }
}

void xmlToTlk(xmlDocPtr doc, FILE *tlk) {
    xmlNodePtr root, person, node;
    const char *val;
    char *ptr;
    char tlk_buffer[288];
    enum { NAME, PRONOUN, DESC, JOB, HEALTH, RESPONSE1, RESPONSE2,
           QUESTION, YESRESP, NORESP, KEYWORD1, KEYWORD2, MAX };
    char *str[MAX];
    int i;
    
    root = xmlDocGetRootElement(doc);
    for (person = root->children; person; person = person->next) {
        if (strcmp((const char *)person->name, "person") != 0)
            continue;

        memset(tlk_buffer, 0, sizeof(tlk_buffer));
        ptr = tlk_buffer;

        val = (const char *) xmlGetProp(person, (xmlChar *) "questionTrigger");
        if (strcmp(val, "none") == 0)
            *ptr++ = 0;
        else if (strcmp(val, "job") == 0)
            *ptr++ = 3;
        else if (strcmp(val, "health") == 0)
            *ptr++ = 4;
        else if (strcmp(val, "keyword1") == 0)
            *ptr++ = 5;
        else if (strcmp(val, "keyword2") == 0)
            *ptr++ = 6;
        else {
            fprintf(stderr, "xml error");
            exit(1);
        }

        val = (const char *) xmlGetProp(person, (xmlChar *) "questionType");
        *ptr++ = (unsigned char) strtoul(val, NULL, 10);

        val = (const char *) xmlGetProp(person, (xmlChar *) "turnAwayProb");
        *ptr++ = (unsigned char) strtoul(val, NULL, 10);

        memset(&str[0], 0, sizeof(char *) * MAX);

        for (node = person->children; node; node = node->next) {
            if (!node->children)
                continue;
                
            if (strcmp((char *) node->name, "name") == 0)
                str[NAME] = (char *)xmlNodeListGetString(doc, node->children, 1);
            else if (strcmp((char *) node->name, "pronoun") == 0)
                str[PRONOUN] = (char *)xmlNodeListGetString(doc, node->children, 1);
            else if (strcmp((char *) node->name, "description") == 0)
                str[DESC] = (char *)xmlNodeListGetString(doc, node->children, 1);
            else if (strcmp((char *) node->name, "job") == 0)
                str[JOB] = (char *)xmlNodeListGetString(doc, node->children, 1);
            else if (strcmp((char *) node->name, "health") == 0)
                str[HEALTH] = (char *)xmlNodeListGetString(doc, node->children, 1);
            else if (strcmp((char *) node->name, "question") == 0)
                str[QUESTION] = (char *)xmlNodeListGetString(doc, node->children, 1);
            else if (strcmp((char *) node->name, "yesresp") == 0)
                str[YESRESP] = (char *)xmlNodeListGetString(doc, node->children, 1);
            else if (strcmp((char *) node->name, "noresp") == 0)
                str[NORESP] = (char *)xmlNodeListGetString(doc, node->children, 1);
            else if (strcmp((char *) node->name, "topic") == 0) {
                if (str[RESPONSE1] == NULL) {
                    str[RESPONSE1] = (char *)xmlNodeListGetString(doc, node->children, 1);
                    str[KEYWORD1] = (char *)xmlGetProp(node, (xmlChar *) "query");
                } else if (str[RESPONSE2] == NULL) {
                    str[RESPONSE2] = (char *)xmlNodeListGetString(doc, node->children, 1);
                    str[KEYWORD2] = (char *)xmlGetProp(node, (xmlChar *) "query");
                } else {
                    fprintf(stderr, "tlk files only allow 2 topics\n");
                    exit(1);
                }
            }
        }

        for (i = 0; i < MAX; i++) {
            if (!str[i]) {
                fprintf(stderr, "person missing tag %d\n", i); /* FIXME: give better info than the index */
                exit(1);
            }
            strcpy(ptr, str[i]);
            ptr += strlen(str[i]) + 1;
            if (ptr > (tlk_buffer + sizeof(tlk_buffer))) {
                fprintf(stderr, "tlk file overflow\n");
                exit(1);
            }
        }
        fwrite(tlk_buffer, sizeof(tlk_buffer), 1, tlk);
    }
}

xmlDocPtr tlkToXml(FILE *tlk) {
    xmlDocPtr doc;
    xmlNodePtr root, node, topic;
    int i;
    char tlk_buffer[288];
    char buf[100];
    char *response1, *response2;

    doc = xmlNewDoc((const xmlChar *)"1.0");
    doc->encoding = xmlStrdup((const xmlChar *)"ISO8859-1");
    root = xmlNewNode(NULL, (const xmlChar *)"u4tlk");
    xmlDocSetRootElement(doc, root);

    xmlNodeSetSpacePreserve(root, 1);

    for (i = 0; ; i++) {
        char *ptr;

        if (fread(tlk_buffer, 1, sizeof(tlk_buffer), tlk) != sizeof(tlk_buffer))
            break;

        node = xmlNewChild(root, NULL, (const xmlChar *)"person", NULL);

        ptr = tlk_buffer + 3;

        if (tlk_buffer[0] == 0)
            strcpy(buf, "none");
        else if (tlk_buffer[0] == 3)
            strcpy(buf, "job");
        else if (tlk_buffer[0] == 4)
            strcpy(buf, "health");
        else if (tlk_buffer[0] == 5)
            strcpy(buf, "keyword1");
        else if (tlk_buffer[0] == 6)
            strcpy(buf, "keyword2");
        else
            strcpy(buf, "???");
        xmlSetProp(node, (const xmlChar *)"questionTrigger", (const xmlChar *)buf);
        sprintf(buf, "%d", tlk_buffer[1]);
        xmlSetProp(node, (const xmlChar *)"questionType", (const xmlChar *)buf);
        sprintf(buf, "%d", (unsigned char) tlk_buffer[2]);
        xmlSetProp(node, (const xmlChar *)"turnAwayProb", (const xmlChar *)buf);

        addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"name", NULL), ptr);

        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"pronoun", NULL), ptr);

        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"description", NULL), ptr);

        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"job", NULL), ptr);

        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"health", NULL), ptr);

        ptr += strlen(ptr) + 1;
        response1 = strdup(ptr);

        ptr += strlen(ptr) + 1;
        response2 = strdup(ptr);

        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"question", NULL), ptr);

        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"yesresp", NULL), ptr);

        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"noresp", NULL), ptr);

        ptr += strlen(ptr) + 1;
        topic = xmlNewTextChild(node, NULL, (const xmlChar *)"topic", NULL);
        xmlSetProp(topic, (const xmlChar *)"query", (const xmlChar *)ptr);
        addAsText(doc, topic, response1);

        ptr += strlen(ptr) + 1;
        topic = xmlNewTextChild(node, NULL, (const xmlChar *)"topic", NULL);
        xmlSetProp(topic, (const xmlChar *)"query", (const xmlChar *)ptr);
        addAsText(doc, topic, response2);

        free(response1);
        free(response2);
    }

    return doc;
}

int main(int argc, char *argv[1]) {
    FILE *in, *out;
    xmlDocPtr doc;
    char *xml;
    int xmlSize;

    if (argc != 4) {
        fprintf(stderr, "usage: tlkconv --toxml file.tlk file.xml\n"
                        "       tlkconv --fromxml file.xml file.tlk\n");
        exit(1);
    }

    in = fopen(argv[2], "rb");
    if (!in) {
        perror(argv[2]);
        exit(1);
    }

    out = fopen(argv[3], "wb");
    if (!out) {
        perror(argv[3]);
        exit(1);
    }

    if (strcmp(argv[1], "--toxml") == 0) {
        doc = tlkToXml(in);
        xmlDocDumpFormatMemory(doc, (xmlChar **)&xml, &xmlSize, 1);
        fwrite(xml, xmlSize, 1, out);
    } else if (strcmp(argv[1], "--fromxml") == 0) {
        fseek(in, 0L, SEEK_END);
        xmlSize = ftell(in);
        fseek(in, 0L, SEEK_SET);
        xml = (char *) malloc(xmlSize);
        fread(xml, xmlSize, 1, in);
        doc = xmlParseMemory(xml, xmlSize);
        if (doc)
            xmlToTlk(doc, out);
    } else {
        fprintf(stderr, "%s: invalid option\n", argv[1]);
        exit(1);
    }

    fclose(in);
    fclose(out);

    return 0;
}
