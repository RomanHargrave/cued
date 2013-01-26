/*
** string.c
**
** Copyright (C) 2012 Robert William Fuller <hydrologiccycle@gmail.com>
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "firstcls.h"

#include <stdio.h>  // fwrite
#include <string.h> // strlen
#include <unistd.h> // write


cc_begin_method(FcString, concat)
    const char *str;
    char *buf;
    cc_obj obj;
    int len = 0;

    if (!strcmp(msg, "init")) {
        cc_msg_super("init");
    }

    if (argc) {

        if (is_str(argv[0])) {
            //
            // passed a const char *
            //
            str = as_str(argv[0]);
            switch (argc) {
                case 1:
                    //
                    // no length passed
                    //
                    len = strlen(str);
                    break;
                case 2:
                default:
                    //
                    // passed a length
                    //
                    len = as_int(argv[1]);
                    break;
            }

        } else if (is_obj(argv[0])) {
            //
            // passed an object that hopefully replies to length and buffer
            //
            obj = as_obj(argv[0]);
            len = as_int(cc_msg(obj, "length"));
            str = as_str(cc_msg(obj, "buffer"));
        }

        //
        // copy the source string
        //
        if (len) {
            buf = (char *) realloc(my->buffer, my->length + len + 1);
            if (!buf) {
                return cc_msg(my, "error", by_str("out of memory allocating string buffer"));
            }
            memcpy(buf + my->length, str, len);

            my->buffer  = buf;
            my->length += len;

            // null terminate
            buf[ my->length ] = 0;
        }
    }

    return by_obj(my);
cc_end_method


cc_begin_method(FcString, copy)
    return cc_msg(my->isa, "new", by_obj(my));
cc_end_method


cc_begin_method(FcString, sub)
    int begin, end;
    FcCheckArgcRange(1, 2);
    begin = as_int(argv[0]);
    end = (argc > 1) ? as_int(argv[1]) : my->length;
    if (begin < 1 || begin > my->length || begin > end || end > my->length  || end < 1) {
        return cc_null;
    } else {
        return cc_msg(my->isa, "new", by_str(&my->buffer[ begin - 1 ]), by_int(end - begin + 1));
    }
cc_end_method


cc_begin_method(FcString, buffer)
    return by_str(my->buffer);
cc_end_method


cc_begin_method(FcString, length)
    return by_int(my->length);
cc_end_method


cc_begin_method(FcString, isEmpty)
    return by_int(my->length ? 0 : 1);
cc_end_method


cc_begin_method(FcString, write)
    cc_arg_t outFile;
    FILE *file;
    int fd, writeln;

    writeln = !strcmp(msg, "writeln") ? 1 : 0;
    if (!argc) {
        outFile = by_ptr(stdout);
        argv = &outFile;
    }
    if (is_int(argv[0])) {
        fd = as_int(argv[0]);
        write(fd, my->buffer, my->length);
        if (writeln) {
            write(fd, "\n", sizeof(char));
        }
    } else if (is_ptr(argv[0])) {
        file = (FILE *) as_ptr(argv[0]);
        fwrite(my->buffer, sizeof(char), my->length, file);
        if (writeln) {
            fputc('\n', file);
        }
    }

    return by_obj(my);
cc_end_method


cc_begin_method(FcString, free)
    //printf("free buffer %s\n", my->buffer);
    free(my->buffer);
    return cc_msg_super("free");
cc_end_method


cc_begin_method(FcString, compare)
    cc_obj obj;
    const char *str;
    int len, rc;
    FcCheckArgc(1);
    obj = as_obj(argv[0]);
    str = as_str(cc_msg(obj, "buffer"));
    len = as_int(cc_msg(obj, "length"));
    rc = memcmp(my->buffer, str, (my->length < len) ? my->length : len);
    if (!rc && my->length != len) {
        rc = (my->length < len) ? -1 : 1;
    }
    return by_int(rc);
cc_end_method


cc_class_object(FcString)

cc_class(FcString,
    cc_method("init",    concatFcString),
    cc_method("buffer",  bufferFcString),
    cc_method("length",  lengthFcString),
    cc_method("copy",    copyFcString),
    cc_method("isEmpty", isEmptyFcString),
    cc_method("concat",  concatFcString),
    cc_method("write",   writeFcString),
    cc_method("writeln", writeFcString),
    cc_method("free",    freeFcString),
    cc_method("sub",     subFcString),
    cc_method("compare", compareFcString),
    )
