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

typedef union _cc_args_t {

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

} cc_args_t;

#define by(x, y)        (cc_args_t) { .x   = y }
#define by_obj(p)       (cc_args_t) { .o   = p }
#define by_ptr(x)       (cc_args_t) { .p   = x }
#define by_str(p)       (cc_args_t) { .s   = p }
#define by_schar(c)     (cc_args_t) { .sc  = c }
#define by_short(n)     (cc_args_t) { .h   = n }
#define by_int(n)       (cc_args_t) { .i   = n }
#define by_long(n)      (cc_args_t) { .l   = n }
#define by_longlong(n)  (cc_args_t) { .ll  = n }
#define by_uchar(c)     (cc_args_t) { .uc  = c }
#define by_ushort(n)    (cc_args_t) { .uh  = n }
#define by_uint(n)      (cc_args_t) { .ui  = n }
#define by_ulong(n)     (cc_args_t) { .ul  = n }
#define by_ulonglong(n) (cc_args_t) { .ull = n }
#define by_char(x)      (cc_args_t) { .c   = x }
#define by_float(n)     (cc_args_t) { .f   = n }
#define by_double(n)    (cc_args_t) { .d   = n }

#define as(t, e)        (e).t
#define as_obj(e)       (e).o
#define as_ptr(e)       (e).p
#define as_str(e)       (e).s
#define as_schar(e)     (e).sc
#define as_short(e)     (e).h
#define as_int(e)       (e).i
#define as_long(e)      (e).l
#define as_longlong(e)  (e).ll
#define as_uchar(e)     (e).uc
#define as_ushort(e)    (e).uh
#define as_uint(e)      (e).ui
#define as_ulong(e)     (e).ul
#define as_ulonglong(e) (e).ull
#define as_char(e)      (e).c
#define as_float(e)     (e).f
#define as_double(e)    (e).d

#define cc_msg(obj, msg, ...)  _cc_send      ( obj, msg, sizeof((union _cc_args_t[]) { __VA_ARGS__ }) / sizeof(union _cc_args_t), (union _cc_args_t[]) { __VA_ARGS__ } )

#define cc_msg_super(msg, ...) _cc_send_super(  my, msg, sizeof((union _cc_args_t[]) { __VA_ARGS__ }) / sizeof(union _cc_args_t), (union _cc_args_t[]) { __VA_ARGS__ } )

extern cc_args_t _cc_send      (cc_obj my, char *msg, int argc, cc_args_t *argv);
extern cc_args_t _cc_send_super(cc_obj my, char *msg, int argc, cc_args_t *argv);


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


#define cc_begin_method(fn_name, cls) \
static cc_args_t fn_name##cls(cc_vars_##cls *my, char *msg, int argc, cc_args_t *argv) \
{

#define cc_begin_meta_method(fn_name, cls) \
static cc_args_t fn_name##cls(cc_class_object *my, char *msg, int argc, cc_args_t *argv) \
{

#define cc_end_method }


typedef cc_args_t (*cc_method_fp)(cc_obj my, char *msg, int argc, cc_args_t *argv);

struct _cc_method_name {

    char *msg;
    union {
        cc_args_t (*fn)();
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
