//
// root.c
//
// Copyright (C) 2008 Robert William Fuller <hydrologiccycle@gmail.com>
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
        return cc_msg(obj, "init");
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


cc_begin_method(Root, init)
    memset((char *) my + sizeof(cc_vars_Root), 0x00, my->isa->size - sizeof(cc_vars_Root));
    return by_obj(my);
cc_end_method


cc_begin_method(Root, free)
    free(my);
    return by_ptr(NULL);
cc_end_method


cc_begin_method(Root, copy)
    cc_class_object *cls = my->isa;
    cc_vars_Root *copy = as_obj(cc_msg(cls, "alloc"));
    memcpy((char *) copy + sizeof(cc_vars_Root), (char *) my + sizeof(cc_vars_Root), cls->size - sizeof(cc_vars_Root));
    return by_obj(copy);
cc_end_method


static cc_arg_t errorRoot(cc_obj my, char *msg, int argc, cc_arg_t *argv)
{
    int i;
    fprintf(stderr, "fatal:  ");
    for (i = 0;  i < argc;  ++i) {
        fprintf(stderr, "%s", as_str(argv[i]));
    }
    fprintf(stderr, "\n");
    abort();
}


cc_construct_methods(MetaRoot, MetaRoot, 
    cc_method("alloc", allocMetaRoot),
    cc_method("new",   newMetaRoot),
    cc_method("error", errorRoot),
    )

cc_class_object MetaRoot = {
    NULL,
    NULL,
    "MetaRoot",
    sizeof(cc_vars_Root),
    0,
    NULL
};


cc_construct_methods(Root, Root,
    cc_method("free",  freeRoot),
    cc_method("init",  initRoot),
    cc_method("error", errorRoot),
    cc_method("copy",  copyRoot),
    )

cc_class_object Root = {
    &MetaRoot,
    NULL,
    "Root",
    sizeof(cc_vars_Root),
    0,
    NULL
};
