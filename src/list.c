/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>

#include "list.h"
#include "debug.h"

/**
 * Appends a new node onto the end of a list.
 */
ListNode *listAppend(ListNode *list, void *data) {
    ListNode *head;
    ListNode *node = malloc(sizeof(ListNode));

    ASSERT(node != NULL, "out of memory error");
    node->data = data;
    node->next = NULL;

    if (!list)
        return node;
    head = list;
    while (head->next)
        head = head->next;
    head->next = node;

    return list;
}

/**
 * Prepends a new node onto the beginning of a list.
 */
ListNode *listPrepend(ListNode *list, void *data) {
    ListNode *node = malloc(sizeof(ListNode));

    ASSERT(node != NULL, "out of memory error");
    node->data = data;
    node->next = list;

    return node;
}

/**
 * Finds a particular value in a list.
 */
ListNode *listFind(ListNode *list, void *data, ListComparator compare) {
    ListNode *node;

    for (node = list; node; node = node->next) {
        if ((*compare)(node->data, data) == 0)
            return node;
    }
    return NULL;
}

/**
 * Removes a node from a list.
 */
ListNode *listRemove(ListNode *list, ListNode *node) {
    ListNode *prev, *n;

    prev = NULL;
    n = list;
    while (n && n != node) {
        prev = n;
        n = n->next;
    }

    if (!n)
        return list;

    if (prev == NULL) {
        n = n->next;
        free(node);
        return n;
    }

    prev->next = n->next;
    free(n);

    return list;
}

/**
 * Returns number of elements in list.
 */
int listLength(ListNode *list) {
    int len;

    len = 0;
    while (list) {
        list = list->next;
        len++;
    }
    return len;
}

void listDelete(ListNode *list) {
    ListNode *node, *next;

    for (node = list; node; node = next) {
        next = node->next;
        free(node);
    }
}
