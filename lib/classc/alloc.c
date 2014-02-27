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

// SNELEMS is the only external dependency of classc
#include <macros.h>

#include <string.h>
#include <stdio.h>


cc_begin_meta_method(MetaAlloc, malloc)
    void *p;
    size_t n;
    char bytes[24];

    cc_check_argc_range(1, 2);
    n = as_size_t(argv[0]);
    p = malloc(n);
    if (!p && 2 == argc && as_int(argv[1])) {
        snprintf(bytes, sizeof(bytes), "%zu", n);
        return cc_error(by_str("out of memory allocating "), by_str(bytes), by_str(" bytes"));
    }

    return by_ptr(p);
cc_end_method


cc_begin_meta_method(MetaAlloc, calloc)
    void *p;
    size_t n;
    char bytes[24];

    cc_check_argc_range(1, 2);
    n = as_size_t(argv[0]);
    p = calloc(1, n);
    if (!p && 2 == argc && as_int(argv[1])) {
        snprintf(bytes, sizeof(bytes), "%zu", n);
        return cc_error(by_str("out of memory allocating "), by_str(bytes), by_str(" bytes"));
    }

    return by_ptr(p);
cc_end_method


cc_begin_meta_method(MetaAlloc, realloc)
    void *p;
    size_t n;
    char bytes[24];

    cc_check_argc_range(2, 3);
    n = as_size_t(argv[1]);
    p = realloc(as_ptr(argv[0]), n);
    if (!p && n && 3 == argc && as_int(argv[2])) {

        // TODO:  cc_error should take int, size_t, etc.
        snprintf(bytes, sizeof(bytes), "%zu", n);
        return cc_error(by_str("out of memory re-allocating "), by_str(bytes), by_str(" bytes"));
    }

    return by_ptr(p);
cc_end_method


cc_begin_meta_method(MetaAlloc, free)
    cc_check_argc(1);
    free(as_ptr(argv[0]));
    return cc_null;
cc_end_method


// methods are manually sorted here since _cc_add_methods() calls reallocMetaAlloc();
// otherwise, there would be no guarantee that the constructor for MetaAllocMethods[]
// would run before cc_category() for another class
//
static cc_method_name MetaAllocMethods[] = {
    cc_method("calloc",     callocMetaAlloc),
    cc_method("free",       freeMetaAlloc),
    cc_method("malloc",     mallocMetaAlloc),
    cc_method("realloc",    reallocMetaAlloc)
};

cc_class_object MetaAllocObj = {
    NULL,
    &MetaRootObj,
    NULL,
    NULL,
    "MetaAlloc",
    -1,
    _CC_FLAG_STATIC_METHODS,
    SNELEMS(MetaAllocMethods),
    MetaAllocMethods
    };


#define cc_vars_Alloc cc_vars_Root
_cc_class_no_methods(Alloc, &RootObj)
