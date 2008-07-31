include(`inherit.m4')
guard_h

typedef struct _FcListNode {

    struct _FcListNode *next;
    struct _FcListNode *prev;
    cc_obj item;

} FcListNode;

class(FcList, Root,
`    FcListNode head;')

class(FcListCursor, Root,
`    cc_vars_FcList *list;'
`    FcListNode *curr;')

unguard_h
