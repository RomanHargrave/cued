/*
** common.c
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

#include <stdio.h>


int FcObjCompare(cc_arg_t item, cc_arg_t key)
{
    return as_int(cc_msg(as_obj(item), "compare", key));
}


unsigned FcObjHash(cc_arg_t item)
{
    return as_uint(cc_msg0(as_obj(item), "hash"));
}


cc_arg_t FcContainerFree(cc_obj my, const char *msg, int argc, cc_arg_t *argv)
{
    _cc_send(my, "empty", argc, argv);
    return cc_msg_super0("free");
}
