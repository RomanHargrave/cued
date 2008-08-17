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


char *cc_types[] = {

    [cc_type_any] = "any",

    [cc_type_o]   = "object",
    [cc_type_p]   = "pointer",
    [cc_type_s]   = "string",

    [cc_type_sc]  = "signed char",
    [cc_type_h]   = "short",
    [cc_type_i]   = "int",
    [cc_type_l]   = "long",
    [cc_type_ll]  = "long long",

    [cc_type_uc]  = "unsigned char",
    [cc_type_uh]  = "unsigned short",
    [cc_type_ui]  = "unsigned int",
    [cc_type_ul]  = "unsigned long",
    [cc_type_ull] = "unsigned long long",

    [cc_type_c]   = "char",
    [cc_type_f]   = "float",
    [cc_type_d]   = "double"
};


static inline int strcmp2(const char *a, const char *b)
{
    return (a == b) ? 0 : strcmp(a, b);
}


typedef enum _cc_lookup_how {

    cc_lookup_by_ptr = 0,
    cc_lookup_by_strcmp = 1,
    cc_lookup_by_both = 2

} cc_lookup_how;


static inline cc_method_fp _cc_lookup_method_internal(
    cc_class_object *cls, char *msg, cc_lookup_how how)
{
    cc_method_name *methods = cls->methods;

    for (;;) {

        if (methods->msg) {

            //printf("comparing %s to %s\n", methods->msg, msg);

            switch (how) {

                case cc_lookup_by_ptr:
                    if (methods->msg == msg) {

                        // found a matching method name
                        return methods->u.fn;
                    }
                    break;

                case cc_lookup_by_strcmp:
                    if (!strcmp(methods->msg, msg)) {

                        // found a matching method name
                        return methods->u.fn;
                    }
                    break;

                case cc_lookup_by_both:
                    if (!strcmp2(methods->msg, msg)) {

                        // found a matching method name
                        return methods->u.fn;
                    }
                    break;
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
        } else if (cls->supercls) {

            // chain to super classes
            cls = cls->supercls;
            methods = cls->methods;
            continue;

        // out of places to look for methods
        } else {

            return NULL;
        }
    }
}


cc_method_fp _cc_lookup_method(cc_class_object *cls, char *msg)
{
#if 0
    //
    // This was a fundamentally misguided approach b/c you cannot mix
    // optimized and non-optimized modules, among other problems
    //
    // TODO:  an interesting conundrum:  if not optimizing, it may be more
    // efficient to call _cc_lookup_method_internal with cc_lookup_by_both;
    // for example, see gcc's -fmerge-constants option
    //
    cc_method_fp method = _cc_lookup_method_internal(cls, msg, cc_lookup_by_ptr);
    if (!method) {
        method = _cc_lookup_method_internal(cls, msg, cc_lookup_by_strcmp);
    }
#endif

#if defined(__GNUC__) && defined(__OPTIMIZE__)
    cc_method_fp method = _cc_lookup_method_internal(cls, msg, cc_lookup_by_both);
#else
    cc_method_fp method = _cc_lookup_method_internal(cls, msg, cc_lookup_by_strcmp);
#endif

    return method;
}


static inline cc_arg_t _cc_send_msg_internal(
    cc_class_object *cls,
    cc_vars_Root *obj,
    char *msg,
    int argc,
    cc_arg_t *argv)
{
    cc_method_fp method = _cc_lookup_method(cls, msg);
    if (!method) {
        if (strcmp2("forward", msg)) {
            method = _cc_lookup_method(cls, "forward");
        }
    }

    if (method) {
        return method(obj, msg, argc, argv);
    }

    // handle method not found
    if (strcmp2("error", msg)) {
        return cc_msg(obj, "error", by_str("could not send message \""), by_str(msg),
            by_str("\" to class \""), by_str(cls->name), by_str("\""));
    }

    fprintf(stderr, "fatal:  could not send message \"%s\" to class \"%s\"\n", msg, cls->name);
    abort();
}


cc_arg_t _cc_send(cc_obj my, char *msg, int argc, cc_arg_t *argv)
{
    cc_vars_Root *obj = (cc_vars_Root *) my;
    return _cc_send_msg_internal(obj->isa, my, msg, argc, argv);
}


cc_arg_t _cc_send_super(cc_obj my, char *msg, int argc, cc_arg_t *argv)
{
    cc_vars_Root *obj = (cc_vars_Root *) my;
    cc_class_object *supercls = obj->isa->supercls;

    if (!supercls) {
        return cc_msg(obj, "error", by_str("cannot send message to super class of class \""), by_str(obj->isa->name), by_str("\""));
    }

    return _cc_send_msg_internal(supercls, obj, msg, argc, argv);
}


// TODO:  it would be quicker to prepend rather than append the methods,
// but is that desirable?  Are category methods more likely to be called
// more frequently than other methods in the class?  Should the macro
// for a category allow specifying that in keeping with the idea that the
// programmer should be able to supply all the information they have
// at their disposal?  Would this be better served by turning the method
// array into an MRU structure?
//
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
