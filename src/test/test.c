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

#ifdef HAVE_CONFIG_H
#include "cued_config.h" // CUED_HAVE_MCHECK_H
#endif

#include "ob.h"

#include "firstcls.h"

#include "macros.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef CUED_HAVE_MCHECK_H
#include <mcheck.h>
#endif


cc_begin_meta_method(MetaAlloc, malloc)
    if (argc < 1) {
        return cc_error(by_str("too few arguments"));
    }
    printf("noisy malloc\n");
    return _cc_send_super(my, "malloc", argc, argv);
cc_end_method

cc_begin_meta_method(MetaAlloc, realloc)
    if (argc < 2) {
        return cc_error(by_str("too few arguments"));
    }
    printf("noisy realloc\n");
    return _cc_send_super(my, "realloc", argc, argv);
cc_end_method

cc_begin_meta_method(MetaAlloc, free)
    if (argc < 1) {
        return cc_error(by_str("too few arguments"));
    }
    printf("noisy free\n");
    return _cc_send_super(my, "free", argc, argv);
cc_end_method

_cc_class_object_with_methods(NoisyAlloc, _CC_PRIORITY_ALLOC, &MetaAlloc,
    cc_method("malloc",     mallocMetaAlloc),
    cc_method("free",       freeMetaAlloc),
    cc_method("realloc",    reallocMetaAlloc)
    )

#define cc_vars_NoisyAlloc cc_vars_Root
_cc_class_no_methods(NoisyAlloc, &Alloc)

_cc_interpose(NoisyAlloc, Alloc, _CC_PRIORITY_INT_ALLOC)


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


// string tests
//

void unitTestString()
{
    cc_obj s[5], s2;
    cc_obj t;
    ssize_t i;
 
    printf("\n\n*** STRING TESTS ***\n");

    s[0] = as_obj(cc_msg(&FcString, "new", by_str("foo")));
    cc_msg0(s[0], "writeln");

    s[1] = as_obj(cc_msg0(s[0], "copy"));
    cc_msg(s[1], "writeln", by_ptr(stdout));

    s[2] = as_obj(cc_msg(&FcString, "new", by_obj(s[0])));
    cc_msg0(s[2], "write");
    printf("\n");

    cc_msg(s[2], "setChar", by_ssize_t(1), by_char('b'));
    printf("char is %c\n", as_char(cc_msg(s[2], "getChar", by_ssize_t(1))));

    s[3] = as_obj(cc_msg(&FcString, "new", by_str("bark"), by_ssize_t(strlen("bark"))));
    cc_msg(s[3], "write", by_int(STDOUT_FILENO));
    printf("\n");

    printf("concat:\n");
    cc_msg(s[3], "concat", by_str("collar"));
    cc_msg0(s[3], "writeln");

    printf("substring:\n");
    s[4] = as_obj(cc_msg(s[3], "sub", by_ssize_t(1), by_ssize_t(4)));
    cc_msg0(s[4], "writeln");

    // test compare
    t = as_obj(cc_msg0(&FcTree, "new"));

    cc_msg(t, "insert", by_obj(s[0]), by_obj(s[1]), by_obj(s[2]), by_obj(s[3]), by_obj(s[4]));
    //_cc_send(t, "insert", SNELEMS(s), s);
#if 0
    for (i = 0;  i < SNELEMS(s);  ++i) {
        cc_msg(t, "insert", by_obj(s[i]));
    }
#endif

    printf("walk tree:\n");
    cc_msg(t, "apply", by_str("writeln"));

    // this only gets some of the strings (those in the tree, not the duplicates)
    //cc_msg(t, "free", by_int(FcEmptyFreeObject));
    cc_msg0(t, "free");

#if 1
    for (i = 0;  i < SNELEMS(s);  ++i) {
        cc_msg0(s[i], "free");
    }
#endif

    s2 = as_obj(cc_msg(&FcString, "new", by_str("/foo/bar/")));
    i = as_ssize_t(cc_msg(s2, "findChar", by_char('/')));
    printf("first index of %s is %zd\n", as_str(cc_msg0(s2, "buffer")), i);
    
    i = as_ssize_t(cc_msg(s2, "findChar", by_char('/'), by_ssize_t(i + 1)));
    printf("second index of %s is %zd\n", as_str(cc_msg0(s2, "buffer")), i);

    i = as_ssize_t(cc_msg(s2, "findChar", by_char('/'), by_ssize_t(i)));
    printf("second index of %s is %zd\n", as_str(cc_msg0(s2, "buffer")), i);

    i = as_ssize_t(cc_msg(s2, "findChar", by_char('/'), by_ssize_t(i + 1)));
    printf("third index of %s is %zd\n", as_str(cc_msg0(s2, "buffer")), i);

    i = as_ssize_t(cc_msg(s2, "findChar", by_char('/'), by_ssize_t(i)));
    printf("third index of %s is %zd\n", as_str(cc_msg0(s2, "buffer")), i);


    i = as_ssize_t(cc_msg(s2, "findCharRev", by_char('/')));
    printf("last index of %s is %zd\n", as_str(cc_msg0(s2, "buffer")), i);

    i = as_ssize_t(cc_msg(s2, "findCharRev", by_char('/'), by_ssize_t(i - 1)));
    printf("second to last index of %s is %zd\n", as_str(cc_msg0(s2, "buffer")), i);

    i = as_ssize_t(cc_msg(s2, "findCharRev", by_char('/'), by_ssize_t(i)));
    printf("second to last index of %s is %zd\n", as_str(cc_msg0(s2, "buffer")), i);

    i = as_ssize_t(cc_msg(s2, "findCharRev", by_char('/'), by_ssize_t(i - 1)));
    printf("third to last index of %s is %zd\n", as_str(cc_msg0(s2, "buffer")), i);


    i = as_ssize_t(cc_msg(s2, "findChar", by_char('/'), by_ssize_t(10)));
    printf("bad index of %s is %zd\n", as_str(cc_msg0(s2, "buffer")), i);

    i = as_ssize_t(cc_msg(s2, "findChar", by_char('/'), by_ssize_t(0)));
    printf("bad index of %s is %zd\n", as_str(cc_msg0(s2, "buffer")), i);

    i = as_ssize_t(cc_msg(s2, "find", by_str("foo")));
    printf("index of foo is %zd\n", i);

    i = as_ssize_t(cc_msg(s2, "find", by_str("this is a test designed")));
    printf("index of this is a test... is %zd\n", i);


    // test error handling
    //cc_msg(s2, "concat", by_double(8.0));


    cc_msg0(s2, "free");
}


// tree tests
//

//#define TREE_NODES 10000
#define TREE_NODES 100

#define TEST_NUM (TREE_NODES / 2)

int int_cmp(cc_arg_t item, cc_arg_t key)
{
    return as_int(item) - as_int(key);
}

void findTest(cc_obj t, const char *msg, int i)
{
    cc_arg_t item;
    item = cc_msg(t, msg, by_int(i));
    if (cc_is_null(item)) {
        printf("%s(%d) returns cc_null\n", msg, i);
    } else {
        printf("%s(%d) returns %d\n",      msg, i, as_int(item));
    }
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

    t = as_obj(cc_msg0(&FcTree, "new"));
    for (i = 0;  i < SNELEMS(fooVector);  ++i) {
        fooVector[i].bar = i;
        cc_msg(t, "insert", by_obj(&fooVector[i]));
    }
    cc_msg(t, "apply", by_str("test"), by_int(0xABCDEF));
    cc_msg0(t, "free");


    // C++ needs the cast to void (ugh)
    t = as_obj(cc_msg(&FcTree, "new", by_ptr((void *) int_cmp)));
    cursor = as_obj(cc_msg0(t, "cursor"));

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


    findTest(t, "findEqual", TEST_NUM);
    findTest(t, "findGreater", TEST_NUM);
    findTest(t, "findLesser", TEST_NUM);
    findTest(t, "findLesserOrEqual", TEST_NUM);
    findTest(t, "findGreaterOrEqual", TEST_NUM);

    cc_msg(t, "remove", by_int(TEST_NUM));
    printf("removed %d\n", TEST_NUM);

    cc_msg(t, "remove", by_int(TEST_NUM - 1));
    printf("removed %d\n", TEST_NUM - 1);

    findTest(t, "findEqual", TEST_NUM);
    findTest(t, "findGreater", TEST_NUM);
    findTest(t, "findLesser", TEST_NUM);
    findTest(t, "findLesserOrEqual", TEST_NUM);
    findTest(t, "findGreaterOrEqual", TEST_NUM);

    // test error handling
    //cc_msg(t, "findEqual", by_int(1), by_int(1));


#if 0
    // enumerate tree
    //

    rc = cc_msg(cursor, "last");
    for ( i = 1; ; ) {
        rc = cc_msg(cursor, "current");
        printf("%d ", as_int(rc));
        rc = cc_msg(cursor, "previous");
        if (cc_is_null(rc)) {
            break;
        }
        ++i;
    }
    printf("\ntree has %d nodes\n", i);
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


    // clean up
    //

    printf("random remove");
    for (i = 0;  ;  ++i) {
        j = rand() % TREE_NODES;
        cc_msg(t, "remove", by_int(n[j]));
        if (!(i % 1000)) {
            printf(".");
        }
        rc = cc_msg0(t, "isEmpty");
        if (as_int(rc)) {
            printf("tree is empty\n");
            break;
        }
    }


    cc_msg0(t,       "free");
    cc_msg0(cursor,  "free");

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

    list = as_obj(cc_msg0(&FcList, "new"));

    // empty?
    if (as_int(cc_msg0(list, "isEmpty"))) {
        printf("list starts out as empty\n");
    }

    //cc_msg(list, "removePrefix");

    cc_msg(list, "affix", by_str("doh"), by_str("bleh"));

    // empty?
    if (!as_int(cc_msg0(list, "isEmpty"))) {
        printf("list is no longer empty\n");
    }

    printf("after adding doh and bleh\n");

    // enumerate
    cursor = as_obj(cc_msg0(list, "cursor"));
    for (item = as_str(cc_msg0(cursor, "first"));  item;  item = as_str(cc_msg0(cursor, "next"))) {
        printf("item in list is \"%s\";  current is %s\n", item, as_str(cc_msg0(cursor, "current")));
    }

    cc_msg0(cursor, "first");

    // delete with cursor
    cc_msg0(cursor, "remove");

    printf("after first, remove:\n");
    for (item = as_str(cc_msg0(cursor, "first"));  item;  item = as_str(cc_msg0(cursor, "next"))) {
        printf("item in list is \"%s\"\n", item);
    }

    cc_msg0(cursor, "first");
    cc_msg(cursor, "prefix", by_str("foo"));
    cc_msg(cursor, "affix", by_str("bar"), by_str("baz"));
    cc_msg0(cursor, "previous");
    printf("after:  first, prefix foo, affix bar & baz, previous;  current is %s\n",
        as_str(cc_msg0(cursor, "current")));

    for (item = as_str(cc_msg0(cursor, "first"));  item;  item = as_str(cc_msg0(cursor, "next"))) {
        printf("item in list is \"%s\"\n", item);
    }

    // apply
    cc_msg(list, "apply", by_ptr((void *) applyStr));

    // test error handling
    //cc_msg0(list, "apply");


    // copy
    cursor2 = as_obj(cc_msg0(cursor, "copy"));
    printf("after copy:\n");
    for (item = as_str(cc_msg0(cursor2, "first"));  item;  item = as_str(cc_msg0(cursor2, "next"))) {
        printf("item in list is \"%s\"\n", item);
    }

    cc_msg0(cursor,  "free");
    cc_msg0(cursor2, "free");
    cc_msg0(list,    "free");
}


int main(int argc, char *argv[])
{
    cc_obj foo;
    cc_arg_t str;

    char test[5] = { 't', 'e', 's', 't', 0 };

#ifdef CUED_HAVE_MCHECK_H
    mtrace();
#endif

    printf("\n*** ELEMENTARY TESTS ***\n");

    printf("sizeof(cc_arg_t) == %zu\n", sizeof(cc_arg_t));
    printf("sizeof(cc_class_object) == %zu\n", sizeof(cc_class_object));
    printf("sizeof(Foo) == %zu\n", sizeof(Foo));


    foo = as_obj(cc_msg0(&Foo, "new"));

    // call a method to set a variable
    cc_msg(foo, "setBar", by_int(34));

    // iterate with argc/argv
    cc_msg(foo, test, by_int(1), by_int(2), by_obj(foo));

    // argv[0] access without arguments
    //cc_msg(foo, test);

    // category
    cc_msg(foo, "blacker", by_int(1));

    // message forwarding
    cc_msg0(foo, "not found");

    // free an object
    cc_msg0(foo, "free");


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
    unitTestString();


    // test error handling
    //foo = as_obj(cc_msg0(&Root, "new"));
    //cc_msg0(foo, "bar");
    //_cc_send_super(foo, "bar", 0, NULL);

#ifdef CUED_HAVE_MCHECK_H
    // stop tracing here in order to avoid seeing the unbalanced free of the methods
    // (class.c line 136: _cc_free_methods)
    muntrace();
#endif

    return (EXIT_SUCCESS);
}
