/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u4.h"
#include "person.h"

char *dup_and_escape(const char *s) {
    char *dup, *p;
    int len, i;

    len = strlen(s);
    for (i = 0; i < strlen(s); i++) {
        if (s[i] == '"' || s[i] == '\n')
            len++;
    }

    dup = malloc(len + 1);
    
    p = dup;
    for (i = 0; i < strlen(s); i++) {
        if (s[i] == '"') {
            *p = '\\';
            ++p;
            *p = '"';
            ++p;
        } else if (s[i] == '\n') {
            *p = '\\';
            ++p;
            *p = 'n';
            ++p;
	} else {
            *p = s[i];
            ++p;
        }
    }
    *p = '\0';

    return dup;
}

void print_portals(FILE *out, char *cityname, int *n_portals) {
    if (strcmp(cityname, "lcb_1") == 0) {
        fprintf(out, "\
extern Map lcb_2_map;

const Portal %s_portals[] = {
\t{ 3, 3, &lcb_2_map, ACTION_KLIMB },
\t{ 27, 3, &lcb_2_map, ACTION_KLIMB }
};\n
", cityname);
        *n_portals = 2;
    }
    else if (strcmp(cityname, "lcb_2") == 0) {
        fprintf(out, "\
extern Map lcb_1_map;

const Portal %s_portals[] = {
\t{ 3, 3, &lcb_1_map, ACTION_DESCEND },
\t{ 27, 3, &lcb_1_map, ACTION_DESCEND }
};\n
", cityname);
        *n_portals = 2;
    }
    else {
        *n_portals = 0;
    }
}

int main(int argc, char **argv) {
    char *cityname;
    int startx, starty;
    FILE *ult, *tlk, *out;
    char buffer[CITY_HEIGHT * CITY_WIDTH];
    int x, y, i, j;
    int n_persons;
    Person persons[CITY_MAX_PERSONS];
    int conv_idx[CITY_MAX_PERSONS];
    char tlk_buffer[288];
    int n_portals;
    char portals[100];

    if (argc != 7) {
	fprintf(stderr, "usage: %s cityname startx starty ultfile tlkfile outfile\n", argv[0]);
	exit(1);
    }

    cityname = argv[1];

    startx = strtoul(argv[2], NULL, 0);
    starty = strtoul(argv[3], NULL, 0);

    if (strcmp(argv[4], "-") == 0)
	ult = stdin;
    else
	ult = fopen(argv[4], "r");
    if (!ult) {
	perror(argv[4]);
	exit(1);
    }

    if (strcmp(argv[5], "-") == 0)
	tlk = stdin;
    else
	tlk = fopen(argv[5], "r");
    if (!tlk) {
	perror(argv[5]);
	exit(1);
    }

    if (strcmp(argv[6], "-") == 0)
	out = stdout;
    else
	out = fopen(argv[6], "w");
    if (!out) {
	perror(argv[6]);
	exit(1);
    }

    fread(buffer, 1, sizeof(buffer), ult);

    for (i = 0; i < CITY_MAX_PERSONS; i++)
	persons[i].tile0 = fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS; i++)
	persons[i].startx = fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS; i++)
	persons[i].starty = fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS; i++)
	persons[i].tile1 = fgetc(ult);

    for (i = 0; i < CITY_MAX_PERSONS * 2; i++)
        (void) fgetc(ult);      /* read redundant startx/starty */

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
	int c = fgetc(ult);
	if (c == 0)
	    persons[i].movement_behavior = MOVEMENT_FIXED;
	else if (c == 1)
	    persons[i].movement_behavior = MOVEMENT_WANDER;
	else if (c == 0x80)
	    persons[i].movement_behavior = MOVEMENT_FOLLOW_AVATAR;
	else if (c == 0xFF)
	    persons[i].movement_behavior = MOVEMENT_ATTACK_AVATAR;
	else {
	    fprintf(stderr, "bad movement behavior value %d\n", c);
	    exit(1);
	}
	  
    }

    for (i = 0; i < CITY_MAX_PERSONS; i++) {
	conv_idx[i] = fgetc(ult);
	persons[i].name = "";
	persons[i].pronoun = "";
	persons[i].description = "";
	persons[i].job = "";
	persons[i].health = "";
	persons[i].response1 = "";
	persons[i].response2 = "";
	persons[i].question = "";
	persons[i].yesresp = "";
	persons[i].noresp = "";
	persons[i].keyword1 = "";
	persons[i].keyword2 = "";
	persons[i].questionTrigger = QTRIGGER_NONE;
	persons[i].questionType = 0;
	persons[i].turnAwayProb = 0;
    }

    for (i = 0; ; i++) {
	if (fread(tlk_buffer, 1, sizeof(tlk_buffer), tlk) != sizeof(tlk_buffer))
	    break;
	for (j = 0; j < CITY_MAX_PERSONS; j++) {
	    if (conv_idx[j] == i+1) {
		char *ptr = tlk_buffer + 3;

		persons[j].questionTrigger = tlk_buffer[0];
		persons[j].questionType = tlk_buffer[1];
		persons[j].turnAwayProb = tlk_buffer[2];

		persons[j].name = dup_and_escape(ptr);
		ptr += strlen(ptr) + 1;
		persons[j].pronoun = dup_and_escape(ptr);
		ptr += strlen(ptr) + 1;
		persons[j].description = dup_and_escape(ptr);
		ptr += strlen(ptr) + 1;
		persons[j].job = dup_and_escape(ptr);
		ptr += strlen(ptr) + 1;
		persons[j].health = dup_and_escape(ptr);
		ptr += strlen(ptr) + 1;
                persons[j].response1 = dup_and_escape(ptr);
		ptr += strlen(ptr) + 1;
                persons[j].response2 = dup_and_escape(ptr);
		ptr += strlen(ptr) + 1;
                persons[j].question = dup_and_escape(ptr);
		ptr += strlen(ptr) + 1;
                persons[j].yesresp = dup_and_escape(ptr);
		ptr += strlen(ptr) + 1;
                persons[j].noresp = dup_and_escape(ptr);
		ptr += strlen(ptr) + 1;
                persons[j].keyword1 = dup_and_escape(ptr);
		ptr += strlen(ptr) + 1;
                persons[j].keyword2 = dup_and_escape(ptr);
	    }
	}
    }

    fprintf(out, "\
/* this file is generated automatically -- DO NOT EDIT!!! */
#include \"../map.h\"

const unsigned char %s_data[] = {
\t", cityname);

    for (y = 0; y < CITY_HEIGHT; y++) {
	for (x = 0; x < CITY_WIDTH; x++) {
	    fprintf(out, "%d", buffer[y * (CITY_WIDTH) + x]);
	    if (y != CITY_HEIGHT - 1 ||
		x != CITY_WIDTH - 1)
		fprintf(out, ",");
	}
	fprintf(out, "\n\t");
    }
    fprintf(out, "\n};\n\n");


    /*
    fprintf(out, "\

const unsigned char %s_data_rle[] = {
\t", cityname);

    { int last_char, run_length;
    last_char = -1;
    run_length = 0;
    for (y = 0; y < CITY_HEIGHT; y++) {
	for (x = 0; x < CITY_WIDTH; x++) {
            if (buffer[y * (CITY_WIDTH) + x] == last_char)
                run_length++;
            else {
                if (last_char != -1)
                    fprintf(out, "%d, %d, ", last_char, run_length);
                last_char = buffer[y * (CITY_WIDTH) + x];
                run_length = 1;
            }
	}
	fprintf(out, "\n\t");
    }
    if (last_char != -1)
        fprintf(out, "%d, %d, ", last_char, run_length);
    fprintf(out, "\n};\n\n");}
    */


    fprintf(out, "const Person %s_persons[] = {", cityname);

    n_persons = 0;
    for (i = 0; i < CITY_MAX_PERSONS; i++) {
	char *movement;
	switch (persons[i].movement_behavior) {
	case MOVEMENT_FIXED:
	    movement = "MOVEMENT_FIXED";
	    break;
	case MOVEMENT_WANDER:
	    movement = "MOVEMENT_WANDER";
	    break;
	case MOVEMENT_FOLLOW_AVATAR:
	    movement = "MOVEMENT_FOLLOW_AVATAR";
	    break;
	case MOVEMENT_ATTACK_AVATAR:
	    movement = "MOVEMENT_ATTACK_AVATAR";
	    break;
	}

	if (persons[i].tile0 != 0) {
	    fprintf(out, "%s
\t{\"%s\", /* name */
\t \"%s\", /* pronoun */
\t \"%s\", /* description */
\t \"%s\", /* job */
\t \"%s\", /* health */
\t \"%s\", /* response1 */
\t \"%s\", /* response2 */
\t \"%s\", /* question */
\t \"%s\", /* yesresp */
\t \"%s\", /* noresp */
\t \"%s\", /* keyword1 */
\t \"%s\", /* keyword2 */
\t %d, /* questionTrigger */
\t %d, /* questionType */
\t %d, /* turnAwayProb */
\t %d, /* tile0 */
\t %d, /* tile1 */
\t %d, /* startx */
\t %d, /* starty */
\t %s /* movement_behavior */
\t}\
", 
		    n_persons == 0 ? "" : ",",
		    persons[i].name, persons[i].pronoun, persons[i].description, 
		    persons[i].job, persons[i].health, persons[i].response1,
		    persons[i].response2, persons[i].question, persons[i].yesresp,
		    persons[i].noresp, persons[i].keyword1, persons[i].keyword2,
		    persons[i].questionTrigger, persons[i].questionType,
		    persons[i].turnAwayProb,
                    persons[i].tile0, persons[i].tile1, persons[i].startx, 
                    persons[i].starty, movement);
	    n_persons++;
	}
    }

    fprintf(out, "};\n\n");

    print_portals(out, cityname, &n_portals);
    if (n_portals)
        sprintf(portals, "%s_portals", cityname);
    else
        strcpy(portals, "0");

    fprintf(out, "\
const Map %s_map = {
\t\"%s\", /* name */
\t%d, /* width */
\t%d, /* height */
\t%d, /* startx */
\t%d, /* starty */
\t%s, /* border_behavior */
\t%d, /* n_portals */
\t%s, /* portals */
\t%d, /* n_persons */
\t%s_persons, /* persons */
\tSHOW_AVATAR, /* flags */
\t%s_data /* data */
};\n\n", cityname, cityname, CITY_WIDTH, CITY_HEIGHT, startx, starty, "BORDER_EXIT2PARENT", n_portals, portals, n_persons, cityname, cityname);

    fclose(out);

    if (ult != stdin)
	fclose(ult);
    if (tlk != stdin)
	fclose(tlk);

    return 0;
}
