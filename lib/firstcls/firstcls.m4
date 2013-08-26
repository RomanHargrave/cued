include(`inherit.m4')
guard_h


typedef enum _FcEmptyHow {

    FcEmptyNone = 0,
    FcEmptyFreeObject,
    FcEmptyFreePointer

} FcEmptyHow;


typedef int (*FcCompareFn)(cc_arg_t item, cc_arg_t key);

extern int FcObjCompare(cc_arg_t item, cc_arg_t key);


typedef void (*FcApplyFn)(cc_arg_t item, int argc, cc_arg_t *argv);


extern cc_arg_t FcContainerFree(cc_obj my, const char *msg, int argc, cc_arg_t *argv);


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


typedef struct _FcTreeNode {

    cc_arg_t item;
    struct _FcTreeNode *left, *right, *parent;
    unsigned color;

} FcTreeNode;

class(FcTree, Root,
`    FcTreeNode sentinel;'
`    FcTreeNode *root;'
`    FcCompareFn cmpMethod;')

class(FcTreeCursor, Root,
`    cc_vars_FcTree *tree;'
`    FcTreeNode *curr;')


class(FcString, Root,
`    char *buffer;'
`    ssize_t length;'
`    ssize_t bufferSize;')


unguard_h
