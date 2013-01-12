include(`inherit.m4')
guard_h


typedef enum _FcEmptyHow {

    FcEmptyNone = 0,
    FcEmptyFreeObject,
    FcEmptyFreePointer

} FcEmptyHow;


#define FcCheckArgc(n) (((n) == argc) ? cc_null : _FcErrorArgc(my, msg, argc, (n), (n)))

#define FcCheckArgcRange(n1, n2) ((argc >= (n1) && argc <= (n2)) ? cc_null : \
                                  _FcErrorArgc(my, msg, argc, (n1), (n2)))

extern cc_arg_t _FcErrorArgc(cc_obj my, const char *msg, int argc, int minArgc, int maxArgc);


typedef int (*FcCompare)(cc_arg_t item, cc_arg_t key);

extern int FcObjCompare(cc_arg_t item, cc_arg_t key);


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
`    FcCompare cmpMethod;')

class(FcTreeCursor, Root,
`    cc_vars_FcTree *tree;'
`    FcTreeNode *curr;')


unguard_h
