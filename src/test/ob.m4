include(`inherit.m4')
comment(    crap.m4 could contain foo, and it would not be output here, although it could be inherited    )
inherit(`crap.m4')
guard_h

class(Foo, Root,
`    char foo[108];'
`    int bar;')

class(Bar, Foo,
`    char filthy;')

class(Baz, Bar,
`    int doh;')

unguard_h
