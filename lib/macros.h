//
// macros.h
//
// Copyright (C) 2008 Robert William Fuller <hydrologiccycle@gmail.com>
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

#ifndef MACROS_H_INCLUDED
#define MACROS_H_INCLUDED

#include <string.h>


//#define SUBSTREQ(cnst, str) (!strncmp((cnst), (str), sizeof(cnst) - 1))
#define SUBSTREQ(cnst, str) (!strncmp((cnst), (str), strlen((cnst))))
#define PIT(type, name) type *name = (type *) -1
#define NELEMS(vector) (sizeof(vector) / sizeof(vector[0]))
#define SNELEMS(vector) ((ssize_t) NELEMS(vector))

// useful for getting rid of compiler warnings
// about signed vs. unsigned comparisons
//
#define ssizeof(x) ((ssize_t) sizeof(x))


#endif // MACROS_H_INCLUDED
