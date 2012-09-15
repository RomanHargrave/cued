//
// unix.h
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
//

#ifndef UNIX_H_INCLUDED
#define UNIX_H_INCLUDED

#include <sys/types.h> // mode_t
#include <stdio.h> // FILE *
#include <fcntl.h> // mode: O_*


#define TIME_RFC_3339_LEN 25

extern char *noextname(const char *name);
extern const char *basename2(const char *name);
extern int strtol2(const char *str, char **endptr, int base, ssize_t *val);
extern FILE *fopen2(const char *pathname, int flags, mode_t mode);
extern int mkdirp(char *pathname);
extern int rfc3339time(char *buf, int bufferSize);


#endif // UNIX_H_INCLUDED
