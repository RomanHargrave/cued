#include "ob.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


cc_begin_method(Foo, test)
    int i;
    printf("args: ");
    for (i = 0;  i < argc;  ++i) {
        printf(" %p", argv[i].p);
    }
    printf("\n");

    printf("msg is %s\n", msg);
    printf("bar is %d\n", my->bar);

    return by_obj(my);
cc_end_method


cc_begin_method(Foo, setBar)
    my->bar = argv[0].i;
    return by_obj(my);
cc_end_method


cc_begin_class_object(Foo)
cc_end_class


cc_begin_class(Foo)
    cc_method("setBar", setBarFoo),
    cc_method("test", testFoo),
    cc_method("blow", testFoo),
    cc_method("blarf", testFoo),
    cc_method("forward", testFoo),
cc_end_class


cc_category(Foo, Blastme,
    cc_method("blacker", testFoo),
    cc_method("doh", testFoo),
    )


#include "firstcls.h"

int main(int argc, char *argv[])
{
#if 0
    printf("sizeof(cc_arg_t) == %lu\n", sizeof(cc_arg_t));
    printf("sizeof(cc_class_object) == %lu\n", sizeof(cc_class_object));
    printf("sizeof(Foo) == %lu\n", sizeof(Foo));

    //cc_obj f = as_obj(cc_msg(&Foo, "alloc"));
    //cc_obj f = as(o, cc_msg(&Foo, "alloc"));

    cc_obj f = as(o, cc_msg(&Foo, "new"));

    char test[5] = { 't', 'e', 's', 't', 0 };
    //char *test = "blowme";
    //char *test = "blarf";

    cc_msg(f, "setBar", by_int(34));

    cc_msg(f, test, by_int(1), by_int(2), by_obj(f));

    cc_msg(f, "blacker", by(i, 1));

    cc_msg(f, "free");

    printf("foo's name is %s\n", Foo.name);

    //cc_msg_super(&Foo, "doh");

    //cc_arg_t a;
    //int n;

    // warning: ISO C forbids casts to union type
    //a = (cc_arg_t) n;

    // error: aggregate value used where an integer was expected
    //n = (int) a;

    // error: incompatible types in assignment
    //n = a;

    //n = a.i;
#else

    cc_obj list = as_obj(cc_msg(&FcList, "new"));

    if (as_int(cc_msg(list, "isEmpty"))) {
        printf("list starts out as empty\n");
    }

    //cc_msg(as_obj(cc_msg(list, "prefix", by_str("doh"))), "affix", by_str("bleh"));
    //cc_msg(list, "prefix", by_str("doh"), by_str("bleh"));
    cc_msg(list, "affix", by_str("doh"), by_str("bleh"));

    if (!as_int(cc_msg(list, "isEmpty"))) {
        printf("list is no longer empty\n");
    }

    cc_obj cursor = as_obj(cc_msg(list, "cursor"));

#if 0
    char *item;
    for (item = as_str(cc_msg(cursor, "first"));  item;  item = as_str(cc_msg(cursor, "next"))) {
        printf("item in list is \"%s\";  current is %s\n", item, as_str(cc_msg(cursor, "current")));
    }
#else
    char *item;
    for (item = as_str(cc_msg(cursor, "last"));  item;  item = as_str(cc_msg(cursor, "previous"))) {
        printf("item in list is \"%s\";  current is %s\n", item, as_str(cc_msg(cursor, "current")));
    }
#endif

    printf("after removePrefix:\n");
    item = as_str(cc_msg(list, "removePrefix"));
    for (item = as_str(cc_msg(cursor, "first"));  item;  item = as_str(cc_msg(cursor, "next"))) {
        printf("item in list is \"%s\"\n", item);
    }
    
    printf("after removeAffix:\n");
    item = as_str(cc_msg(list, "removeAffix"));
    for (item = as_str(cc_msg(cursor, "first"));  item;  item = as_str(cc_msg(cursor, "next"))) {
        printf("item in list is \"%s\"\n", item);
    }

#if 0
    printf("after removePrefix:\n");
    item = as_str(cc_msg(list, "removePrefix"));
    for (item = as_str(cc_msg(cursor, "first"));  item;  item = as_str(cc_msg(cursor, "next"))) {
        printf("item in list is \"%s\"\n", item);
    }
#endif
    
#endif
    return (EXIT_SUCCESS);
}
