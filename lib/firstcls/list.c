/*
** list.c
**
** Copyright (C) 1996-2008 Robert William Fuller <hydrologiccycle@gmail.com>
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "firstcls.h"

#include <stdio.h>
#include <limits.h> // INT_MAX


cc_begin_method(FcList, init)
    cc_msg_super0("init");
    my->head.next = my->head.prev = &my->head;
    return by_obj(my);
cc_end_method


cc_begin_method(FcList, isEmpty)
    int rc;
    rc = (&my->head == my->head.next) ? 1 : 0;
    return by_int(rc);
cc_end_method


static inline FcListNode *insertBefore(FcListNode *next, cc_arg_t item)
{
    FcListNode *node, *prev;

    node = (FcListNode *) malloc(sizeof(FcListNode));
    if (!node) {
        return NULL;
    }

    prev = next->prev;

    node->prev = prev;
    prev->next = node;

    node->next = next;
    next->prev = node;

    node->item = item;

    return node;
}


static inline FcListNode *insertAfter(FcListNode *prev, cc_arg_t item)
{
    FcListNode *node, *next;

    node = (FcListNode *) malloc(sizeof(FcListNode));
    if (!node) {
        return NULL;
    }

    next = prev->next;

    node->prev = prev;
    prev->next = node;

    node->next = next;
    next->prev = node;

    node->item = item;

    return node;
}


cc_begin_method(FcList, prefix)
    int i;

    // push onto list from right to left so that order will be same as given
    for (i = argc - 1;  i >= 0;  --i) {
        if (!insertBefore(my->head.next, argv[i])) {
            return cc_msg(my, "error", by_str("out of memory allocating list node"));
        }
    }

    return by_obj(my);
cc_end_method


cc_begin_method(FcList, affix)
    int i;
    for (i = 0;  i < argc;  ++i) {
        if (!insertAfter(my->head.prev, argv[i])) {
            return cc_msg(my, "error", by_str("out of memory allocating list node"));
        }
    }
    return by_obj(my);
cc_end_method


static inline void removeNode(cc_vars_FcList *my, FcListNode *node)
{
    FcListNode *next, *prev;

    next = node->next;
    prev = node->prev;

    next->prev = prev;
    prev->next = next;

    if (&my->head != node) {
        free(node);
    } else {
        // this does not have to be fatal:  could comment this line and continue
        cc_msg(my, "error", by_str("attempt to remove item from empty list"));
    }
}


static inline cc_arg_t removeAndReturn(cc_vars_FcList *my, FcListNode *node)
{
    cc_arg_t item = node->item;
    removeNode(my, node);
    return item;
}


cc_begin_method(FcList, removePrefix)
    return removeAndReturn(my, my->head.next);
cc_end_method


cc_begin_method(FcList, removeAffix)
    return removeAndReturn(my, my->head.prev);
cc_end_method


cc_begin_method(FcList, empty)
    FcListNode *curr, *next;
    FcEmptyHow how;
    FcCheckArgcRange(0, 1);
    how = argc ? (FcEmptyHow) as_int(argv[0]) : FcEmptyNone;
    for (curr = my->head.next;  curr != &my->head;  curr = next) {
        next = curr->next;
        switch (how) {
            case FcEmptyNone:
                //printf("delete %s\n", as_str(curr->item));
                break;
            case FcEmptyFreeObject:
                cc_msg0(as_obj(curr->item), "free");
                break;
            case FcEmptyFreePointer:
                free(as_ptr(curr->item));
                break;
        }
        free(curr);
    }
    return cc_msg0(my, "init");
cc_end_method


cc_begin_method(FcList, cursor)
    return cc_msg(&FcListCursor, "new", by_obj(my));
cc_end_method


cc_begin_method(FcList, apply)
    FcListNode *node;
    FcApplyFn applyFn;
    const char *applyMsg;
    FcCheckArgcRange(1, INT_MAX);
    applyFn  = is_ptr(argv[0]) ? (FcApplyFn) as_ptr(argv[0]) : NULL;
    applyMsg = is_str(argv[0]) ?             as_str(argv[0]) : NULL;
    for (node = my->head.next;  node != &my->head;  node = node->next) {
        if (applyMsg) {
            _cc_send(as_obj(node->item), applyMsg, argc - 1, &argv[1]);
        } else {
            applyFn(        node->item,            argc - 1, &argv[1]);
        }
    }
    return by_obj(my);
cc_end_method


cc_class_object(FcList)


cc_class(FcList,
    cc_method("init",           initFcList),
    cc_method("isEmpty",        isEmptyFcList),
    cc_method("prefix",         prefixFcList),
    cc_method("affix",          affixFcList),
    cc_method("removePrefix",   removePrefixFcList),
    cc_method("removeAffix",    removeAffixFcList),
    cc_method("cursor",         cursorFcList),
    cc_method("empty",          emptyFcList),
    cc_method("apply",          applyFcList),
    cc_method("free",           FcContainerFree)
    )


static inline cc_arg_t getCurrent(cc_vars_FcListCursor *my)
{
    // item is equivalent to cc_null in the list head
    return my->curr->item;
}

cc_begin_method(FcListCursor, current)
    return getCurrent(my);
cc_end_method

cc_begin_method(FcListCursor, first)
    my->curr = my->list->head.next;
    return getCurrent(my);
cc_end_method

cc_begin_method(FcListCursor, last)
    my->curr = my->list->head.prev;
    return getCurrent(my);
cc_end_method

cc_begin_method(FcListCursor, next)
    my->curr = my->curr->next;
    return getCurrent(my);
cc_end_method

cc_begin_method(FcListCursor, prev)
    my->curr = my->curr->prev;
    return getCurrent(my);
cc_end_method

cc_begin_method(FcListCursor, remove)
    FcListNode *curr = my->curr;
    my->curr = curr->next;
    return removeAndReturn(my->list, curr);
cc_end_method

cc_begin_method(FcListCursor, prefix)
    FcListNode *curr = my->curr;
    int i;
    for (i = 0;  i < argc;  ++i) {
        if (!insertBefore(curr, argv[i])) {
            return cc_msg(my, "error", by_str("out of memory allocating list node"));
        }
    }
    return by_obj(my);
cc_end_method

cc_begin_method(FcListCursor, affix)
    FcListNode *curr = my->curr;
    int i;
    for (i = 0;  i < argc;  ++i) {

        // insert by order given...
        curr = insertAfter(curr, argv[i]);
        if (!curr) {
            return cc_msg(my, "error", by_str("out of memory allocating list node"));
        }

        // ...and point to item after new item
        my->curr = curr->next;
    }
    return by_obj(my);
cc_end_method

cc_begin_method(FcListCursor, init)
    cc_msg_super0("init");
    FcCheckArgc(1);
    my->list = (cc_vars_FcList *) as_obj(argv[0]);
    my->curr = &my->list->head;
    return by_obj(my);
cc_end_method

cc_class_object(FcListCursor)


cc_class(FcListCursor,
    cc_method("init",       initFcListCursor),
    cc_method("current",    currentFcListCursor),
    cc_method("first",      firstFcListCursor),
    cc_method("last",       lastFcListCursor),
    cc_method("next",       nextFcListCursor),
    cc_method("previous",   prevFcListCursor),
    cc_method("prefix",     prefixFcListCursor),
    cc_method("affix",      affixFcListCursor),
    cc_method("remove",     removeFcListCursor),
    )
