//
// classc.h
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

#ifndef CLASSC_H_INCLUDED
#define CLASSC_H_INCLUDED

#include <stdlib.h> // size_t


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
    cc_type_d

} cc_type_t;

typedef struct _cc_arg_t {

    union {
        cc_obj o;
        void *p;
        char *s;

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
    } u;

    cc_type_t t;

} cc_arg_t;

#define by(x, y) (cc_arg_t) { .u = { .x = (y) }, .t = cc_type_##x }

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

#if defined(__GNUC__)
#define as(x, y) ({ \
    cc_arg_t _cc_tmp_rc = (y); \
    if (cc_type_##x != _cc_tmp_rc.t && cc_type_any != _cc_tmp_rc.t) { \
        abort(); \
    } \
    _cc_tmp_rc.u.x; })
#else
#warning "Type checking is not implemented for this compiler"
#define as(x, y) ((y).u.x)
#endif

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

// TODO:  multiple evaluation of VA_ARGS is a problem here,
// which begs for a gcc front-end for classc
//

#define cc_msg(obj, msg, ...)  _cc_send      ( obj, msg, sizeof((struct _cc_arg_t[]) { __VA_ARGS__ }) / sizeof(struct _cc_arg_t), (struct _cc_arg_t[]) { __VA_ARGS__ } )

#define cc_msg_super(msg, ...) _cc_send_super(  my, msg, sizeof((struct _cc_arg_t[]) { __VA_ARGS__ }) / sizeof(struct _cc_arg_t), (struct _cc_arg_t[]) { __VA_ARGS__ } )

extern cc_arg_t _cc_send      (cc_obj my, char *msg, int argc, cc_arg_t *argv);
extern cc_arg_t _cc_send_super(cc_obj my, char *msg, int argc, cc_arg_t *argv);


typedef struct _cc_class_object cc_class_object;
extern cc_class_object MetaRoot, Root;


#define cc_begin_class(cls) \
cc_class_object cls = { \
    &Meta##cls, \
    &cc_##cls##_isa, \
    #cls, \
    sizeof(cc_vars_##cls), \
    {

#define cc_begin_class_object(cls) \
cc_class_object Meta##cls = { \
    NULL, \
    &cc_Meta##cls##_isa, \
    "Meta" #cls, \
    sizeof(cc_vars_##cls), \
    {

#define cc_end_class cc_end_methods };


typedef struct _cc_method_name cc_method_name;
extern void _cc_add_methods(cc_class_object *cls, cc_method_name *newMethods);

#define cc_category(cls, cat, ...) \
static void cls##cat##Constructor(void) __attribute__((constructor)); \
static void cls##cat##Constructor(void) \
{ \
    static cc_method_name cls##cat[] = { \
    __VA_ARGS__ \
    cc_end_methods; \
    _cc_add_methods(&cls, cls##cat); \
}


#define cc_begin_methods {
#define cc_method(name, function) { name, { .fn = function } }
#define cc_end_methods { NULL, { .next = NULL } } }


#define cc_begin_method(cls, fn_name) \
static cc_arg_t fn_name##cls(cc_vars_##cls *my, char *msg, int argc, cc_arg_t *argv) \
{

#define cc_begin_meta_method(cls, fn_name) \
static cc_arg_t fn_name##cls(cc_class_object *my, char *msg, int argc, cc_arg_t *argv) \
{

#define cc_end_method }


typedef cc_arg_t (*cc_method_fp)(cc_obj my, char *msg, int argc, cc_arg_t *argv);

struct _cc_method_name {

    char *msg;
    union {
        cc_arg_t (*fn)();
        struct _cc_method_name *next;
    } u;

};

struct _cc_class_object {

    // isa will point to meta-class to support sending messages to class objects
    struct _cc_class_object *meta;

    // super class
    struct _cc_class_object *supercls;

    // class name
    char *name;

    // size of an instance of the class
    size_t size;

    // method pointers paired with their names
    cc_method_name methods[];
};

extern cc_method_fp _cc_lookup_method(cc_class_object *cls, char *msg);


typedef struct _cc_vars_Root {

    cc_class_object *isa;

} cc_vars_Root;

#endif // CLASSC_H_INCLUDED
