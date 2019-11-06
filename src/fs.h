#ifndef FS_H
#define FS_H
#include "lib/bst.h"
#include "lib/locks.h"

typedef struct tecnicofs_node {
    lock* sync_lock;
    node* bstRoot;
} tecnicofs_node;

typedef struct tecnicofs {
    int numBuckets;
    tecnicofs_node* fs;
    int nextINumber;
} tecnicofs;

int obtainNewInumber(tecnicofs*);
tecnicofs new_tecnicofs(int);
void free_tecnicofs(tecnicofs);
void create(tecnicofs, char*, int);
void delete(tecnicofs, char*);
int lookup(tecnicofs, char*);
void print_tecnicofs_tree(FILE*, tecnicofs);
lock* get_lock(tecnicofs, char*);

#endif /* FS_H */
