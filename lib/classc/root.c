//
// root.c
//
// Copyright (C) 2008-2012 Robert William Fuller <hydrologiccycle@gmail.com>
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


cc_begin_meta_method(MetaRoot, new)
    cc_obj obj = as_obj(cc_msg(my, "alloc"));
    if (obj) {
        return _cc_send(obj, "init", argc, argv);
    } else {
        return cc_msg(my, "error", by_str("alloc method did not return instance for class \""),
                      by_str(my->name), by_str("\""));
    }
cc_end_method


cc_begin_meta_method(MetaRoot, alloc)
    cc_vars_Root *obj = (cc_vars_Root *) malloc(my->size);
    if (obj) {
        obj->isa = my;
        return by_obj(obj);
    } else {
        return cc_msg(my, "error", by_str("out of memory allocating object of class \""),
                      by_str(my->name), by_str("\""));
    }
cc_end_method


cc_begin_meta_method(MetaRoot, initVector)
    cc_method_fp initMethod;
    cc_vars_Root *obj;
    int i, nelems;

    if (argc < 2) {
        return cc_msg(my, "error", by_str("too few arguments to initVector for class \""),
                      by_str(my->name), by_str("\""));
    }

    // some optimization for vector init: look up the method only once (optimize for large vector);
    // this lookup is correct because we are using "my" as the class, not "my->isa" which would be the meta class
    //
    initMethod = cc_lookup_method(my, "init");
    if (!initMethod) {
        return cc_msg(my, "error", by_str("initVector called for class \""),
                      by_str(my->name), by_str("\" which lacks init method"));
    }

    // TODO:  should check for "!is_ptr && !is_obj && !is_any" and treat as error?
    // is it a pointer until it is made into an object?  or is it an object that is not initialized?
    //
    obj = (cc_vars_Root *) argv[0].u.p;
    nelems = as_int(argv[1]);
    for (i = 0;  i < nelems;  ++i) {

        obj->isa = my;

        // should there be an option to skip the initMethod for arrays allocated in zeroed memory (statically?)
        (void) initMethod(obj, "init", nelems - 2, &argv[2]);

        obj = (cc_vars_Root *) ((char *) obj + my->size);
    }
    return cc_null;
cc_end_method


cc_begin_method(Root, init)
    memset((char *) my + sizeof(cc_vars_Root), 0x00, my->isa->size - sizeof(cc_vars_Root));
    return by_obj(my);
cc_end_method


cc_begin_method(Root, free)
    free(my);
    return cc_null;
cc_end_method


cc_begin_method(Root, copy)
    cc_class_object *cls = my->isa;
    cc_vars_Root *copy = (cc_vars_Root *) as_obj(cc_msg(cls, "alloc"));
    memcpy((char *) copy + sizeof(cc_vars_Root), (char *) my + sizeof(cc_vars_Root),
           cls->size - sizeof(cc_vars_Root));
    return by_obj(copy);
cc_end_method


// this method gets used for MetaRoot too which works fine because my is not referenced
cc_begin_method(Root, error)
    int i;
    fprintf(stderr, "fatal:  ");
    for (i = 0;  i < argc;  ++i) {
        fprintf(stderr, "%s", as_str(argv[i]));
    }
    fprintf(stderr, "\n");
    abort();
cc_end_method


cc_construct_methods(MetaRoot, MetaRoot, 
    cc_method("new",        newMetaRoot),
    cc_method("alloc",      allocMetaRoot),
    cc_method("initVector", initVectorMetaRoot),
    cc_method("error",      errorRoot),
    )
cc_destruct_methods(MetaRoot)

cc_class_object MetaRoot = {
    NULL,
    NULL,
    "MetaRoot",
    (size_t) -1,
    0,
    NULL
};


cc_construct_methods(Root, Root,
    cc_method("init",  initRoot),
    cc_method("free",  freeRoot),
    cc_method("copy",  copyRoot),
    cc_method("error", errorRoot),
    )
cc_destruct_methods(Root)

cc_class_object Root = {
    &MetaRoot,
    NULL,
    "Root",
    sizeof(cc_vars_Root),
    0,
    NULL
};
