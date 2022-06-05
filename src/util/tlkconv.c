/*
 * tlkconv.c
 */

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#define EX_USAGE     64  /* command line usage error */
#define EX_NOINPUT   66  /* cannot open input */
#define EX_CANTCREAT 73  /* can't create (user) output file */


struct Talk {
    char* name;
    char* pronoun;
    char* look;
    char* job;
    char* health;
    char* response1;
    char* response2;
    char* question;
    char* yes;
    char* no;
    char* topic1;
    char* topic2;
    char* askAfter;
    uint8_t questionHumility;
    uint8_t turnAway;
};

void talk_init(struct Talk* ts, char* tlk_buffer) {
    char *ptr = tlk_buffer + 3;

    ts->questionHumility = tlk_buffer[1];
    ts->turnAway = ((uint8_t*) tlk_buffer)[2];

#define SET(memb) \
    ts->memb = ptr; \
    ptr += strlen(ptr) + 1

    SET(name);
    SET(pronoun);
    SET(look);
    SET(job);
    SET(health);
    SET(response1);
    SET(response2);
    SET(question);
    SET(yes);
    SET(no);
    SET(topic1);
    SET(topic2);

    switch( tlk_buffer[0] )
    {
        case 3:
            ts->askAfter = ts->job; break;
        case 4:
            ts->askAfter = ts->health; break;
        case 5:
            ts->askAfter = ts->topic1; break;
        case 6:
            ts->askAfter = ts->topic2; break;
        default:
            ts->askAfter = NULL; break;
    }
}

//--------------------------------------

static char fixed[128];

/*
 * Make the first character lowercase and ensure the string ends with a period.
 */
const char* fixLook(const struct Talk* ts) {
    char* it   = fixed;
    char* last = fixed + sizeof(fixed) - 1;
    const char* src  = ts->look;

    while (it != last && *src)
        *it++ = *src++;

    fixed[0] = tolower(fixed[0]);
    if (it != last && it[-1] != '.')
        *it++ = '.';

    *it = '\0';
    return fixed;
}

//--------------------------------------

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

xmlDocPtr tlkToXml(FILE *tlk, int editLook) {
    xmlDocPtr doc;
    xmlNodePtr root, node;
    int i;
    char tlk_buffer[288];
    char buf[100];
    struct Talk ts;

    doc = xmlNewDoc((const xmlChar *)"1.0");
    // FIXME: this encoding is not found on my machine; is it universal?
    //doc->encoding = xmlStrdup((const xmlChar *)"ISO8859-1");
    root = xmlNewNode(NULL, (const xmlChar *)"dialogue");
    xmlDocSetRootElement(doc, root);

    xmlNodeSetSpacePreserve(root, 1);

    for (i = 0; ; i++) {
        xmlNodePtr job, health, kw1, kw2, target;

        if (fread(tlk_buffer, 1, sizeof(tlk_buffer), tlk) != sizeof(tlk_buffer))
            break;
        talk_init(&ts, tlk_buffer);

        node = xmlNewChild(root, NULL, (const xmlChar *)"person", NULL);

        sprintf(buf, "%d", i);
        xmlSetProp(node, (const xmlChar *)"id", (const xmlChar *)buf);

        xmlSetProp(node, (const xmlChar *)"name", (const xmlChar *)ts.name);
        xmlSetProp(node, (const xmlChar *)"pronoun", (const xmlChar *)ts.pronoun);

        sprintf(buf, "%d", ts.turnAway);
        xmlSetProp(node, (const xmlChar *)"turnAwayProb", (const xmlChar *)buf);

        addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"description", NULL), editLook ? fixLook(&ts) : ts.look);

        job = addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"topic", NULL), ts.job);
        xmlSetProp(job, (const xmlChar *)"query", (const xmlChar *)"job");

        health = addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"topic", NULL), ts.health);
        xmlSetProp(health, (const xmlChar *)"query", (const xmlChar *)"health");

        kw1 = addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"topic", NULL), ts.response1);
        xmlSetProp(kw1, (const xmlChar *)"query", (const xmlChar *)ts.topic1);

        kw2 = addAsText(doc, xmlNewTextChild(node, NULL, (const xmlChar *)"topic", NULL), ts.response2);
        xmlSetProp(kw2, (const xmlChar *)"query", (const xmlChar *)ts.topic2);

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
            q = addAsText(doc, xmlNewTextChild(target, NULL, (const xmlChar *)"question", NULL), ts.question);

            // Get the question type
            sprintf(buf, "%d", ts.questionHumility);
            xmlSetProp(q, (const xmlChar *)"type", (const xmlChar *)buf);

            addAsText(doc, xmlNewTextChild(q, NULL, (const xmlChar *)"yes", NULL), ts.yes);
            addAsText(doc, xmlNewTextChild(q, NULL, (const xmlChar *)"no", NULL), ts.no);
        }
    }

    return doc;
}

//--------------------------------------

static char bstrBuf[128];

/*
 * Boron single line string.
 */
const char* bstr(const char* src) {
    char* it   = bstrBuf;
    char* last = bstrBuf + sizeof(bstrBuf) - 1;
    int ch;

    while (it != last && *src) {
        ch = *src++;
        if (ch == '\n') {
            *it++ = '^';
            *it++ = '/';
        } else if (ch == '\t') {
            *it++ = '^';
            *it++ = '-';
        } else
            *it++ = ch;
    }

    *it = '\0';
    return bstrBuf;
}

void askBoron(FILE *out, struct Talk* ts) {
    fprintf(out, "    ask%s {%s} [\n", ts->questionHumility ? "-humility" : "",
                                       bstr(ts->question));
    fprintf(out, "      {%s}\n", bstr(ts->yes));
    fprintf(out, "      {%s}\n"
                 "    ]\n", bstr(ts->no));
}

void tlkToBoron(FILE *tlk, FILE* out, int editLook) {
    char tlk_buffer[288];
    const char* look;
    struct Talk ts;
    int num = 0;
    int i;

    for (i = 0; ; i++) {
        if (fread(tlk_buffer, 1, sizeof(tlk_buffer), tlk) != sizeof(tlk_buffer))
            break;
        talk_init(&ts, tlk_buffer);
        look = editLook ? fixLook(&ts) : ts.look;

        fprintf( out, "[\n  name: \"%s\"\t; %d\n", bstr(ts.name), num++ );
        fprintf( out, "  pronoun: \"%s\"\n", ts.pronoun );
        fprintf( out, "  look: \"%s\"\n", bstr(look) );
        if (ts.turnAway)
            fprintf( out, "  turn-away: %d\n", ts.turnAway );
        fprintf( out, "  topics: [\n" );

        fprintf( out, "    \"job\" {%s}\n", bstr(ts.job) );
        if (ts.askAfter == ts.job) askBoron(out, &ts);

        fprintf( out, "    \"health\" {%s}\n", bstr(ts.health) );
        if (ts.askAfter == ts.health) askBoron(out, &ts);

        fprintf( out, "    \"%s\" {%s}\n", ts.topic1, bstr(ts.response1) );
        if (ts.askAfter == ts.topic1) askBoron(out, &ts);

        fprintf( out, "    \"%s\" {%s}\n", ts.topic2, bstr(ts.response2) );
        if (ts.askAfter == ts.topic2) askBoron(out, &ts);

        fprintf( out, "  ]\n]" );
    }
    fprintf( out, "\n" );
}

//--------------------------------------

void usage() {
    printf("Usage: tlkconv [OPTIONS] <input-file>\n"
           "\nOptions:\n"
           "  -f <format>   Output file format (boron, tlk, xml)\n"
           "  -h            Print this help and exit\n"
           "  -l            Edit look for consistency\n"
           "  -o <file>     Output filename    (defaults to stdout)\n");
}

void missingArg(char* option) {
    printf("Missing argument for %s\n", option);
    exit(EX_USAGE);
}

int main(int argc, char *argv[1]) {
    FILE *in, *out;
    char *infile = NULL;
    char *outfile = NULL;
    int outFormat = 'x';
    int editLook = 0;
    xmlDocPtr doc;
    char *xml;
    int xmlSize;
    int i;

    for (i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
                case 'f':
                    if (++i >= argc)
                        missingArg(argv[i-1]);
                    outFormat = argv[i][0];
                    break;
                case 'h':
                    usage();
                    return 0;
                case 'l':
                    editLook = 1;
                    break;
                case 'o':
                    if (++i >= argc)
                        missingArg(argv[i-1]);
                    outfile = argv[i];
                    break;
                default:
                    printf("Invalid option %s\n", argv[i]);
                    return EX_USAGE;
            }
        } else {
            infile = argv[i];
       }
    }

    if (! infile) {
        usage();
        return EX_USAGE;
    }

    in = fopen(infile, "rb");
    if (!in) {
        perror(infile);
        return EX_NOINPUT;
    }

    if (outfile) {
        out = fopen(outfile, "wb");
        if (!out) {
            perror(outfile);
            fclose(in);
            return EX_CANTCREAT;
        }
    } else {
        out = stdout;
    }

    switch (outFormat) {
        case 'b':
            tlkToBoron(in, out, editLook);
            break;

        case 't':
            fseek(in, 0L, SEEK_END);
            xmlSize = ftell(in);
            fseek(in, 0L, SEEK_SET);
            xml = (char *) malloc(xmlSize);
            fread(xml, xmlSize, 1, in);
            doc = xmlParseMemory(xml, xmlSize);
            if (doc)
                xmlToTlk(doc, out);
            break;

        case 'x':
            doc = tlkToXml(in, editLook);
            xmlDocDumpFormatMemory(doc, (xmlChar **)&xml, &xmlSize, 1);
            fwrite(xml, xmlSize, 1, out);
            xmlFree(xml);
            break;

        default:
            printf("Invalid format '%c'\n", outFormat);
    }

    if (out != stdout)
        fclose(out);

    fclose(in);
    return 0;
}
