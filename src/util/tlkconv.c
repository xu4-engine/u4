/*
 * $Id$
 */

#include <stdio.h>
#include <string.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

xmlNodePtr addAsText(xmlDocPtr doc, xmlNodePtr node, const char *in) {
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

    return node;
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
        int kw = KEYWORD1;

        if (strcmp((const char *)person->name, "person") != 0)
            continue;

        memset(tlk_buffer, 0, sizeof(tlk_buffer));
        memset(&str[0], 0, sizeof(char *) * MAX);

        str[NAME] = (char *) xmlGetProp(person, (xmlChar *) "name");
        str[PRONOUN] = (char *) xmlGetProp(person, (xmlChar *) "pronoun");

        val = (const char *) xmlGetProp(person, (xmlChar *) "turnAwayProb");
        tlk_buffer[2] = (unsigned char) strtoul(val, NULL, 10);

        for (node = person->children; node; node = node->next) {
            xmlNodePtr child;

            if (!node->children)
                continue;

            if (strcmp((char *) node->name, "description") == 0)
                str[DESC] = (char *)xmlNodeListGetString(doc, node->children, 1);
            else if (strcmp((char *) node->name, "topic") == 0) {
                char *query = (char *) xmlGetProp(node, (xmlChar *) "query");
                char trigger = 0;

                if (strcasecmp(query, "job") == 0) {
                    str[JOB] = (char *)xmlNodeListGetString(doc, node->children, 1);
                    trigger = 3;
                }
                else if (strcasecmp(query, "health") == 0) {
                    str[HEALTH] = (char *)xmlNodeListGetString(doc, node->children, 1);
                    trigger = 4;
                }
                else {
                    if (kw > KEYWORD2) {
                        fprintf(stderr, ".tlk files only allow 2 topics other than 'job' and 'health'\n");
                        fprintf(stderr, "If you need more than 2 keywords, then it is recommended you\n");
                        fprintf(stderr, "keep your dialogue in .xml\n");
                        exit(1);
                    }
                    str[kw - (KEYWORD1 - RESPONSE1)] = (char *)xmlNodeListGetString(doc, node->children, 1);
                    str[kw] = query;
                    trigger = 6 - (KEYWORD2 - kw);
                    kw++;
                }

                // Check for questions
                for (child = node->children; child; child = child->next) {
                    xmlNodePtr qchild;
                    if (strcasecmp((const char *)child->name, "question") != 0)
                        continue;

                    tlk_buffer[0] = trigger;
                    val = (const char *) xmlGetProp(child, (xmlChar *) "type");
                    tlk_buffer[1] = (unsigned char) strtoul(val, NULL, 10);

                    for (qchild = child->children; qchild; qchild = qchild->next) {
                        if (xmlNodeIsText(qchild))
                            str[QUESTION] = (char *)xmlNodeGetContent(qchild);
                        else if (strcasecmp((const char *)qchild->name, "yes") == 0)
                            str[YESRESP] = (char *)xmlNodeListGetString(doc, qchild->children, 1);
                        else if (strcasecmp((const char *)qchild->name, "no") == 0)
                            str[NORESP] = (char *)xmlNodeListGetString(doc, qchild->children, 1);
                    }
                }
            }
            else {
                fprintf(stderr, "An unknown node %s was found in the .xml file", (char *)node->name);
                exit(1);
            }
        }

        // Fill these with dummy info so it doesn't break anything.
        if (!str[QUESTION]) str[QUESTION] = strdup("\0");
        if (!str[YESRESP]) str[YESRESP] = strdup("\0");
        if (!str[NORESP]) str[NORESP] = strdup("\0");

        ptr = &tlk_buffer[3];
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

            if (*str[i] == '\0')
                free(str[i]);
            else xmlFree(str[i]);
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
    char *response1, *response2, *question, *yes, *no;

    doc = xmlNewDoc((const xmlChar *)"1.0");
    // FIXME: this encoding is not found on my machine; is it universal?
    //doc->encoding = xmlStrdup((const xmlChar *)"ISO8859-1");
    root = xmlNewNode(NULL, (const xmlChar *)"dialogue");
    xmlDocSetRootElement(doc, root);

    xmlNodeSetSpacePreserve(root, 1);

    for (i = 0; ; i++) {
        char *ptr;
        xmlNodePtr job, health, kw1, kw2, target;

        if (fread(tlk_buffer, 1, sizeof(tlk_buffer), tlk) != sizeof(tlk_buffer))
            break;

        node = xmlNewChild(root, NULL, (const xmlChar *)"person", NULL);

        ptr = tlk_buffer + 3;

        sprintf(buf, "%d", i);
        xmlSetProp(node, (const xmlChar *)"id", (const xmlChar *)buf);

        xmlSetProp(node, (const xmlChar *)"name", (const xmlChar *)ptr);
        ptr += strlen(ptr) + 1;

        xmlSetProp(node, (const xmlChar *)"pronoun", (const xmlChar *)ptr);
        ptr += strlen(ptr) + 1;

        sprintf(buf, "%d", (unsigned char) tlk_buffer[2]);
        xmlSetProp(node, (const xmlChar *)"turnAwayProb", (const xmlChar *)buf);

        addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"description", NULL), ptr);
        ptr += strlen(ptr) + 1;

        job = addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"topic", NULL), ptr);
        xmlSetProp(job, (const xmlChar *)"query", (const xmlChar *)"job");
        ptr += strlen(ptr) + 1;

        health = addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"topic", NULL), ptr);
        xmlSetProp(health, (const xmlChar *)"query", (const xmlChar *)"health");
        ptr += strlen(ptr) + 1;

        response1 = strdup(ptr);
        ptr += strlen(ptr) + 1;

        response2 = strdup(ptr);
        ptr += strlen(ptr) + 1;

        question = strdup(ptr);
        ptr += strlen(ptr) + 1;

        yes = strdup(ptr);
        ptr += strlen(ptr) + 1;

        no = strdup(ptr);
        ptr += strlen(ptr) + 1;

        kw1 = addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"topic", NULL), response1);
        xmlSetProp(kw1, (const xmlChar *)"query", (const xmlChar *)ptr);
        ptr += strlen(ptr) + 1;

        kw2 = addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"topic", NULL), response2);
        xmlSetProp(kw2, (const xmlChar *)"query", (const xmlChar *)ptr);
        ptr += strlen(ptr) + 1;

        switch(tlk_buffer[0]) {
        case 3: target = job; break;
        case 4: target = health; break;
        case 5: target = kw1; break;
        case 6: target = kw2; break;
        case 0:
        default:
            target = NULL;
            break;
        }

        if (target) {
            xmlNodePtr q;
            q = addAsText(doc, xmlNewTextChild(target, NULL, (const xmlChar *)"question", NULL), question);

            // Get the question type
            sprintf(buf, "%d", tlk_buffer[1]);
            xmlSetProp(q, (const xmlChar *)"type", (const xmlChar *)buf);

            addAsText(doc, xmlNewTextChild(q, NULL, (const xmlChar *)"yes", NULL), yes);
            addAsText(doc, xmlNewTextChild(q, NULL, (const xmlChar *)"no", NULL), no);
        }

        free(response1);
        free(response2);
        free(question);
        free(yes);
        free(no);
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
        xmlFree(xml);
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
