
/*
 * File: memutils.h
 * Description: Describes utilities for data structure manipulation
 * Author: David Ferreira de Sousa Duque, 93698
 *
 * Timestamp: 3rd May, 17:41
 *
 * SOURCE (IAED PROJECT 2018/19):
 * https://github.com/ddavness/ist-projects/blob/master/1st%20year/IAED/IAED_2nd_proj/memutils.h
 *
 * Only the linked list implementation was imported.
*/

#ifndef MEMUTILS
#define MEMUTILS

typedef struct lnode ListNode;
typedef struct {
	ListNode* first;
	ListNode* last;
} RootNode;

struct lnode {
	ListNode* _next;
	ListNode* _prev;
	char* key;
	void* content;
};

/* Double-Linked list abstraction function prototypes */
RootNode* createLinkedList();
ListNode* findInList(RootNode*, char*);
ListNode* appendToList(RootNode*, void*);
void removeFromList(RootNode*, ListNode*);
void destroyLinkedList(RootNode*, void(void*));

/* Hashing prototypes */
int hashKey(char*);

/* Hashtable abstraction function prototypes */
RootNode** createHashTable();
ListNode* findInHashTable(RootNode**, char*);
ListNode* addToHashTable(RootNode**, char*, void*);
void removeFromHashTable(RootNode**, ListNode*);
void destroyHashTable(RootNode**, void(void*));

#endif