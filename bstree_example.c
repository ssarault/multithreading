#include "bstree.h"

int main(int argc, char **argv)
{
    BSTree *tree = BSTree_new();
    BSTree_Args iargs = { tree, 0 };
    for (int i = 0; i < 10; i++) {
        iargs.key = i;
        BSTree_insert(&iargs);
    }
    BSTree_print(tree);
    for (int i = 0; i < 4; i++) {
        iargs.key = i;
        BSTree_delete(&iargs);
    }
    puts("after delete");
    BSTree_print(tree);
    BSTree_Args_Destroy dargs = { tree, true };
    BSTree_destroy(&dargs);
    return 0;
}