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


cc_begin_method(FcList, init)
    cc_msg_super("init");
    my->head.next = my->head.prev = &my->head;
    return by_obj(my);
cc_end_method


cc_begin_method(FcList, isEmpty)
    if (&my->head == my->head.next) {
        return by_int(1);
    } else {
        return by_int(0);
    }
cc_end_method


static inline FcListNode *insertBefore(FcListNode *next, cc_obj item)
{
    FcListNode *node, *prev;

    node = malloc(sizeof(FcListNode));
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


static inline FcListNode *insertAfter(FcListNode *prev, cc_obj item)
{
    FcListNode *node, *next;

    node = malloc(sizeof(FcListNode));
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
        if (!insertBefore(my->head.next, as_obj(argv[i]))) {
            return cc_msg(my, "error", by_str("out of memory allocating list node"));
        }
    }

    return by_obj(my);
cc_end_method


cc_begin_method(FcList, affix)
    int i;
    for (i = 0;  i < argc;  ++i) {
        if (!insertAfter(my->head.prev, as_obj(argv[i]))) {
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
    cc_obj item = node->item;
    removeNode(my, node);
    return by_obj(item);
}


cc_begin_method(FcList, removePrefix)
    return removeAndReturn(my, my->head.next);
cc_end_method


cc_begin_method(FcList, removeAffix)
    return removeAndReturn(my, my->head.prev);
cc_end_method


cc_begin_method(FcList, cursor)
    cc_vars_FcListCursor *cursor = as_obj(cc_msg(&FcListCursor, "alloc"));
    cursor->list = my;
    cursor->curr = &my->head;
    return by_obj(cursor);
cc_end_method


cc_begin_class_object(FcList)
cc_end_class


cc_begin_class(FcList)
    cc_method("init",           initFcList),
    cc_method("isEmpty",        isEmptyFcList),
    cc_method("prefix",         prefixFcList),
    cc_method("affix",          affixFcList),
    cc_method("removePrefix",   removePrefixFcList),
    cc_method("removeAffix",    removeAffixFcList),
    cc_method("cursor",         cursorFcList),
cc_end_class


static inline cc_arg_t getCurrent(cc_vars_FcListCursor *my)
{
    // item should be NULL in the list head
    return by_obj(my->curr->item);
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
        if (!insertBefore(curr, as_obj(argv[i]))) {
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
        curr = insertAfter(curr, as_obj(argv[i]));
        if (!curr) {
            return cc_msg(my, "error", by_str("out of memory allocating list node"));
        }

        // ...and point to item after new item
        my->curr = curr->next;
    }
    return by_obj(my);
cc_end_method

cc_begin_method(FcListCursor, copy)
    cc_vars_FcListCursor *cursor = as_obj(cc_msg(&FcListCursor, "alloc"));
    cursor->list = my->list;
    cursor->curr = my->curr;
    return by_obj(cursor);
cc_end_method

cc_begin_class_object(FcListCursor)
cc_end_class


cc_begin_class(FcListCursor)
    cc_method("current",    currentFcListCursor),
    cc_method("first",      firstFcListCursor),
    cc_method("last",       lastFcListCursor),
    cc_method("next",       nextFcListCursor),
    cc_method("previous",   prevFcListCursor),
    cc_method("prefix",     prefixFcListCursor),
    cc_method("affix",      affixFcListCursor),
    cc_method("remove",     removeFcListCursor),
    cc_method("copy",       copyFcListCursor),
cc_end_class
