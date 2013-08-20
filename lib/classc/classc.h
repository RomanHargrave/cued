//
// classc.h
//
// Copyright (C) 2008-2013 Robert William Fuller <hydrologiccycle@gmail.com>
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

#ifndef CLASSC_H_INCLUDED
#define CLASSC_H_INCLUDED

#include <stdlib.h> // abort
#include <unistd.h> // ssize_t on BSD family


typedef void *cc_obj;

typedef enum _cc_type_t {

    cc_type_any = 0,
    cc_type_o,
    cc_type_p,
    cc_type_s,
    cc_type_sc,
    cc_type_h,
    cc_type_i,
    cc_type_l,
    cc_type_ll,
    cc_type_uc,
    cc_type_uh,
    cc_type_ui,
    cc_type_ul,
    cc_type_ull,
    cc_type_c,
    cc_type_f,
    cc_type_d,
    cc_type_sst,
    cc_type_st

} cc_type_t;

typedef struct _cc_arg_t {

    union {
        cc_obj o;
        void *p;
        const char *s;

        signed char sc;
        short int h;
        int i;
        long int l;
        long long int ll;

        unsigned char uc;
        unsigned short int uh;
        unsigned int ui;
        unsigned long int ul;
        unsigned long long int ull;

        char c;

        float f;
        double d;

        ssize_t sst;
        size_t st;
    } u;

    cc_type_t t;

} cc_arg_t;

extern const char *cc_type_names[];

#define cc_is_null(x) ({ cc_arg_t _cc_tmp_arg = (x);  _cc_tmp_arg.t == cc_type_any && !_cc_tmp_arg.u.ull; })
extern cc_arg_t cc_null;

#ifdef __cplusplus
#define by(x, y) ({ cc_arg_t _cc_tmp_arg;  _cc_tmp_arg.u.x = (y);  _cc_tmp_arg.t = cc_type_##x;  _cc_tmp_arg; })
#else
#define by(x, y) ((cc_arg_t) { .u = { .x = (y) }, .t = cc_type_##x })
#endif

#define by_obj(p)       by(o,   (p))
#define by_ptr(x)       by(p,   (x))
#define by_str(p)       by(s,   (p))
#define by_schar(c)     by(sc,  (c))
#define by_short(n)     by(h,   (n))
#define by_int(n)       by(i,   (n))
#define by_long(n)      by(l,   (n))
#define by_longlong(n)  by(ll,  (n))
#define by_uchar(c)     by(uc,  (c))
#define by_ushort(n)    by(uh,  (n))
#define by_uint(n)      by(ui,  (n))
#define by_ulong(n)     by(ul,  (n))
#define by_ulonglong(n) by(ull, (n))
#define by_char(x)      by(c,   (x))
#define by_float(n)     by(f,   (n))
#define by_double(n)    by(d,   (n))
#define by_ssize_t(n)   by(sst, (n))
#define by_size_t(n)    by(st,  (n))

#define as(x, y) ({ \
    cc_arg_t _cc_tmp_rc = (y); \
    if (cc_type_##x != _cc_tmp_rc.t && cc_type_any != _cc_tmp_rc.t) { \
        fprintf(stderr, "fatal:  expected type \"%s\", but received type \"%s\" at line %d in file \"%s\"\n", \
                cc_type_names[cc_type_##x], cc_type_names[_cc_tmp_rc.t], __LINE__, __FILE__); \
        abort(); \
    } \
    _cc_tmp_rc.u.x; })

#define as_obj(p)       as(o,   (p))
#define as_ptr(x)       as(p,   (x))
#define as_str(p)       as(s,   (p))
#define as_schar(c)     as(sc,  (c))
#define as_short(n)     as(h,   (n))
#define as_int(n)       as(i,   (n))
#define as_long(n)      as(l,   (n))
#define as_longlong(n)  as(ll,  (n))
#define as_uchar(c)     as(uc,  (c))
#define as_ushort(n)    as(uh,  (n))
#define as_uint(n)      as(ui,  (n))
#define as_ulong(n)     as(ul,  (n))
#define as_ulonglong(n) as(ull, (n))
#define as_char(x)      as(c,   (x))
#define as_float(n)     as(f,   (n))
#define as_double(n)    as(d,   (n))
#define as_ssize_t(n)   as(sst, (n))
#define as_size_t(n)    as(st,  (n))

#define is(x, y) ((cc_type_##x == (y).t) ? 1 : 0)

#define is_obj(p)       is(o,   (p))
#define is_ptr(x)       is(p,   (x))
#define is_str(p)       is(s,   (p))
#define is_schar(c)     is(sc,  (c))
#define is_short(n)     is(h,   (n))
#define is_int(n)       is(i,   (n))
#define is_long(n)      is(l,   (n))
#define is_longlong(n)  is(ll,  (n))
#define is_uchar(c)     is(uc,  (c))
#define is_ushort(n)    is(uh,  (n))
#define is_uint(n)      is(ui,  (n))
#define is_ulong(n)     is(ul,  (n))
#define is_ulonglong(n) is(ull, (n))
#define is_char(x)      is(c,   (x))
#define is_float(n)     is(f,   (n))
#define is_double(n)    is(d,   (n))
#define is_ssize_t(n)   is(sst, (n))
#define is_size_t(n)    is(st,  (n))


// for compilers that do not support empty arrays (Solaris)
#define cc_msg0(obj, msg)  (_cc_send(     obj, msg, 0, NULL))
#define cc_msg_super0(msg) (_cc_send_super(my, msg, 0, NULL))

#define cc_msg(obj, msg, ...) ({ \
    cc_arg_t _cc_tmp_args[] = { __VA_ARGS__ }; \
    _cc_send(obj, msg, sizeof(_cc_tmp_args) / sizeof(cc_arg_t), _cc_tmp_args); \
})

#define cc_msg_super(msg, ...) ({ \
    cc_arg_t _cc_tmp_args[] = { __VA_ARGS__ }; \
    _cc_send_super(my, msg, sizeof(_cc_tmp_args) / sizeof(cc_arg_t), _cc_tmp_args); \
})

#define cc_error(...) ({ \
    cc_arg_t _cc_tmp_args[] = { __VA_ARGS__ }; \
    _cc_error(my, msg, sizeof(_cc_tmp_args) / sizeof(cc_arg_t), _cc_tmp_args, __FILE__, __LINE__); \
})

extern cc_arg_t _cc_send      (cc_obj my, const char *msg, int argc, cc_arg_t *argv);
extern cc_arg_t _cc_send_super(cc_obj my, const char *msg, int argc, cc_arg_t *argv);
extern cc_arg_t _cc_error     (cc_obj my, const char *msg, int argc, cc_arg_t *argv, const char *fileName, int lineno);


typedef struct _cc_class_object cc_class_object;
extern cc_class_object MetaRoot, Root;
extern cc_class_object MetaAlloc, Alloc;


typedef struct _cc_method_name cc_method_name;
extern void _cc_add_methods (cc_class_object *cls, ssize_t numMethods, cc_method_name *newMethods);
extern void _cc_free_methods(cc_class_object *cls);


#define _CC_PRIORITY_ALLOC      110
#define _CC_PRIORITY_INT_ALLOC  120
#define _CC_PRIORITY_CLASS      130
#define _CC_PRIORITY_CATEGORY   140
#define _CC_PRIORITY_INTERPOSE  150

#define _cc_interpose(poser, deposed, priority) \
static void Interpose##poser(void) __attribute__((constructor(priority))); \
static void Interpose##poser(void) \
{ \
    cc_class_object saved; \
    saved   = deposed; \
    deposed = poser; \
    poser   = saved; \
}

#define cc_interpose(poser, deposed) _cc_interpose(poser, deposed, _CC_PRIORITY_INTERPOSE)

#define _cc_construct_methods(cls, prefix, priority, ...) \
static void prefix##Constructor(void) __attribute__((constructor(priority))); \
static void prefix##Constructor(void) \
{ \
    static cc_method_name prefix##Methods[] = { \
        __VA_ARGS__ \
    }; \
    _cc_add_methods(&cls, sizeof(prefix##Methods) / sizeof(cc_method_name), prefix##Methods); \
}

#define _cc_destruct_methods(cls, prefix, priority) \
static void prefix##Destructor(void) __attribute__((destructor(priority))); \
static void prefix##Destructor(void) \
{ \
    _cc_free_methods(&cls); \
}


#define _cc_class_no_methods(cls, supercls) \
cc_class_object cls = { \
    &Meta##cls, \
    supercls, \
    #cls, \
    sizeof(cc_vars_##cls), \
    0, \
    0, \
    NULL \
    };

#define cc_class_no_methods(cls) _cc_class_no_methods(cls, &cc_##cls##_isa)

#define _cc_class(cls, priority, supercls, ...) \
_cc_class_no_methods (cls, supercls) \
_cc_construct_methods(cls, cls, priority, __VA_ARGS__) \
_cc_destruct_methods (cls, cls, priority)

#define cc_class(cls, ...) _cc_class(cls, _CC_PRIORITY_CLASS, &cc_##cls##_isa, __VA_ARGS__)

#define _cc_class_object(cls, supercls) \
cc_class_object Meta##cls = { \
    NULL, \
    supercls, \
    "Meta" #cls, \
    -1, \
    0, \
    0, \
    NULL \
    };

#define cc_class_object(cls) _cc_class_object(cls, &cc_Meta##cls##_isa)

#define _cc_class_object_with_methods(cls, priority, supercls, ...) \
_cc_class_object(cls, supercls) \
_cc_construct_methods(Meta##cls, Meta##cls, priority, __VA_ARGS__) \
_cc_destruct_methods (Meta##cls, Meta##cls, priority)

#define cc_class_object_with_methods(cls, ...) \
       _cc_class_object_with_methods(cls, _CC_PRIORITY_CLASS, &cc_Meta##cls##_isa, __VA_ARGS__)

#define  _cc_category(cls, prefix, priority, ...) \
_cc_construct_methods(cls, prefix, priority, __VA_ARGS__) \
_cc_destruct_methods (cls, prefix, priority)

#define cc_category(cls, cat, ...) _cc_category(cls, cls##cat, _CC_PRIORITY_CATEGORY, __VA_ARGS__)

#define cc_method(name, function) { name, (cc_method_fp) function }


#define cc_begin_method(cls, fn_name) \
static cc_arg_t fn_name##cls(cc_vars_##cls *my, const char *msg, int argc, cc_arg_t *argv) \
{

#define cc_begin_meta_method(cls, fn_name) \
static cc_arg_t fn_name##cls(cc_class_object *my, const char *msg, int argc, cc_arg_t *argv) \
{

#define cc_end_method }


typedef cc_arg_t (*cc_method_fp)(cc_obj my, const char *msg, int argc, cc_arg_t *argv);

extern cc_method_fp cc_lookup_method(cc_class_object *cls, const char *msg);


struct _cc_method_name {

    const char *msg;

    cc_method_fp fn;
};

struct _cc_class_object {

    // isa will point to meta-class to support sending messages to class objects
    struct _cc_class_object *meta;

    // super class
    struct _cc_class_object *supercls;

    // class name
    const char *name;

    // size of an instance of the class
    ssize_t size;

    int flags;

    ssize_t numMethods;

    // method pointers paired with their names
    cc_method_name *methods;
};

#define _CC_FLAG_STATIC_METHODS     0x00000001

typedef struct _cc_vars_Root {

    cc_class_object *isa;

} cc_vars_Root;

#endif // CLASSC_H_INCLUDED
