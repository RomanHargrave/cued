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
    printf("forward handler called with message \"%s\"\n", msg);
    return by_obj(my);
cc_end_method


cc_begin_method(Foo, compare)
    int rc;
    cc_vars_Foo *key = (cc_vars_Foo *) as_obj(argv[0]);
    rc = my->bar - key->bar;
    return by_int(rc);
cc_end_method


cc_class_object(Foo)


cc_class(Foo,
    cc_method("setBar", setBarFoo),
    cc_method("test", testFoo),
    cc_method("forward", forwardFoo),
    cc_method("compare", compareFoo),
    )

cc_category(Foo, Blastme,
    cc_method("blacker", testFoo),
    )


#include "firstcls.h"


// tree tests
//

#define NELEMS(vector) (sizeof(vector) / sizeof(vector[0]))
#define SNELEMS(vector) ((ssize_t) NELEMS(vector))

//#define TREE_NODES 10000
#define TREE_NODES 1000

int int_cmp(cc_arg_t item, cc_arg_t key)
{
    return as_int(item) - as_int(key);
}

void unitTestTree()
{
    int i, j;
    cc_vars_Foo fooVector[5];
    cc_arg_t rc;
    cc_obj t, cursor;
    FcTreeNode *node;
    int n[TREE_NODES];


    printf("\n\n*** TREE TESTS ***\n");


    // vector init
    cc_msg(&Foo, "initVector", by_ptr(fooVector), by_int(SNELEMS(fooVector)));
    //cc_msg(&fooVector[4], "test", by_int(5));

    t = as_obj(cc_msg(&FcTree, "new"));
    for (i = 0;  i < SNELEMS(fooVector);  ++i) {
        fooVector[i].bar = i;
        cc_msg(t, "insert", by_obj(&fooVector[i]));
    }
    cc_msg(t, "apply", by_str("test"), by_int(0xABCDEF));
    cc_msg(t, "free");


    // C++ needs the cast to void (ugh)
    t = as_obj(cc_msg(&FcTree, "new", by_ptr((void *) int_cmp)));
    cursor = as_obj(cc_msg(t, "cursor"));

    for (i = 0;  i < TREE_NODES;  ++i) {

        // insert randomly
        // n[i] = rand();

        // insert pathologically
        //
        n[i] = i;

        rc = cc_msg(t, "insert", by_int(n[i]));
        if (as_int(rc) != n[i]) {
            abort();
        }
    }


#if 0
    // enumerate tree
    //

    rc = cc_msg(cursor, "last");
    for ( i = 1; ; ) {
        rc = cc_msg(cursor, "current");
        printf("%d\n", as_int(rc));
        rc = cc_msg(cursor, "previous");
        if (cc_is_null(rc)) {
            break;
        }
        ++i;
    }
    printf("tree has %d nodes\n", i);
#endif


    // check depths
    //

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


    // remove (pathologically)
    //

    for (i = 0;  i < TREE_NODES * 95/100;  ++i) {
        cc_msg(t, "remove", by_int(n[i]));
    }


    // check depths
    //

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


    printf("random remove");
    for (j = 0;  ;  ++j) {
        i = rand() % TREE_NODES;
        cc_msg(t, "remove", by_int(n[i]));
        if (!(j % 1000)) {
            printf(".");
        }
        rc = cc_msg(t, "isEmpty");
        if (as_int(rc)) {
            printf("tree is empty\n");
            break;
        }
    }


    cc_msg(t,       "free");
    cc_msg(cursor,  "free");

    //cc_msg(t, "init", by_ptr(NULL), by_ptr(NULL));
}


// list tests
//

void applyStr(cc_arg_t item, int argc, cc_arg_t *argv)
{
    printf("apply:  string is %s\n", as_str(item));
}

void unitTestList()
{
    cc_obj list, cursor, cursor2;
    const char *item;


    printf("\n\n*** LIST TESTS ***\n");


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

    printf("after adding doh and bleh\n");

    // enumerate
    cursor = as_obj(cc_msg(list, "cursor"));
    for (item = as_str(cc_msg(cursor, "first"));  item;  item = as_str(cc_msg(cursor, "next"))) {
        printf("item in list is \"%s\";  current is %s\n", item, as_str(cc_msg(cursor, "current")));
    }

    cc_msg(cursor, "first");

    // delete with cursor
    cc_msg(cursor, "remove");

    printf("after first, remove:\n");
    for (item = as_str(cc_msg(cursor, "first"));  item;  item = as_str(cc_msg(cursor, "next"))) {
        printf("item in list is \"%s\"\n", item);
    }

    cc_msg(cursor, "first");
    cc_msg(cursor, "prefix", by_str("foo"));
    cc_msg(cursor, "affix", by_str("bar"), by_str("baz"));
    cc_msg(cursor, "previous");
    printf("after:  first, prefix foo, affix bar & baz, previous;  current is %s\n",
        as_str(cc_msg(cursor, "current")));

    for (item = as_str(cc_msg(cursor, "first"));  item;  item = as_str(cc_msg(cursor, "next"))) {
        printf("item in list is \"%s\"\n", item);
    }

    // apply
    cc_msg(list, "apply", by_ptr((void *) applyStr));


    // copy
    cursor2 = as_obj(cc_msg(cursor, "copy"));
    printf("after copy:\n");
    for (item = as_str(cc_msg(cursor2, "first"));  item;  item = as_str(cc_msg(cursor2, "next"))) {
        printf("item in list is \"%s\"\n", item);
    }

    cc_msg(cursor,  "free");
    cc_msg(cursor2, "free");
    cc_msg(list,    "free");
}


int main(int argc, char *argv[])
{
    cc_obj foo;
    cc_arg_t str;

    char test[5] = { 't', 'e', 's', 't', 0 };

    printf("\n*** ELEMENTARY TESTS ***\n");

    printf("sizeof(cc_arg_t) == %zu\n", sizeof(cc_arg_t));
    printf("sizeof(cc_class_object) == %zu\n", sizeof(cc_class_object));
    printf("sizeof(Foo) == %zu\n", sizeof(Foo));


    foo = as_obj(cc_msg(&Foo, "new"));

    // call a method to set a variable
    cc_msg(foo, "setBar", by_int(34));

    // iterate with argc/argv
    cc_msg(foo, test, by_int(1), by_int(2), by_obj(foo));

    // argv[0] access without arguments
    cc_msg(foo, test);

    // category
    cc_msg(foo, "blacker", by_int(1));

    // message forwarding
    cc_msg(foo, "not found");

    // free an object
    cc_msg(foo, "free");


    // class name
    printf("Foo's name is \"%s\"\n", Foo.name);


    //cc_arg_t a;
    //int n;

    // warning: ISO C forbids casts to union type
    //a = (cc_arg_t) n;

    // error: aggregate value used where an integer was expected
    //n = (int) a;

    // error: incompatible types in assignment
    //n = a;

    //n = a.i;


    // by/is
    str = by_str("foo");
    if (is_str(str)) {
        printf("is_str(str) returns true\n");
    }

    unitTestList();
    unitTestTree();

    return (EXIT_SUCCESS);
}
