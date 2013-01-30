//
// unix.c:  general library routines
//
// Copyright (C) 2007,2008 Robert William Fuller <hydrologiccycle@gmail.com>
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

#include "unix.h"
#include "macros.h"
#include "dmalloc.h"

#include <stdlib.h> // strtol
#include <sys/stat.h> // mkdir
#include <unistd.h> // close
#include <time.h>
#include <errno.h>


char *noextname(const char *name)
{
    const char *end;
    char *newstr;
    int chars, bytes;

    // whacky, but follows mbcs pattern
    //
    end = strrchr(name, '.');
    if (end) {
        chars = end - name;
        bytes = chars * sizeof(char);
        newstr = (char *) libc_malloc(bytes + sizeof(char));
        if (newstr) {
            memcpy(newstr, name, bytes);
            newstr[chars] = 0;
        }
    } else {
        newstr = strdup(name);
    }

    return newstr;
}


const char *basename2(const char *name)
{
    const char *base = strrchr(name, '/');
    if (base) {
        base += 1;
    } else {
        base = name;
    }

    return base;
}


int strtol2(const char *str, char **endptr, int base, ssize_t *val)
{
    errno = 0;
    *val = strtol(str, endptr, base);

    return errno ? -1 : 0;
}


FILE *fopen2(const char *pathname, int flags, mode_t mode)
{
    PIT(FILE, fp);
    PIT(const char, fdmode);
    int fd;

    switch (O_ACCMODE & flags) {

        case O_RDONLY:
            fdmode = "r";
            break;

        case O_WRONLY:
            fdmode = (O_APPEND & flags) ? "a" : "w";
            break;

        case O_RDWR:
            fdmode = (O_APPEND & flags) ? "a+" : "w+";
            break;

        default:
            errno = EINVAL;
            return NULL;
            break;
    }

    fd = open(pathname, flags, mode);
    if (fd < 0) {
        return NULL;
    }

    fp = fdopen(fd, fdmode);
    if (!fp) {
        int saverrno = errno;
        close(fd);
        errno = saverrno;
    }

    return fp;
}


int mkdirp(char *pathname)
{
    char *subpath, *slash;
    int rc;

    // skip a leading slash because mkdir on "" does not succeed
    if (pathname[0] == '/') {
        subpath = &pathname[1];
    } else {
        subpath = pathname;
    }

    for (;;) {

        slash = strchr(subpath, '/');
        if (slash) {
            slash[0] = 0;
        }

        rc = mkdir(pathname, 0777);
        if (rc) {
            switch (errno) {

                case EEXIST:
                    break;

                default:
                    return rc;
            }
        }

        if (slash) {
            slash[0] = '/';
            subpath = &slash[1];
        } else {

            // if it already existed, and it's the last component,
            // then make sure it's a directory
            //
            if (rc) {
                struct stat statbuf;

                if (stat(pathname, &statbuf)) {
                    return -1;
                }

                if (!(S_ISDIR(statbuf.st_mode))) {
                    errno = EEXIST;
                    return -1;
                }
            }

            break;
        }
    }

    return 0;
}


int rfc3339time(char *buf, int bufferSize)
{
    struct tm lt;
    time_t t;
    int len, rc;

    t = time(0);
    if (-1 == t) {
        //printf("bad time()\n");
        return -1;
    }
    if (!localtime_r(&t, &lt)) {
        //printf("bad localtime_r\n");
        return -1;
    }

    rc = strftime(buf, bufferSize, "%Y-%m-%d %H:%M:%S%z", &lt);
    if (!rc || rc == bufferSize) {
        //printf("bad strftime: rc=%d\n", rc);
        return -1;
    }

    len = strlen(buf);

    // make sure there is enough room for the new null terminator
    if (bufferSize < len + 2) {
        return -1;
    }

    buf[len + 1] = 0;
    buf[len] = buf[len - 1];
    buf[len - 1] = buf[len - 2];
    buf[len - 2] = ':';

    return 0;
}


#ifndef CUED_HAVE_MEMRCHR

void *memrchr(const void *p, int c, size_t n)
{
    const char *s = (const char *) p;

    while (n--) {
        if (c == s[n]) {
            return (void *) &s[n];
        }
    }

    return NULL;
}

#endif // CUED_HAVE_MEMRCHR


#ifndef CUED_HAVE_MEMMEM

void *memmem(const void *str, size_t str_len, const void *sub, size_t sub_len)
{
    const char *cstr = (const char *) str;
    const char *csub = (const char *) sub;
    ssize_t n;

    for (n = str_len - sub_len;  n >= 0;  --n) {
        if (cstr[n] == csub[0] && !memcmp(&cstr[n], csub, sub_len)) {
            return (void *) &cstr[n];
        }
    }

    return NULL;
}

#endif // CUED_HAVE_MEMMEM
