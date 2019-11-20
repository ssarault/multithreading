#include "bstree.h"

typedef struct BSTreeNode {
    pthread_mutex_t mtx;
    struct BSTreeNode *left;
    struct BSTreeNode *right;
    int key;
} BSTreeNode;


typedef struct BSTree {
    pthread_mutex_t mtx;
    BSTreeNode *root;
} BSTree;


BSTree *BSTree_new()
{
    BSTree *t = malloc(sizeof(BSTree));
    t->root = NULL;
    pthread_mutex_init(&t->mtx, NULL);
    return t;
}


static BSTreeNode *BSTreeNode_new(int key)
{
    BSTreeNode *n = malloc(sizeof(BSTreeNode));
    n->left = n->right = NULL;
    n->key = key;
    pthread_mutex_init(&n->mtx, NULL);
    return n;
}


void *BSTree_insert(void *args)
{
    BSTree *tree = ((BSTree_Args *)args)->tree;
    int key = ((BSTree_Args *)args)->key;

    pthread_mutex_lock(&tree->mtx);

    if (tree->root == NULL) {
        tree->root = BSTreeNode_new(key);
        pthread_mutex_unlock(&tree->mtx);
        return NULL;
    }

    BSTreeNode *node = tree->root;
    BSTreeNode *next;
    pthread_mutex_lock(&node->mtx);
    pthread_mutex_unlock(&tree->mtx);

    for (;;) {
        if (key == node->key)
            break; // bail, key already exists

        if (key < node->key) {
            if (node->left == NULL) {
                node->left = BSTreeNode_new(key);
                break;
            }
            next = node->left;
        } else {
            if (node->right == NULL) {
                node->right = BSTreeNode_new(key);
                break;
            }
            next = node->right;
        }

        pthread_mutex_lock(&next->mtx);
        pthread_mutex_unlock(&node->mtx);
        node = next;
    }

    pthread_mutex_unlock(&node->mtx);
    return NULL;
}


bool BSTree_lookup(BSTree *tree, int key)
{
    pthread_mutex_lock(&tree->mtx);

    if (tree->root == NULL) {
        // tree is empty, bail
        pthread_mutex_unlock(&tree->mtx);
        return false;
    }

    BSTreeNode *node = tree->root;
    BSTreeNode *next;
    pthread_mutex_lock(&node->mtx);
    pthread_mutex_unlock(&tree->mtx);

    for (;;) {

        if (key == node->key) {
            pthread_mutex_unlock(&node->mtx);
            return true;
        }

        if (key < node->key)
            next = node->left;
        else 
            next = node->right;

        if (next == NULL) {
            pthread_mutex_unlock(&node->mtx);
            return false;
        }

        pthread_mutex_lock(&next->mtx);
        pthread_mutex_unlock(&node->mtx);
        node = next;
    }
}


#define BSTREE_ARGS_EXPAND \
((BSTree_Args *)args)->tree, ((BSTree_Args *)args)->key


void *BSTree_lookup_threaded(void *args) 
{
    ((BSTree_Args_Lookup *)args)->found = BSTree_lookup(BSTREE_ARGS_EXPAND);
    return &((BSTree_Args_Lookup *)args)->found;
}


#define BSTREE_FIND_MATCH(RETURN) \
    do { \
        for(;;) { \
            if (node->key == key) \
                goto found; \
            \
            if (key < node->key) { \
                if (node->left == NULL)  \
                    break; \
                next = node->left; \
            } \
            else if (node->right == NULL) \
                break; \
            else \
                next = node->right; \
            \
            pthread_mutex_unlock(&parent->mtx); \
            pthread_mutex_lock(&next->mtx); \
            parent = node; \
            node = next; \
        } \
        \
        pthread_mutex_unlock(&parent->mtx); \
        pthread_mutex_unlock(&node->mtx); \
        RETURN; \
    found: \
        break; \
   } while(0)


#define BSTREE_FIND_MIN_RIGHT \
do { \
    BSTreeNode *next; \
    \
    for (;;) { \
        if (min_right->left == NULL) \
            break; \
                    \
        next = min_right->left; \
        pthread_mutex_lock(&next->mtx); \
        pthread_mutex_unlock(&min_right->mtx); \
        min_right = next; \
    } \
} while(0)


void *BSTree_delete(void *args)
{
    BSTree *tree = ((BSTree_Args *)args)->tree;
    int key = ((BSTree_Args *)args)->key;

    pthread_mutex_lock(&tree->mtx);

    if (tree->root == NULL) { 
        // tree is empty, bail
        pthread_mutex_unlock(&tree->mtx);
        return NULL;
    }

    BSTreeNode *parent = (BSTreeNode *)tree;
    BSTreeNode *node = tree->root;
    BSTreeNode **toReplace;
    BSTreeNode *next;

    pthread_mutex_lock(&node->mtx);

    BSTREE_FIND_MATCH(return NULL);

    if (parent == (BSTreeNode *)tree)
        toReplace = &tree->root;
    else if (key < parent->key)
        toReplace = &parent->left;
    else
        toReplace = &parent->right; 

    /*  node will be replaced with one of its' children,
        right child takes priority over left
    */

    if ((node->left == NULL) && (node->right == NULL)) // node has no children
        *toReplace = NULL;
    else if (node->right == NULL)
        *toReplace = node->left;
    else
        *toReplace = node->right;

    if (node->left != NULL) { 
        /*  node->left will become the left child of the left-most
            child of node->right, aka the child of node->right
            with the smallest key */
            
        pthread_mutex_lock(&node->right->mtx);
        BSTreeNode *min_right = node->right;
        BSTREE_FIND_MIN_RIGHT;
        min_right->left = node->left;
        pthread_mutex_unlock(&min_right->mtx);
    }

    pthread_mutex_unlock(&node->mtx);
    free(node);
    pthread_mutex_unlock(&parent->mtx);
    return NULL;
}

static void *BSTree_destroy_internal(void *node_)
{

    BSTreeNode *left, *right, *node;
    node = node_;
    left = node->left;
    right = node->right;
    free(node);

    if (left != NULL)
        BSTree_destroy_internal(left);

    if (right != NULL)
        BSTree_destroy_internal(right);
    
    return NULL;
}


void *BSTree_destroy(void *args)
{
    BSTree *tree = ((BSTree_Args_Destroy *)args)->tree;
    bool multi_thread = ((BSTree_Args_Destroy *)args)->multi_thread;
    
    if (tree == NULL)
        return NULL;

    BSTreeNode *node = tree->root;
    free(tree);

    if (node == NULL)
        return NULL;
    
    if (!multi_thread)
        return BSTree_destroy_internal(node);

    BSTreeNode *left, *right;
    left = node->left;
    right = node->right;
    free(node);

    if (left != NULL) {
        if (right != NULL) {
            pthread_attr_t tattr;
            pthread_t thread_id;
            pthread_attr_init(&tattr);
            pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
            pthread_create(&thread_id, &tattr, BSTree_destroy_internal, right);
        }
        return BSTree_destroy_internal(left);
    }

    if (right != NULL)
        return BSTree_destroy_internal(right);

    return NULL;
}

static void BSTree_print_internal(BSTreeNode *node)
{
    printf("%d\n", node->key);
    BSTreeNode *left, *right;
    left = node->left;
    right = node->right;

    if (left != NULL)
        pthread_mutex_lock(&left->mtx);

    if (right != NULL)
        pthread_mutex_lock(&right->mtx);

    pthread_mutex_unlock(&node->mtx);

    if (left != NULL)
        BSTree_print_internal(left);

    if (right != NULL)
        BSTree_print_internal(right);
}


void *BSTree_print(void *tree_)
{
    BSTree *tree = tree_;
    pthread_mutex_lock(&tree->mtx);

    if (tree->root == NULL) {
        pthread_mutex_unlock(&tree->mtx);
        puts("*empty*");
        return NULL;
    }

    BSTreeNode *node = tree->root;
    pthread_mutex_lock(&node->mtx);
    pthread_mutex_unlock(&tree->mtx);

    BSTree_print_internal(node);
    return NULL;
}


// void *BSTree_print(void *args)
// {
//     BSTree *tree = ((BSTree_Args *)args)->tree;
//     pthread_mutex_lock(&tree->mtx);

//     if (tree->root == NULL)
//     {
//         pthread_mutex_unlock(&tree->mtx);
//         puts("*empty*");
//         return;
//     }

//     BSTreeNode *node = tree->root;
//     pthread_mutex_lock(&node->mtx);
//     pthread_mutex_unlock(&tree->mtx);

//     size_t buff_sz = 64;
//     BSTreeNode **buff = malloc(sizeof(BSTreeNode *) * buff_sz);
//     size_t head, tail;
//     head = tail = 0;
//     size_t count = 1;
//     size_t depth = 1;
//     size_t begin_row;

//     for(;;) {
//         if (count == 1)
//             begin_row = tail;
//         buff[tail] = node;
//         tail = (tail + 1) % buff_sz;
//         if (tail == head) {
//             size_t old_sz = buff_sz;
//             size_t adjuster = buff_sz - 1;
//             buff_sz <<= 1;
//             buff = realloc(buff, sizeof(BSTreeNode *) * buff_sz);
//             if (buff == NULL) {
//                 printf("\n buffer overflow print terminated\n");
//                 return NULL;
//             }
//             for(size_t i = head; i < old_sz; i++) {
//                 buff[i + adjuster] = buff[i];
//             }
//             head += adjuster;
//         }

//     }
// }

#undef BSTREE_FIND_MATCH
#undef BSTREE_FIND_MIN_RIGHT
#undef BSTREE_ARGS_EXPAND