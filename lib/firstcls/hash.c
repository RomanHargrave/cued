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

#define FC_HASH_FLAG_EXTENSIBLE     0x00000001


typedef struct _FcHashBucket {

    cc_arg_t item;
    struct _FcHashBucket *next;
    unsigned hash;

} FcHashBucket;


cc_begin_method(FcHash, init)
    cc_msg_super0("init");

    cc_check_argc_range(0, 3);
    my->buckets    = (argc)     ?           as_ssize_t(argv[0]) : 64;
    my->cmpMethod  = (argc > 1) ? (FcCompareFn) as_ptr(argv[1]) : FcObjCompare;
    my->hashMethod = (argc > 2) ? (FcHashFn)    as_ptr(argv[2]) : FcObjHash;

    if (my->buckets < 1) {
        return cc_error(by_str("invalid number of buckets"));
    }

    my->mask = my->buckets - 1;
    if (!(my->mask & my->buckets)) {
        my->flags = FC_HASH_FLAG_EXTENSIBLE;
        printf("hash table is power of two\n");
    }

    my->table = (FcHashBucket **) as_ptr(cc_msg(Alloc, "calloc", by_size_t(sizeof(FcHashBucket *) * my->buckets)));

    return by_obj(my);
cc_end_method


cc_begin_method(FcHash, isEmpty)
    int rc;
    rc = !my->filled;
    return by_int(rc);
cc_end_method


static inline unsigned HashSlot(cc_vars_FcHash *my, unsigned hash)
{
    unsigned slot;
    if (my->flags & FC_HASH_FLAG_EXTENSIBLE) {
        slot = hash & my->mask;
    } else {
        slot = hash % my->buckets;
    }
    return slot;
}


cc_begin_method(FcHash, insert)
    FcHashBucket *bucket;
    ssize_t slot;
    int i;
    for (i = 0;  i < argc;  ++i) {
        // TODO:  resize logic

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


// TODO:  findAndPromote (MRU behavior)


cc_begin_method(FcHash, find)
    cc_arg_t rc = cc_null;
    cc_arg_t key;
    FcHashBucket *bucket;
    unsigned hash;

    cc_check_argc(1);
    key = argv[0];
    hash = (my->hashMethod)(key);
    for (bucket = my->table[ HashSlot(my, hash) ];  bucket;  bucket = bucket->next) {
        if (hash == bucket->hash && !(my->cmpMethod)(bucket->item, key)) {
            rc = bucket->item;
            break;
        }
    }
    return rc;
cc_end_method


cc_begin_method(FcHash, remove)
    cc_arg_t rc = cc_null;
    cc_arg_t key;
    FcHashBucket **prev, *bucket;
    unsigned hash;

    cc_check_argc(1);
    key = argv[0];
    hash = (my->hashMethod)(key);
    prev = &my->table[ HashSlot(my, hash) ];
    for (bucket = *prev;  bucket;  bucket = bucket->next) {
        if (hash == bucket->hash && !(my->cmpMethod)(bucket->item, key)) {
            rc    = bucket->item;
            *prev = bucket->next;
            cc_msg(Alloc, "free", by_ptr(bucket));
            break;
        }
        prev = &bucket->next;
    }

    return rc;
cc_end_method


cc_class_object(FcHash)

cc_class(FcHash,
    cc_method("init",               initFcHash),
    cc_method("isEmpty",            isEmptyFcHash),
//    cc_method("empty",              emptyFcHash),
//    cc_method("free",               FcContainerFree),

    // should list gain an insert/remove that do a comparison?
    cc_method("insert",             insertFcHash),

    // should list gain a findEqual that does a comparison?
    cc_method("find",               findFcHash),

    cc_method("remove",             removeFcHash),
//    cc_method("apply",              applyFcHash),
    )
