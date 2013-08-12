//
// alloc.c
//
// Copyright (C) 2013 Robert William Fuller <hydrologiccycle@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "classc.h"

#include <string.h>
#include <stdio.h>


// TODO:  error handling for arguments...  borrow from firstcls?
//


cc_begin_meta_method(MetaAlloc, malloc)
    if (argc < 1) {
        return cc_error(by_str("too few arguments"));
    }
    return by_ptr(malloc(as_size_t(argv[0])));
cc_end_method


cc_begin_meta_method(MetaAlloc, calloc)
    if (argc < 2) {
        return cc_error(by_str("too few arguments"));
    }
    return by_ptr(calloc(as_size_t(argv[0]), as_size_t(argv[1])));
cc_end_method


cc_begin_meta_method(MetaAlloc, realloc)
    if (argc < 2) {
        return cc_error(by_str("too few arguments"));
    }
    return by_ptr(realloc(as_ptr(argv[0]), as_size_t(argv[1])));
cc_end_method


cc_begin_meta_method(MetaAlloc, free)
    if (argc < 1) {
        return cc_error(by_str("too few arguments"));
    }
    free(as_ptr(argv[0]));
    return cc_null;
cc_end_method


// TODO:  chicken and egg problem here...  Root will call MetaAlloc:realloc to allocate the method table and sort it
// will have to manually sort it and cannot use cc_add_methods b/c it expects realloc table
//  OR, add a flags field to class to indicate whether methods are statically or dynamically allocated
//
// need to make _cc_compare_names extern
//
// constructor will sort table and assign to class
//

//    qsort(cls->methods, cls->numMethods, sizeof(cc_method_name), (int (*)(const void *, const void *))
//          _cc_compare_names);

// static void prefix##Constructor(void) __attribute__((constructor)); \
// static void prefix##Constructor(void) \
// { \
//    cc_method_name prefix##Methods[] = { \
//        __VA_ARGS__ \
//    }; \
//    _cc_add_methods(&cls, sizeof(prefix##Methods) / sizeof(cc_method_name), prefix##Methods); \
// }


cc_construct_methods(MetaAlloc, MetaAlloc, 
    cc_method("malloc",     mallocMetaAlloc),
    cc_method("calloc",     callocMetaAlloc),
    cc_method("realloc",    reallocMetaAlloc),
    cc_method("free",       freeMetaAlloc),
    )

cc_class_object MetaAlloc = {
    NULL,
    &MetaRoot,
    "MetaAlloc",
    -1,
    0,
    NULL
};

cc_class_object Alloc = {
    &MetaAlloc,
    NULL,
    "Alloc",
    sizeof(cc_vars_Root),
    0,
    NULL
};
