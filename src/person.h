/*
 * $Id$
 */

#ifndef PERSON_H
#define PERSON_H

typedef enum {
    MOVEMENT_FIXED,
    MOVEMENT_WANDER,
    MOVEMENT_FOLLOW_AVATAR,
    MOVEMENT_ATTACK_AVATAR
} PersonMovementBehavior;

typedef enum {
    QTRIGGER_NONE = 0,
    QTRIGGER_JOB = 3,
    QTRIGGER_HEALTH = 4,
    QTRIGGER_KEYWORD1 = 5,
    QTRIGGER_KEYWORD2 = 6
} PersonQuestionTrigger;

typedef enum {
    QUESTION_SHOULDSAYYES,
    QUESTION_SHOULDSAYNO
} PersonQuestionType;

typedef struct _Person {
    char *name;
    char *pronoun;
    char *description;
    char *job;
    char *health;
    char *response1;
    char *response2;
    char *question;
    char *yesresp;
    char *noresp;
    char *keyword1;
    char *keyword2;
    PersonQuestionTrigger questionTrigger;
    PersonQuestionType questionType;
    int turnAwayProb;
    unsigned int tile0, tile1;
    unsigned int startx, starty;
    PersonMovementBehavior movement_behavior;
} Person;

#endif
