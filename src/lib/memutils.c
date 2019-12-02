/*
 * File: memutils.c
 * Description: Implements utilities for data structure manipulation
 * Author: David Ferreira de Sousa Duque, 93698
 *
 * Timestamp: 4th May, 11:50
 * SOURCE (IAED PROJECT 2018/19):
 * https://github.com/ddavness/ist-projects/blob/master/1st%20year/IAED/IAED_2nd_proj/memutils.c
 *
 * Only the linked list implementation was imported.
*/

#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "memutils.h"

#define newGlobal(TYPE) malloc(sizeof(TYPE))

/**************************************
 * DOUBLE-LINKED LISTS IMPLEMENTATION *
 **************************************/

/*
   Creates a new, empty linked list (an empty root node).
   RootNodes are the heads of every linked list, pointing both
   to the first and last element of the list.
*/
RootNode* createLinkedList() {
    RootNode* root = newGlobal(RootNode);

    root -> first = NULL;
    root -> last = NULL;

    return root;
}

/*
   Performs a linear search on the list. Goes from the first to
   last nodes and returns the one that has the same key of the one
   passed. This search method is expensive, so it's only relevant
   on some other implementations, like open addressing in an hash
   table (which I used in this very project).
*/
ListNode* findInList(RootNode* start, char* key) {
    ListNode* current = start -> first;
    while (current) {
        if (!strcmp(current -> key, key)) {
            return current; 
        }

        current = current -> _next;
    }

    return NULL;
}

/*
   Appends a node to the end of the list, preserving the intent
   of keeping the order of the elements the same. It basically
   jumps to the last element of the list (we don't need to do a
   linear search here because the root node also has a pointer
   for the last element) and attaches the new node there.
*/
ListNode* appendToList(RootNode* list, void* content) {
    ListNode *new, *last;
    new = newGlobal(ListNode);
    last = list -> last;

    new -> key = NULL;
    new -> _prev = last;
    new -> _next = NULL;
    new -> content = content;

    if (last) {
        last -> _next = new;
    } else {
        list -> first = new;
        list -> last = new;
    }
    list -> last = new;

    return new;
}

/*
   Removes an element from the list. Since the node doesn't know
   which list it is part of (while I could technically append a
   pointer to the root node of the list, it would be expensive
   memory-wise), we also need to pass in the corresponding RootNode
   in case the first/last members are changed.
*/
void removeFromList(RootNode* start, ListNode* to_remove) {
    ListNode* prev;
    ListNode* next;

    prev = to_remove -> _prev;
    next = to_remove -> _next;

    if (prev) {
        prev -> _next = next;
    } else {
        start -> first = next;
    }
    if (next) {
        next -> _prev = prev;
    } else {
        start -> last = prev;
    }

    free(to_remove);
}

/*
   Destroys a list, freeing all nodes and respective contents. Since
   however that some contents might be more complex than simple primitive
   types (we mean pointers and pointer-based structs), that could end up
   in allocated memory being "lost" (as in "the program no longer can reach
   the pointer to it"), one can pass a pointer to a function that will handle
   the freeing of the contents of the nodes. If none is passed, then free() is
   used.
*/
void destroyLinkedList(RootNode* list, void (*free_function)(void*)) {
    ListNode* aux; ListNode* node;
    node = list -> first;
    while (node){
        if (node -> key) {
            free(node -> key);
        }
        if (free_function) {
            free_function(node -> content);
        } else {
            free(node -> content);
        }
        aux = node;
        node = node -> _next;
        free(aux);
    }

    free(list);
}
