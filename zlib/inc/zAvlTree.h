#ifndef _AVL_TREE_H_
#define _AVL_TREE_H_

typedef int zAvlKey_t;
typedef struct AVL_NODE_TYPE{    
    int left;    
    int right;   

    int height:8;
    int bUsed:1;
    int rsvd:23;
    
    zAvlKey_t key;
} zAvlNode_t;

#ifndef TBL_SIZE
//#define TBL_SIZE(a) ( (sizeof(a)) / (sizeof(a[0])) )
#endif



typedef struct AVL_TREE_CTRL_TYPE {
  zAvlTreeNodePtr_t    _nodeprt;
  zAvlTreeAllocNode_t  _alloc;
  zAvlTreeFreeNode_t   _free;
  zAvlTreeCompareKey_t _compare;
} zAvlTreeCtrl_t;


#endif /*_AVL_TREE_H_*/
