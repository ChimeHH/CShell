#ifndef _AVL_TREE_API_H_
#define _AVL_TREE_API_H_



/*
typedef struct AVL_NODE_TYPE{    
    int left;    
    int right;   

    int height:8;
    int bUsed:1;
    int rsvd:23;
    
    zAvlKey_t key;
} zAvlNode_t;
*/

typedef void* (*zAvlTreeNodePtr_t)(int p);
typedef int (*zAvlTreeAllocNode_t)(void* key, int left, int right);
typedef void (*zAvlTreeFreeNode_t)(int p);
typedef int (*zAvlTreeCompareKey_t)(void* k1, void *k2);

void* zAvlTreeInitService(zAvlTreeNodePtr_t fnNodePtr, zAvlTreeAllocNode_t fnAlloc, zAvlTreeFreeNode_t fnFree, zAvlTreeCompareKey_t fnCompare);

int  zAvlTreeHeight(void *avl, int tree);

void zAvlTreePreorderWalk(void *avl, int tree);

void zAvlTreeInorderWalk(void *avl, int tree);

void zAvlTreePostorderWalk(void *avl, int tree);

void zAvlTreeShow(void *avl, int tree, int parent, int direction);


int zAvlTreeSearch(void *avl, int x, void* key);

int zAvlTreeIterSearch(void *avl, int x, void* key);


int zAvlTreeMinimum(void *avl, int tree);

int zAvlTreeMaximum(void *avl, int tree);

int zAvlTreeInsert(void *avl, int tree, void* key);

int zAvlTreeDelete(void *avl, int tree, void* key);

void zAvlTreeDestory(void *avl, int tree);




#endif /*_AVL_TREE_API_H_*/
