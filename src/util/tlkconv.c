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
            xmlAddChild(node, xmlNewText(buffer));
        }
            
        begin = end;

        while (*begin == '\n') {
            xmlAddChild(node, xmlNewCharRef(doc, "&#xA;"));
            begin++;
        }
    }
}

void xmlToTlk(xmlDocPtr doc, FILE *tlk) {
    xmlNodePtr root, person, node;
    const char *val;
    char *ptr;
    char tlk_buffer[288];
    
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

        for (node = person->children; node; node = node->next) {
            if (!node->children)
                continue;
                
            val = xmlNodeListGetString(doc, node->children, 1);
            strcpy(ptr, val);
            ptr += strlen(val) + 1;
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
    xmlNodePtr root, node;
    int i;
    char tlk_buffer[288];
    char buf[100];

    doc = xmlNewDoc("1.0");
    doc->encoding = xmlStrdup("ISO8859-1");
    root = xmlNewNode(NULL, "u4tlk");
    xmlDocSetRootElement(doc, root);

    xmlNodeSetSpacePreserve(root, 1);

    for (i = 0; ; i++) {
        char *ptr;

        if (fread(tlk_buffer, 1, sizeof(tlk_buffer), tlk) != sizeof(tlk_buffer))
            break;

        node = xmlNewChild(root, NULL, "person", NULL);

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
        xmlSetProp(node, "questionTrigger", buf);
        sprintf(buf, "%d", tlk_buffer[1]);
        xmlSetProp(node, "questionType", buf);
        sprintf(buf, "%d", (unsigned char) tlk_buffer[2]);
        xmlSetProp(node, "turnAwayProb", buf);

        addAsText(doc, xmlNewTextChild(node, NULL, "name", NULL), ptr);
        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, "pronoun", NULL), ptr);
        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, "description", NULL), ptr);
        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, "job", NULL), ptr);
        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, "health", NULL), ptr);
        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, "response1", NULL), ptr);
        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, "response2", NULL), ptr);
        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, "question", NULL), ptr);
        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, "yesresp", NULL), ptr);
        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, "noresp", NULL), ptr);
        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, "keyword1", NULL), ptr);
        ptr += strlen(ptr) + 1;
        addAsText(doc, xmlNewTextChild(node, NULL, "keyword2", NULL), ptr);
    }

    return doc;
}

int main(int argc, char *argv[1]) {
    FILE *in, *out;
    xmlDocPtr doc;
    xmlChar *xml;
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
        xmlDocDumpFormatMemory(doc, &xml, &xmlSize, 1);
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
