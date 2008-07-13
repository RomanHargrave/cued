//
// classc.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


typedef struct _cc_vars_Root {

    cc_class_object *isa;

} cc_vars_Root;


cc_method_fp _cc_lookup_method(cc_class_object *cls, char *msg)
{
    cc_method_name *methods = cls->methods;

    for (;;) {

        if (methods->msg) {

            //printf("comparing %s to %s\n", methods->msg, msg);

            // compare method name
            if (methods->msg == msg || !strcmp(methods->msg, msg)) {

                return methods->u.fn;
            }

            // advance to next method
            ++methods;
            continue;

        // no more methods in this table;  is it chained to another?
        } else if (methods->u.next) {

            // chain to next table
            methods = methods->u.next;
            continue;

        // is there a super class?
        } else if (cls->super) {

            // chain to super classes
            cls = cls->super;
            methods = cls->methods;
            continue;

        // out of places to look for methods
        } else {

            return NULL;
        }
    }
}


static inline cc_args_t _cc_send_msg_internal(
    cc_class_object *cls,
    cc_vars_Root *obj,
    char *msg,
    int argc,
    cc_args_t *argv)
{
    cc_method_fp method = _cc_lookup_method(cls, msg);
    if (!method) {
        if ("forward" != msg && strcmp("forward", msg)) {
            method = _cc_lookup_method(cls, "forward");
        }
    }

    if (method) {
        return method(obj, msg, argc, argv);
    }

    // handle method not found
    if ("error" != msg && strcmp("error", msg)) {
        return cc_msg(obj, "error", by_str("could not send message \""), by_str(msg),
            by_str("\" to class \""), by_str(cls->name), by_str("\""));
    }

    fprintf(stderr, "fatal:  could not send message \"%s\" to class \"%s\"\n", msg, cls->name);
    abort();
}


cc_args_t _cc_send(cc_obj my, char *msg, int argc, cc_args_t *argv)
{
    cc_vars_Root *obj = (cc_vars_Root *) my;
    return _cc_send_msg_internal(obj->isa, my, msg, argc, argv);
}


cc_args_t _cc_send_super(cc_obj my, char *msg, int argc, cc_args_t *argv)
{
    cc_vars_Root *obj = (cc_vars_Root *) my;

    if (!obj->isa->super) {
        return cc_msg(obj, "error", by_str("cannot send message to super class of class \""), by_str(obj->isa->name), by_str("\""));
    }

    return _cc_send_msg_internal(obj->isa->super, my, msg, argc, argv);
}


void _cc_add_methods(cc_class_object *cls, cc_method_name *newMethods)
{
    cc_method_name *methods = cls->methods;
    for (;;) {
        if (methods->msg) {
            ++methods;
            continue;
        } else if (methods->u.next) {
            methods = methods->u.next;
            continue;
        } else {
            methods->u.next = newMethods;
            break;
        }
    }
}


cc_begin_meta_method(alloc, MetaRoot)
    cc_vars_Root *obj = (cc_vars_Root *) calloc(1, my->size);
    if (obj) {
        obj->isa = my;
    }

    printf("root allocated %p for class %s with size %lu\n", obj, my->name, my->size);

    return by_obj(obj);
cc_end_method


cc_begin_method(free, Root)
    free(my);

    printf("root freed %p\n", my);

    return by_ptr(NULL);
cc_end_method


static cc_args_t errorRoot(cc_obj my, char *msg, int argc, cc_args_t *argv)
{
    int i;
    fprintf(stderr, "fatal:  ");
    for (i = 0;  i < argc;  ++i) {
        fprintf(stderr, "%s", as_str(argv[i]));
    }
    fprintf(stderr, "\n");
    abort();
}


cc_class_object MetaRoot = {
    NULL,
    NULL,
    "MetaRoot",
    sizeof(cc_vars_Root),
    cc_begin_methods

    cc_method("alloc", allocMetaRoot),
    cc_method("error", errorRoot),

    cc_end_methods
};


cc_class_object Root = {
    &MetaRoot,
    NULL,
    "Root",
    sizeof(cc_vars_Root),
    cc_begin_methods

    cc_method("free",  freeRoot),
    cc_method("error", errorRoot),

    cc_end_methods
};
