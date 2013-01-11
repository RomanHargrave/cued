//
// test.c
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

#include "ob.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


cc_begin_method(Foo, test)
    int i;
    printf("args: ");
    for (i = 0;  i < argc;  ++i) {
        argv[i].t = cc_type_any;
        printf(" %p", as_ptr(argv[i]));
    }
    printf("\n");

    if (!argc) {
        printf("argv[0] type is %d\n", argv[0].t);
    }

    printf("msg is %s\n", msg);
    printf("bar is %d\n", my->bar);

    return by_obj(my);
cc_end_method


cc_begin_method(Foo, setBar)
    my->bar = as_int(argv[0]);
    return by_obj(my);
cc_end_method


cc_begin_method(Foo, forward)
    printf("forward handler called with message %s!!!\n", msg);
    return by_obj(my);
cc_end_method


cc_class_object(Foo)


cc_class(Foo,
    cc_method("setBar", setBarFoo),
    cc_method("test", testFoo),
    cc_method("blow", testFoo),
    cc_method("blarf", testFoo),
    cc_method("forward", forwardFoo),
    )

cc_category(Foo, Blastme,
    cc_method("blacker", testFoo),
    cc_method("doh", testFoo),
    )


cc_vars_Foo fooVector[5];

#define NELEMS(vector) (sizeof(vector) / sizeof(vector[0]))

//#define TREE_NODES 1000000
#define TREE_NODES 1000

#include "firstcls.h"


int int_cmp(cc_arg_t item, cc_arg_t key)
{
    return as_int(item) - as_int(key);
}


int main(int argc, char *argv[])
{
    const char *item;
    cc_obj list, cursor, f, t;
    cc_arg_t foo, rc;

    char test[5] = { 't', 'e', 's', 't', 0 };

    printf("sizeof(cc_arg_t) == %zu\n", sizeof(cc_arg_t));
    printf("sizeof(cc_class_object) == %zu\n", sizeof(cc_class_object));
    printf("sizeof(Foo) == %zu\n", sizeof(Foo));

    f = as_obj(cc_msg(&Foo, "new"));

    // call a method to set a variable
    cc_msg(f, "setBar", by_int(34));

    // iterate with argc/argv
    cc_msg(f, test, by_int(1), by_int(2), by_obj(f));

    // argv[0] access without arguments
    cc_msg(f, test);

    // category
    cc_msg(f, "blacker", by_int(1));

    // message forwarding
    cc_msg(f, "not found");

    // free an object
    cc_msg(f, "free");

    // class name
    printf("foo's name is %s\n", Foo.name);


    //cc_arg_t a;
    //int n;

    // warning: ISO C forbids casts to union type
    //a = (cc_arg_t) n;

    // error: aggregate value used where an integer was expected
    //n = (int) a;

    // error: incompatible types in assignment
    //n = a;

    //n = a.i;


    // list
    //

    list = as_obj(cc_msg(&FcList, "new"));

    // empty?
    if (as_int(cc_msg(list, "isEmpty"))) {
        printf("list starts out as empty\n");
    }

    //cc_msg(list, "removePrefix");

    cc_msg(list, "affix", by_str("doh"), by_str("bleh"));

    // empty?
    if (!as_int(cc_msg(list, "isEmpty"))) {
        printf("list is no longer empty\n");
    }

    // enumerate
    cursor = as_obj(cc_msg(list, "cursor"));
    for (item = as_str(cc_msg(cursor, "first"));  item;  item = as_str(cc_msg(cursor, "next"))) {
        printf("item in list is \"%s\";  current is %s\n", item, as_str(cc_msg(cursor, "current")));
    }

    cc_msg(cursor, "first");

    // delete with cursor
    cc_msg(cursor, "remove");

    printf("after remove:\n");
    for (item = as_str(cc_msg(cursor, "first"));  item;  item = as_str(cc_msg(cursor, "next"))) {
        printf("item in list is \"%s\"\n", item);
    }

    cc_msg(cursor, "first");
    cc_msg(cursor, "prefix", by_str("foo"));
    cc_msg(cursor, "affix", by_str("bar"), by_str("baz"));
    cc_msg(cursor, "previous");
    printf("current is %s\n", as_str(cc_msg(cursor, "current")));

    printf("from the top:\n");
    for (item = as_str(cc_msg(cursor, "first"));  item;  item = as_str(cc_msg(cursor, "next"))) {
        printf("item in list is \"%s\"\n", item);
    }


    // copy
    cursor = as_obj(cc_msg(cursor, "copy"));
    printf("after copy:\n");
    for (item = as_str(cc_msg(cursor, "first"));  item;  item = as_str(cc_msg(cursor, "next"))) {
        printf("item in list is \"%s\"\n", item);
    }

    cc_msg(cursor, "free");

    cc_msg(list, "free");
    //cc_msg(list, "empty");


    // by/is
    foo = by_str("foo");
    if (is_str(foo)) {
        printf("is_str(foo) returns true\n");
    }


    // vector init
    cc_msg(&Foo, "initVector", by_ptr(fooVector), by_int(NELEMS(fooVector)));
    cc_msg(&fooVector[4], "test", by_int(5));


    // test tree
    //

    // C++ needs the cast to void (ugh)
    t = as_obj(cc_msg(&FcTree, "new", by_ptr((void *) int_cmp)));
    int i, n[TREE_NODES];
//    for (i = TREE_NODES - 1;  i >= 0;  --i) {
    for (i = 0;  i < TREE_NODES;  ++i) {
//        n[i] = rand();
        n[i] = i;
        rc = cc_msg(t, "insert", by_int(n[i]));
        if (as_int(rc) != n[i]) {
            printf("doh\n");
        }
    }

    // enumerate tree
    //

#if 0
    cursor = as_obj(cc_msg(t, "cursor"));
    rc = cc_msg(cursor, "first");
    for ( i = 1; ; ) {
        rc = cc_msg(cursor, "current");
        printf("%d\n", as_int(rc));
        rc = cc_msg(cursor, "next");
        if (cc_is_null(rc)) {
            break;
        }
        ++i;
    }
    printf("tree has %d nodes\n", i);
#endif

    FcTreeNode *node;

    node = ((cc_vars_FcTree *) t)->root;
    for (i = 0;  node != &((cc_vars_FcTree *) t)->sentinel;  ++i) {
        node = node->left;
    }
    printf("left depth is %d\n", i);

    node = ((cc_vars_FcTree *) t)->root;
    for (i = 0;  node != &((cc_vars_FcTree *) t)->sentinel;  ++i) {
        node = node->right;
    }
    printf("right depth is %d\n", i);

#if 1
    for (i = 0;  i < TREE_NODES * 5/6;  ++i) {
        cc_msg(t, "remove", by_int(n[i]));
    }
#else
    for (i = TREE_NODES - 1;  i > TREE_NODES / 2;  --i) {
        cc_msg(t, "remove", by_int(n[i]));
    }
#endif

    node = ((cc_vars_FcTree *) t)->root;
    for (i = 0;  node != &((cc_vars_FcTree *) t)->sentinel;  ++i) {
        node = node->left;
    }
    printf("left depth is %d\n", i);

    node = ((cc_vars_FcTree *) t)->root;
    for (i = 0;  node != &((cc_vars_FcTree *) t)->sentinel;  ++i) {
        node = node->right;
    }
    printf("right depth is %d\n", i);

#if 1
    for (;;) {
        i = rand() % TREE_NODES;
        cc_msg(t, "remove", by_int(n[i]));
        rc = cc_msg(t, "isEmpty");
        if (as_int(rc)) {
            printf("tree is empty\n");
            break;
        }
    }
#endif

    cc_msg(t, "free");

    //cc_msg(t, "init", by_ptr(NULL), by_ptr(NULL));

    return (EXIT_SUCCESS);
}
