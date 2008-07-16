#include "ob.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


cc_begin_method(test, Foo)
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


cc_begin_method(setBar, Foo)
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


int main(int argc, char *argv[])
{
    printf("sizeof(cc_args_t) == %lu\n", sizeof(cc_args_t));
    printf("sizeof(cc_class_object) == %lu\n", sizeof(cc_class_object));
    printf("sizeof(Foo) == %lu\n", sizeof(Foo));

    //cc_obj f = as_obj(cc_msg(&Foo, "alloc"));
    cc_obj f = as(o, cc_msg(&Foo, "alloc"));

    char test[5] = { 't', 'e', 's', 't', 0 };
    //char *test = "blowme";
    //char *test = "blarf";

    cc_msg(f, "setBar", by_int(34));

    cc_msg(f, test, by_int(1), by_int(2), by_obj(f));

    cc_msg(f, "blacker", by(i, 1));

    cc_msg(f, "free");

    printf("foo's name is %s\n", Foo.name);

    //cc_msg_super(&Foo, "doh");

    //cc_args_t a;
    //int n;

    // warning: ISO C forbids casts to union type
    //a = (cc_args_t) n;

    // error: aggregate value used where an integer was expected
    //n = (int) a;

    // error: incompatible types in assignment
    //n = a;

    //n = a.i;

    return (EXIT_SUCCESS);
}
