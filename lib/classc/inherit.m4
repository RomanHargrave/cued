define(`comment',`dnl')dnl
comment(

inherit.m4

Copyright (C) 2008 Robert William Fuller <hydrologiccycle@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

)
dnl
comment(        define standard guards for header files         )
dnl
define(`basename', `patsubst(__file__, `^\(.*/\)?\(.*\)\.m4$', `\2')')dnl
define(`upcase', `translit(`$*', `a-z', `A-Z')')dnl
define(`htoken', `upcase(basename)')dnl
define(`guard_h', ``#'ifndef' `htoken()'_H_INCLUDED
``#'define' `htoken()'_H_INCLUDED

`undivert(1)'`dnl')dnl
dnl
define(`unguard_h', `undivert(`1', `2')'
``#'endif //' `htoken()'_H_INCLUDED)dnl
dnl
comment(        include standard header                         )
dnl
divert(1)`#'include "classc.h"
`'divert(0)dnl
dnl
comment(        define class in terms of what it inherits       )
dnl
define(`cvars_Root', `    cc_class_object *isa;')dnl
define(`class', `define(`cvars_$1', cvars_$2
`shift(shift($@))')'dnl
typedef struct _cc_vars_$1 {
cvars_$2
`shift(shift($@))'
} cc_vars_$1;dnl
`divert(1)'dnl
extern cc_class_object *$1`,' $1Obj`,' Meta$1Obj;
`divert(0)'dnl
`divert(2)'
``#ifndef'' cc_$1_isa
``#define'' cc_$1_isa $2Obj
``#define'' cc_Meta$1_isa Meta$2Obj
``#endif''
`divert(0)')dnl
dnl
comment(        inherit from classes in another m4 file         )
define(`inherit', `divert(-1)'`include($1)'`divert(0)'`dnl')dnl
dnl