/*
** hash.c
**
** Copyright (C) 2014 Robert William Fuller <hydrologiccycle@gmail.com>
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
#include <string.h> // strcmp

#define FC_HASH_FLAG_RESIZABLE      0x00000001


typedef struct _FcHashBucket {

    cc_arg_t item;
    struct _FcHashBucket *next;
    ssize_t hash;

} FcHashBucket;


cc_begin_method(FcHash, init)
    cc_msg_super0("init");

    cc_check_argc_range(0, 3);
    my->initialSize = my->buckets = argc ?   as_ssize_t(argv[0]) : 64;
    my->cmpMethod  = (argc > 1) ? (FcCompareFn)  as_ptr(argv[1]) : FcObjCompare;
    my->hashMethod = (argc > 2) ? (FcHashFn)     as_ptr(argv[2]) : FcObjHash;

    if (my->buckets < 1) {
        return cc_error(by_str("invalid number of buckets"));
    }

    my->mask = my->buckets - 1;
    if (!(my->mask & my->buckets)) {
        my->flags = FC_HASH_FLAG_RESIZABLE;
    }

    my->table = (FcHashBucket **) as_ptr(cc_msg(Alloc, "calloc", by_size_t(sizeof(FcHashBucket *) * my->buckets)));
    if (!my->table) {
        return cc_error(by_str("out of memory allocating hash table"));
    }

    return by_obj(my);
cc_end_method


cc_begin_method(FcHash, isEmpty)
    int rc;
    rc = !my->filled;
    return by_int(rc);
cc_end_method


static inline ssize_t HashSlot(cc_vars_FcHash *my, ssize_t hash)
{
    ssize_t slot;
    if (my->flags & FC_HASH_FLAG_RESIZABLE) {
        slot = hash & my->mask;
    } else {
        slot = hash % my->buckets;
    }
    return slot;
}


static inline void HashShrinkTable(cc_vars_FcHash *my, const char *msg)
{
    FcHashBucket **newTable, **prev, *bucket;
    ssize_t newBuckets, oldBuckets, i, j;

    oldBuckets = my->buckets;
    newBuckets = oldBuckets >> 1;  // sign extension should not be a problem
    printf("resizing from %zd to %zd; filled=%zd\n", oldBuckets, newBuckets, my->filled);

    // move hash chains from upper part of table to lower
    for (i = newBuckets, j = 0;  i < oldBuckets;  ++i, ++j) {

        // chances are good some chains are empty
        if (my->table[i]) {

            // find end of chain
            prev = &my->table[j];
            for (bucket = *prev;  bucket;  bucket = bucket->next) {
                prev = &bucket->next;
            }

            // append chain
            *prev = my->table[i];
        }
    }

    my->buckets = newBuckets;
    my->mask    = newBuckets - 1;

    newTable = (FcHashBucket **) as_ptr(cc_msg(Alloc,
               "realloc", by_ptr(my->table), by_size_t(sizeof(FcHashBucket *) * newBuckets)));
    if (newTable) {
        my->table = newTable;
    }
}


static inline void HashExtendTable(cc_vars_FcHash *my, const char *msg)
{
    FcHashBucket **newTable, **prev, *bucket, *next;
    ssize_t newBuckets, oldBuckets, i, j;

    oldBuckets = my->buckets;
    newBuckets = oldBuckets << 1;
    printf("resizing from %zd to %zd; filled=%zd\n", oldBuckets, newBuckets, my->filled);

    newTable = (FcHashBucket **) as_ptr(cc_msg(Alloc,
               "realloc", by_ptr(my->table), by_size_t(sizeof(FcHashBucket *) * newBuckets)));
    if (!newTable) {
        return;
    }
    memset(&newTable[oldBuckets], 0x00, sizeof(FcHashBucket *) * oldBuckets);

    for (i = 0, j = oldBuckets;  i < oldBuckets;  ++i, ++j) {
        prev = &newTable[i];
        for (bucket = *prev;  bucket;  bucket = next) {
            next = bucket->next;
            if (bucket->hash & oldBuckets) {
                *prev = next;
                bucket->next = newTable[j];
                newTable[j]  = bucket;
            } else {
                prev = &bucket->next;
            }
        }
    }

    my->table   = newTable;
    my->buckets = newBuckets;
    my->mask    = newBuckets - 1;
}


cc_begin_method(FcHash, insert)
    FcHashBucket *bucket;
    ssize_t slot;
    int i;

    // was my->buckets << 2;  << 1 might be better than nothing...
    if ((my->flags & FC_HASH_FLAG_RESIZABLE) && (my->filled + argc > my->buckets)) {
        HashExtendTable(my, "insert");
    }

    for (i = 0;  i < argc;  ++i) {
        bucket = (FcHashBucket *) as_ptr(cc_msg(Alloc, "malloc", by_size_t(sizeof(FcHashBucket))));
        if (!bucket) {
            return cc_error(by_str("out of memory allocating hash bucket"));
        }

        bucket->item = argv[i];
        bucket->hash = (my->hashMethod)(bucket->item);
        slot = HashSlot(my, bucket->hash);

        bucket->next = my->table[slot];
        my->table[slot] = bucket;

        ++my->filled;
    }
    return by_obj(my);
cc_end_method


cc_begin_method(FcHash, find)
    cc_arg_t item = cc_null;
    cc_arg_t key;
    FcHashBucket *bucket;
    ssize_t hash;

    cc_check_argc(1);
    key = argv[0];
    hash = (my->hashMethod)(key);
    for (bucket = my->table[ HashSlot(my, hash) ];  bucket;  bucket = bucket->next) {
        if (hash == bucket->hash && !(my->cmpMethod)(bucket->item, key)) {
            item = bucket->item;
            break;
        }
    }
    return item;
cc_end_method


cc_begin_method(FcHash, findOrRemove)
    cc_arg_t item = cc_null;
    cc_arg_t key;
    FcHashBucket **prev, *bucket;
    ssize_t slot;
    int i;
    ssize_t hash;
    int find = !strcmp(msg, "findFreq") ? 1 : 0;

    for (i = 0;  i < argc;  ++i) {
        key = argv[i];
        hash = (my->hashMethod)(key);
        slot = HashSlot(my, hash);
        prev = &my->table[slot];
        for (bucket = *prev;  bucket;  bucket = bucket->next) {
            if (hash == bucket->hash && !(my->cmpMethod)(bucket->item, key)) {
                item  = bucket->item;
                *prev = bucket->next;
                if (find) {
                    bucket->next = my->table[slot];
                    my->table[slot] = bucket;
                } else {
                    cc_msg(Alloc, "free", by_ptr(bucket));
                    --my->filled;
                }
                break;
            }
            prev = &bucket->next;
        }
    }

    // sign extension should not be a problem
    if (   !find && (my->flags & FC_HASH_FLAG_RESIZABLE)
        && (my->filled < (my->buckets >> 2)) && (my->initialSize < my->buckets))
    {
        HashShrinkTable(my, msg);
    }

    return item;
cc_end_method


cc_begin_method(FcHash, apply)
    FcHashBucket *bucket;
    FcApplyFn applyFn;
    const char *applyMsg;
    ssize_t i;

    cc_check_argc_range(1, INT_MAX);
    applyFn  = is_ptr(argv[0]) ? (FcApplyFn) as_ptr(argv[0]) : NULL;
    applyMsg = is_str(argv[0]) ?             as_str(argv[0]) : NULL;

    for (i = 0;  i < my->buckets;  ++i) {
        for (bucket = my->table[i];  bucket;  bucket = bucket->next) {
            if (applyMsg) {
                _cc_send(as_obj(bucket->item), applyMsg, argc - 1, &argv[1]);
            } else {
                applyFn(        bucket->item,            argc - 1, &argv[1]);
            }
        }
    }

    return by_obj(my);
cc_end_method


cc_begin_method(FcHash, empty)
    FcHashBucket *bucket, *next, **newTable;
    ssize_t i;
    int resize;
    FcEmptyHow how;

    cc_check_argc_range(0, 2);
    how    = (argc)     ? (FcEmptyHow) as_int(argv[0]) : FcEmptyNone;
    resize = (argc > 1) ?              as_int(argv[1]) : 0;

    for (i = 0;  i < my->buckets;  ++i) {
        for (bucket = my->table[i];  bucket;  bucket = next) {
            next = bucket->next;
            switch (how) {
                case FcEmptyNone:
                    break;
                case FcEmptyFreeObject:
                    cc_msg0(as_obj(bucket->item), "free");
                    break;
                case FcEmptyFreePointer:
                    free(as_ptr(bucket->item));
                    break;
            }
            cc_msg(Alloc, "free", by_ptr(bucket));
            --my->filled;
        }
        my->table[i] = NULL;
    }

    if (my->filled) {
        return cc_error(by_str("internal error"));
    }

    if (resize) {
        newTable = (FcHashBucket **) as_ptr(cc_msg(Alloc,
                   "realloc", by_ptr(my->table), by_size_t(sizeof(FcHashBucket *) * my->initialSize)));
        if (newTable) {
            my->table   = newTable;
            my->buckets = my->initialSize;
            my->mask    = my->buckets - 1;
        }
    }

    return by_obj(my);
cc_end_method


cc_begin_method(FcHash, free)
    cc_msg0(my, "empty");
    cc_msg(Alloc, "free", by_ptr(my->table));
    return cc_msg_super0("free");
cc_end_method


cc_class_object(FcHash)

cc_class(FcHash,
    cc_method("init",               initFcHash),
    cc_method("isEmpty",            isEmptyFcHash),
    cc_method("empty",              emptyFcHash),
    cc_method("free",               freeFcHash),
    cc_method("insert",             insertFcHash),
    cc_method("find",               findFcHash),
    cc_method("findFreq",           findOrRemoveFcHash),
    cc_method("remove",             findOrRemoveFcHash),
    cc_method("apply",              applyFcHash),
    )
