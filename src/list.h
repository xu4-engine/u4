/*
 * $Id$
 */

#ifndef LIST_H
#define LIST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A generic linked list data type.  By convention, an empty list is
 * represented by a NULL pointer and a non-empty list is represented
 * by a pointer to its first ListNode.  Note that most of the list
 * functions return a ListNode which in most cases should be assigned
 * to the list variable.  
 */
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

#ifdef __cplusplus
}
#endif

#endif /* LIST_H */
