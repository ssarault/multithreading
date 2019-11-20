#ifndef SSARAULT_BSTREE_H
#define SSARAULT_BSTREE_H

#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

/**
 * struct BSTree is a thread safe binary search tree.
 * The nodes in the tree are private data structures
 * and should not be accessed directly. All operations
 * on the BSTree are thread safe except BSTree_destroy.
 */

typedef struct BSTree BSTree;

typedef struct BSTree_Args {
    BSTree *tree;
    int key;
} BSTree_Args;

typedef struct BSTree_Args_Destroy {
    BSTree *tree;
    bool multi_thread;
} BSTree_Args_Destroy;

typedef struct BSTree_Args_Lookup{
    BSTree *tree;
    int key;
    bool found;
} BSTree_Args_Lookup;

typedef struct BSTree_Args_Print {
    BSTree *tree;
    void **buffer;
    bool ordered;
} BSTree_Args_Print;

/**
 * BSTree_new allocates a struct BSTree
 * and initializes it's root node to NULL. 
 * free should not be called directly on
 * the returned BSTree, free memory
 * with BSTree_destroy instead
 */
BSTree *BSTree_new();

/**
 * BSTree_destroy free's all nodes
 * in the tree and struct BSTree itself.
 * This function is NOT thread-safe, no other
 * threads except threads created internally
 * by BSTree_destroy should be accessing the BSTree
 * when destroy is called. Receives a struct
 * BSTree_Args_Destroy as a parameter, if field
 * multi_thread is set to true BSTree_Destroy may
 * create another thread to assist with
 * the deletion of the tree.
 */
void *BSTree_destroy(void *args);

/**
 * BSTree_insert and BSTree_delete take
 * struct BSTree_Args as their parameters
 * and are both thread safe.
 */
void *BSTree_insert(void *args);

/**
 * BSTree_delete removes the node associated
 * with the parameter key from the tree.
 * After being removed from the tree
 * the node is freed from memory
 */
void *BSTree_delete(void *args);

/**
 * BSTree_lookup and BSTree_lookup_threaded are
 * both thread safe. BSTree_lookup_threaded receives
 * struct BSTree_Args_Lookup as its parameters to make it
 * compatible with the pthreads api.
 */
bool BSTree_lookup(BSTree *tree, int key);
void *BSTree_lookup_threaded(void *args); 

void *BSTree_print(void *tree_);

#endif