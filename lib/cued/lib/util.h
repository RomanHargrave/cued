//
// util.h
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

#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED


extern int util_realloc_items(void **items, int itemSize, int *itemAlloc, int numItems, int newItems, int itemHint);

extern int         util_add_context   (const void *key, const void *value);
extern const void *util_get_context   (const void *key);
extern int         util_remove_context(const void *key);


#endif // UTIL_H_INCLUDED
