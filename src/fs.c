#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fs.h"
#include "lib/color.h"
#include "lib/bst.h"
#include "lib/hash.h"
#include "lib/locks.h"

tecnicofs new_tecnicofs(int buckets){
    tecnicofs root;
    root.numBuckets = buckets;
    root.fs = malloc(sizeof(tecnicofs_node) * buckets);

    if (!root.fs) {
        fprintf(stderr, red_bold("Failed to allocate TecnicoFS!"));
        perror("\nError");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < buckets; i++) {
        tecnicofs_node* bucket = root.fs + i;
        bucket -> bstRoot = NULL;
        bucket -> sync_lock = malloc(sizeof(lock));
        INIT_LOCK(bucket -> sync_lock);
    }

    return root;
}

void free_tecnicofs(tecnicofs root){
    tecnicofs_node* fs = root.fs;
    for (int i = 0; i < root.numBuckets; i++) {
        tecnicofs_node* fsnode = fs + i;
        DESTROY_LOCK(fsnode -> sync_lock);
        free(fsnode -> sync_lock);
        free_tree(fsnode -> bstRoot);
    }
    free(fs);
}

void create(tecnicofs fs, char *name, int inumber){
    tecnicofs_node* fsnode = fs.fs + hash(name, fs.numBuckets);
    fsnode -> bstRoot = insert(fsnode -> bstRoot, name, inumber);
}

void delete(tecnicofs fs, char *name){
    tecnicofs_node* fsnode = fs.fs + hash(name, fs.numBuckets);
    fsnode -> bstRoot = remove_item(fsnode -> bstRoot, name);
}

int lookup(tecnicofs fs, char *name){
    tecnicofs_node* fsnode = fs.fs + hash(name, fs.numBuckets);

    node* searchNode = search(fsnode -> bstRoot, name);
    if (searchNode) {
        return searchNode -> inumber;
    }
    return -1;
}

lock* get_lock(tecnicofs fs, char *name){
    tecnicofs_node* fsnode = fs.fs + hash(name, fs.numBuckets);
    return fsnode -> sync_lock;
}

void print_tecnicofs_tree(FILE* fp, tecnicofs fs){
    for (int i = 0; i < fs.numBuckets; i++) {
        tecnicofs_node* fsnode = fs.fs + i;
        print_tree(fp, fsnode -> bstRoot);
    }
}
