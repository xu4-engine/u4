/*
 * $Id$
 */

#ifndef LIST_H
#define LIST_H

typedef struct _ListNode {
    void *data;
    struct _ListNode *next;
    struct _ListNode *prev;
} ListNode;

typedef int (*ListComparator)(void *, void *);

ListNode *listAppend(ListNode *list, void *data);
ListNode *listPrepend(ListNode *list, void *data);
ListNode *listFind(ListNode *list, void *data, ListComparator compare);
ListNode *listRemove(ListNode *list, ListNode *node);
int listLength(ListNode *list);
void listDelete(ListNode *list);

#endif /* LIST_H */
