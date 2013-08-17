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


// TODO:  error handling for arguments...  borrow from firstcls?
//
// TODO:  move error handling for malloc failure to here?
//


cc_begin_meta_method(MetaAlloc, malloc)
    if (argc < 1) {
        return cc_error(by_str("too few arguments"));
    }
    return by_ptr(malloc(as_size_t(argv[0])));
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


// this has to be sorted manually since _cc_add_methods() calls reallocMetaAlloc() and there is no guarantee
// that the constructor for MetaAllocMethods[] will run before cc_category() for another class
//
static cc_method_name MetaAllocMethods[] = {
    cc_method("free",       freeMetaAlloc),
    cc_method("malloc",     mallocMetaAlloc),
    cc_method("realloc",    reallocMetaAlloc)
};

cc_class_object MetaAlloc = {
    NULL,
    &MetaRoot,
    "MetaAlloc",
    -1,
    _CC_FLAG_STATIC_METHODS,
    SNELEMS(MetaAllocMethods),
    MetaAllocMethods
};

cc_class_object Alloc = {
    &MetaAlloc,
    NULL,
    "Alloc",
    sizeof(cc_vars_Root),
    0,
    0,
    NULL
};
