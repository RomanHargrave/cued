include(`inherit.m4')
guard_h


typedef struct _FcListNode {

    cc_arg_t item;
    struct _FcListNode *next;
    struct _FcListNode *prev;

} FcListNode;

class(FcList, Root,
`    FcListNode head;')

class(FcListCursor, Root,
`    cc_vars_FcList *list;'
`    FcListNode *curr;')


typedef int (*FcCompare)(cc_arg_t item, cc_arg_t key);

extern int FcObjCompare(cc_arg_t item, cc_arg_t key);

typedef struct _FcTreeNode {

    cc_arg_t item;
    struct _FcTreeNode *left, *right, *parent;
    unsigned color;

} FcTreeNode;

class(FcTree, Root,
`    FcTreeNode sentinel;'
`    FcTreeNode *root;'
`    FcCompare cmpMethod;')

class(FcTreeCursor, Root,
`    cc_vars_FcTree *tree;'
`    FcTreeNode *curr;')


unguard_h
