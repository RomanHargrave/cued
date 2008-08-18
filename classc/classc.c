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


#if defined(__GNUC__) && defined(__OPTIMIZE__)

static inline int strcmp2(const char *a, const char *b)
{
    return (a == b) ? 0 : strcmp(a, b);
}

#else
#define strcmp2 strcmp
#endif


cc_method_fp _cc_lookup_method(cc_class_object *cls, char *msg)
{
    cc_method_name *methods;
    int i;

    for (;;) {

        methods = cls->methods;
        for (i = 0;  i < cls->numMethods;  ++i) {
            if (!strcmp2(methods[i].msg, msg)) {

                // found a matching method name
                return methods[i].fn;
            }
        }

        // is there a super class?
        if (cls->supercls) {

            // chain to super classes
            cls = cls->supercls;
            continue;
        }

        // out of places to look for methods
        return NULL;
    }
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


void _cc_add_methods(cc_class_object *cls, size_t numMethods, cc_method_name *newMethods)
{
#if 0
    if (!numMethods) {
        return;
    }
#endif

    // TODO:  need a destructor??
    cls->methods = realloc(cls->methods, (cls->numMethods + numMethods) * sizeof(cc_method_name));
    if (!cls->methods) {
        fprintf(stderr, "fatal:  realloc failed in %s\n", __FUNCTION__);
        abort();
    }

    memcpy(&cls->methods[cls->numMethods], newMethods, numMethods * sizeof(cc_method_name));

    cls->numMethods += numMethods;
}
