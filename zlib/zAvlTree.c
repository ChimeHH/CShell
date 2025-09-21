#include "zType_Def.h"

#include "zTraceApi.h"
#include "zAvlTreeApi.h"
#include "zAvlTree.h"


#define AVLCTRL(avl)                          ((zAvlTreeCtrl_t*)(avl))
#define _AvlTreeNodePtr(p)                    ((zAvlNode_t*)(avl->_nodeprt(p)))
#define _AvlTreeAllocNode(key, left, right)   (avl->_alloc(key, left, right))
#define _AvlTreeFreeNode(p)                   (avl->_free(p))
#define _AvlTreeCompareKey(k1, k2)            (avl->_compare(k1, k2))

#define HEIGHT(p)  ( (p==0) ? -1 : ((_AvlTreeNodePtr(p))->height) )

static int _AvlTreeHeight(zAvlTreeCtrl_t *avl, int tree)
{
  return HEIGHT(tree);
}



static int _AvlTreeMinimum(zAvlTreeCtrl_t *avl, int tree)
{
  if (_AvlTreeNodePtr(tree) == 0)
    return 0;

  while(_AvlTreeNodePtr(tree)->left != 0)
    tree = _AvlTreeNodePtr(tree)->left;
  return tree;
}
 

static int _AvlTreeMaximum(zAvlTreeCtrl_t *avl, int tree)
{
  if (_AvlTreeNodePtr(tree) == 0)
    return 0;

  while(_AvlTreeNodePtr(tree)->right != 0)
    tree = _AvlTreeNodePtr(tree)->right;
  return tree;
}

static int _AvlTreeLLR(zAvlTreeCtrl_t *avl, int k2) //left left rotation
{
  int k1;

  k1 = _AvlTreeNodePtr(k2)->left;
  _AvlTreeNodePtr(k2)->left = _AvlTreeNodePtr(k1)->right;
  _AvlTreeNodePtr(k1)->right = k2;

  _AvlTreeNodePtr(k2)->height = MAX( HEIGHT(_AvlTreeNodePtr(k2)->left), HEIGHT(_AvlTreeNodePtr(k2)->right)) + 1;
  _AvlTreeNodePtr(k1)->height = MAX( HEIGHT(_AvlTreeNodePtr(k1)->left), _AvlTreeNodePtr(k2)->height) + 1;

  return k1;
}


static int _AvlTreeRRR(zAvlTreeCtrl_t *avl, int k1) //right right rotation
{
  int k2;

  k2 = _AvlTreeNodePtr(k1)->right;
  _AvlTreeNodePtr(k1)->right = _AvlTreeNodePtr(k2)->left;
  _AvlTreeNodePtr(k2)->left = k1;

  _AvlTreeNodePtr(k1)->height = MAX( HEIGHT(_AvlTreeNodePtr(k1)->left), HEIGHT(_AvlTreeNodePtr(k1)->right)) + 1;
  _AvlTreeNodePtr(k2)->height = MAX( HEIGHT(_AvlTreeNodePtr(k2)->right), _AvlTreeNodePtr(k1)->height) + 1;

  return k2;
}


static int _AvlTreeLRR(zAvlTreeCtrl_t *avl, int k3) //left right rotation
{
  _AvlTreeNodePtr(k3)->left = _AvlTreeRRR(avl, _AvlTreeNodePtr(k3)->left);

  return _AvlTreeLLR(avl, k3);
}


static int _AvlTreeRLR(zAvlTreeCtrl_t *avl, int k1) //right left rotation
{
  _AvlTreeNodePtr(k1)->right = _AvlTreeLLR(avl, _AvlTreeNodePtr(k1)->right);

  return _AvlTreeRRR(avl, k1);
}


static int _AvlTreeDeleteNode(zAvlTreeCtrl_t *avl, int tree, int z)
{
  
  if (_AvlTreeNodePtr(tree)==0 || _AvlTreeNodePtr(z)==0)
    return 0;

  if (_AvlTreeCompareKey (&_AvlTreeNodePtr(z)->key, &_AvlTreeNodePtr(tree)->key) < 0)
  {
    _AvlTreeNodePtr(tree)->left = _AvlTreeDeleteNode(avl, _AvlTreeNodePtr(tree)->left, z);
    
    if (HEIGHT(_AvlTreeNodePtr(tree)->right) - HEIGHT(_AvlTreeNodePtr(tree)->left) == 2)
    {
      int rt =  _AvlTreeNodePtr(tree)->right;
      if (HEIGHT(_AvlTreeNodePtr(rt)->left) > HEIGHT(_AvlTreeNodePtr(rt)->right))
        tree = _AvlTreeRLR(avl, tree);
      else
        tree = _AvlTreeRRR(avl, tree);
    }
  }
  else if (_AvlTreeCompareKey(&_AvlTreeNodePtr(z)->key , &_AvlTreeNodePtr(tree)->key) > 0)
  {
    _AvlTreeNodePtr(tree)->right = _AvlTreeDeleteNode(avl, _AvlTreeNodePtr(tree)->right, z);
    
    if (HEIGHT(_AvlTreeNodePtr(tree)->left) - HEIGHT(_AvlTreeNodePtr(tree)->right) == 2)
    {
      int lt =  _AvlTreeNodePtr(tree)->left;
      if (HEIGHT(_AvlTreeNodePtr(lt)->right) > HEIGHT(_AvlTreeNodePtr(lt)->left))
        tree = _AvlTreeLRR(avl, tree);
      else
        tree = _AvlTreeLLR(avl, tree);
    }
  }
  else  
  {    
    if ((_AvlTreeNodePtr(tree)->left) && (_AvlTreeNodePtr(tree)->right))
    {
      if (HEIGHT(_AvlTreeNodePtr(tree)->left) > HEIGHT(_AvlTreeNodePtr(tree)->right))
      {         
        int max = _AvlTreeMaximum(avl, _AvlTreeNodePtr(tree)->left);
        _AvlTreeNodePtr(tree)->key = _AvlTreeNodePtr(max)->key;
        _AvlTreeNodePtr(tree)->left = _AvlTreeDeleteNode(avl, _AvlTreeNodePtr(tree)->left, max);
      }
      else
      {   
        int min = _AvlTreeMaximum(avl, _AvlTreeNodePtr(tree)->right);
        _AvlTreeNodePtr(tree)->key = _AvlTreeNodePtr(min)->key;
        _AvlTreeNodePtr(tree)->right = _AvlTreeDeleteNode(avl, _AvlTreeNodePtr(tree)->right, min);
      }
    }
    else
    {
      int tmp = tree;
      tree = _AvlTreeNodePtr(tree)->left ? _AvlTreeNodePtr(tree)->left : _AvlTreeNodePtr(tree)->right;
      _AvlTreeFreeNode(tmp);
    }
  }

  return tree;
}



static void _AvlTreePreorderWalk(zAvlTreeCtrl_t *avl, int tree)
{
  if(_AvlTreeNodePtr(tree) != 0)
  {
    zTraceP("%d ", _AvlTreeNodePtr(tree)->key);
    _AvlTreePreorderWalk(avl, _AvlTreeNodePtr(tree)->left);
    _AvlTreePreorderWalk(avl, _AvlTreeNodePtr(tree)->right);
  }
}


static void _AvlTreeInorderWalk(zAvlTreeCtrl_t *avl, int tree)
{
  if(_AvlTreeNodePtr(tree) != 0)
  {
    _AvlTreeInorderWalk(avl, _AvlTreeNodePtr(tree)->left);
    zTraceP("%d ", _AvlTreeNodePtr(tree)->key);
    _AvlTreeInorderWalk(avl, _AvlTreeNodePtr(tree)->right);
  }
}


static void _AvlTreePostorderWalk(zAvlTreeCtrl_t *avl, int tree)
{
  if(_AvlTreeNodePtr(tree) != 0)
  {
    _AvlTreePostorderWalk(avl, _AvlTreeNodePtr(tree)->left);
    _AvlTreePostorderWalk(avl, _AvlTreeNodePtr(tree)->right);
    zTraceP("%d ", _AvlTreeNodePtr(tree)->key);
  }
}


static int _AvlTreeSearch(zAvlTreeCtrl_t *avl, int x, void* key)
{
  if (_AvlTreeNodePtr(x)==0 || _AvlTreeCompareKey(&_AvlTreeNodePtr(x)->key, key)==0)
    return x;

  if (_AvlTreeCompareKey(key , &_AvlTreeNodePtr(x)->key) < 0)
    return _AvlTreeSearch(avl, _AvlTreeNodePtr(x)->left, key);
  else
    return _AvlTreeSearch(avl, _AvlTreeNodePtr(x)->right, key);
}


static int _AvlTreeIterSearch(zAvlTreeCtrl_t *avl, int x, void* key)
{
  while ((_AvlTreeNodePtr(x)!=0) && ( 0 != _AvlTreeCompareKey(&_AvlTreeNodePtr(x)->key, key)))
  {
    if (_AvlTreeCompareKey(key , &_AvlTreeNodePtr(x)->key) < 0)
      x = _AvlTreeNodePtr(x)->left;
    else
      x = _AvlTreeNodePtr(x)->right;
  }

  return x;
}



static int _AvlTreeInsert(zAvlTreeCtrl_t *avl, int tree, void* key)
{
  if (_AvlTreeNodePtr(tree) == 0) 
  {    
    tree = _AvlTreeAllocNode(key, 0, 0);
    if (_AvlTreeNodePtr(tree)==0)
    {
      zTraceP("ERROR: create avltree node failed!\n");
      return 0;
    }
  }
  else if (_AvlTreeCompareKey(key , &_AvlTreeNodePtr(tree)->key) < 0)
  {
    _AvlTreeNodePtr(tree)->left = _AvlTreeInsert(avl, _AvlTreeNodePtr(tree)->left, key);
    
    if (HEIGHT(_AvlTreeNodePtr(tree)->left) - HEIGHT(_AvlTreeNodePtr(tree)->right) == 2)
    {
      if ( _AvlTreeCompareKey(key , &_AvlTreeNodePtr(_AvlTreeNodePtr(tree)->left)->key) < 0)
        tree = _AvlTreeLLR(avl, tree);
      else
        tree = _AvlTreeLRR(avl, tree);
    }
  }
  else if (_AvlTreeCompareKey(key , &_AvlTreeNodePtr(tree)->key) > 0)
  {
    _AvlTreeNodePtr(tree)->right = _AvlTreeInsert(avl, _AvlTreeNodePtr(tree)->right, key);
    
    if (HEIGHT(_AvlTreeNodePtr(tree)->right) - HEIGHT(_AvlTreeNodePtr(tree)->left) == 2)
    {
      if (_AvlTreeCompareKey(key, &_AvlTreeNodePtr(_AvlTreeNodePtr(tree)->right)->key) > 0)
        tree = _AvlTreeRRR(avl, tree);
      else
        tree = _AvlTreeRLR(avl, tree);
    }
  }
  else //key == avltree_root(tree)->key)
  {
    zTraceError("Invalid Duplicated AvlNode_t\n");
  }

  _AvlTreeNodePtr(tree)->height = MAX( HEIGHT(_AvlTreeNodePtr(tree)->left), HEIGHT(_AvlTreeNodePtr(tree)->right)) + 1;

  return tree;
}



static int _AvlTreeDelete(zAvlTreeCtrl_t *avl, int tree, void* key)
{
  int z; 
  z = _AvlTreeSearch(avl, tree, key);
  if (_AvlTreeNodePtr(z) != 0)
    tree = _AvlTreeDeleteNode(avl, tree, z);
  return tree;
}


static void _AvlTreeDestory(zAvlTreeCtrl_t *avl, int tree)
{
  if (_AvlTreeNodePtr(tree)==0)
    return ;

  if (_AvlTreeNodePtr(tree)->left != 0)
    _AvlTreeDestory(avl, _AvlTreeNodePtr(tree)->left);
  if (_AvlTreeNodePtr(tree)->right != 0)
    _AvlTreeDestory(avl, _AvlTreeNodePtr(tree)->right);

  _AvlTreeFreeNode(tree);
}


static void _AvlTreeShow(zAvlTreeCtrl_t *avl, int tree, int parent, int direction)
{
  if(_AvlTreeNodePtr(tree) != 0)
  {
    if(direction==0)  
      zTraceP("%2d is root\n", tree);
    else        
      zTraceP("%2d is %2d's %6s child\n", tree, parent, direction==1?"right" : "left");

    _AvlTreeShow(avl, _AvlTreeNodePtr(tree)->left,  tree, -1);
    _AvlTreeShow(avl, _AvlTreeNodePtr(tree)->right, tree,  1);
  }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//below are external APIs:
void* zAvlTreeInitService(zAvlTreeNodePtr_t fnNodePtr, zAvlTreeAllocNode_t fnAlloc, zAvlTreeFreeNode_t fnFree, zAvlTreeCompareKey_t fnCompare)
{
  zAvlTreeCtrl_t* avl = (zAvlTreeCtrl_t *)malloc(sizeof(zAvlTreeCtrl_t));

  memset(avl, 0, sizeof(zAvlTreeCtrl_t));
  avl->_nodeprt = fnNodePtr;
  avl->_alloc   = fnAlloc;
  avl->_free    = fnFree;
  avl->_compare = fnCompare;

  return avl;
}

int  zAvlTreeHeight(void *avl, int tree)
{
  return _AvlTreeHeight((zAvlTreeCtrl_t *)avl, tree);
}

void zAvlTreePreorderWalk(void *avl, int tree)
{
  return _AvlTreePreorderWalk((zAvlTreeCtrl_t *)avl, tree);
}

void zAvlTreeInorderWalk(void *avl, int tree)
{
  return _AvlTreeInorderWalk((zAvlTreeCtrl_t *)avl, tree);
}

void zAvlTreePostorderWalk(void *avl, int tree)
{
  return _AvlTreePostorderWalk((zAvlTreeCtrl_t *)avl, tree);
}

void zAvlTreeShow(void *avl, int tree, int parent, int direction)
{
  return _AvlTreeShow((zAvlTreeCtrl_t *)avl, tree, parent, direction);
}


int zAvlTreeSearch(void *avl, int tree, void* key)
{
  return _AvlTreeSearch((zAvlTreeCtrl_t *)avl, tree, key);
}

int zAvlTreeIterSearch(void *avl, int tree, void* key)
{
  return _AvlTreeIterSearch((zAvlTreeCtrl_t *)avl, tree, key);
}


int zAvlTreeMinimum(void *avl, int tree)
{
  return _AvlTreeMinimum((zAvlTreeCtrl_t *)avl, tree);
}

int zAvlTreeMaximum(void *avl, int tree)
{
  return _AvlTreeMaximum((zAvlTreeCtrl_t *)avl, tree);
}

int zAvlTreeInsert(void *avl, int tree, void* key)
{ 
  return _AvlTreeInsert((zAvlTreeCtrl_t *)avl, tree, key);
}

int zAvlTreeDelete(void *avl, int tree, void* key)
{
  return _AvlTreeDelete((zAvlTreeCtrl_t *)avl, tree, key);
}

void zAvlTreeDestory(void *avl, int tree)
{
  _AvlTreeDestory((zAvlTreeCtrl_t *)avl, tree);
  free(avl);
}

#define AVLTREE_SELFTEST
#ifdef AVLTREE_SELFTEST

typedef struct {
      int a;
      int b;
    } NodeKey_t;
typedef struct NODE_TYPE{    
    int left;    
    int right;   

    int height:8;
    int bUsed:1;
    int rsvd:23;
    
    NodeKey_t key;
} Node_t;

static Node_t avltree_pool[1024] = { {0, }, };


static void* NodePtr(int p)
{
  if(p>0)  return &avltree_pool[p];

  return 0;
}


static int AllocNode(void* key, int left, int right)
{
  int p;

  for(p=1; p<TBL_SIZE(avltree_pool); p++)
  {
    Node_t *ptr = (Node_t *)NodePtr(p);
    if(ptr->bUsed) continue;
    
    ptr->key = *(NodeKey_t*)key;
    ptr->height = 0;
    ptr->bUsed = 1;
    ptr->left = left;
    ptr->right = right;
  
    return p;
  }

  return 0;
}

static void FreeNode(int p)
{
  ((Node_t*)NodePtr(p))->bUsed = 0;

  return ;
}

static int CompareKey(void* k1, void *k2)
{
  return *(int*)k1 - *(int*)k2;
}

static int arr[]= {7, 4, 10, 2, 6, 8, 12, 1, 3,}; //{3,2,1,4,5,6,7,16,15,14,13,12,11,10,8,9};


int testAvlTree()
{
  int i,ilen;
  int root=0;

  void* avl = zAvlTreeInitService(NodePtr, AllocNode, FreeNode, CompareKey);

  zTraceP("root height: %d\n", _AvlTreeHeight(avl, root));
  zTraceP("adding: ");
  ilen = TBL_SIZE(arr);
  for(i=0; i<ilen; i++)
  {
    zTraceP("%d ", arr[i]);
    
    NodeKey_t key;
    key.a = arr[i];
    key.b = i;
    root = _AvlTreeInsert(avl, root, &key);
  }

  zTraceP("\ndetail: \n");
  _AvlTreeShow(avl, root, 0, 0);

  i = 7;
  zTraceP("\ndelete root: %d", i);
  root = _AvlTreeDelete(avl, root, &i);

  zTraceP("\nheight: %d", _AvlTreeHeight(avl, root));
  
  zTraceP("\ndetail: \n");
  _AvlTreeShow(avl, root, 0, 0);

  _AvlTreeDestory(avl, root);

  return 0;
}

#endif
