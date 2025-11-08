#pragma once

// C stdlib
#include <stddef.h>
#include <stdint.h>

// Doubly linked list to store the nodes according to the time order
struct DList {
    DList *prev = NULL;
    DList *next = NULL;
};

inline void dlist_init(DList *list) {
    list->prev = list->next = list;
}

inline bool dlist_empty(DList *list) {
    return list->next == list;
}

// Detach by changing the pointers of the previous and the next nodes
inline void dlist_detach(DList *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

// Insert before the target node
inline void dlist_insert_before(DList *target, DList *rookie) {
    target->prev->next = rookie;
    rookie->prev = target->prev;
    rookie->next = target;
    target->prev = rookie;
}