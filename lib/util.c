//
// util.c:
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

#include "util.h"

#include <stdlib.h>


int reallocItems(void **items, int itemSize, int *itemAlloc, int numItems, int newItems, int itemHint)
{
    void *rcAlloc;
    int emptyItems, allocItems;

    // for idempotence
    allocItems = *itemAlloc;
 
    // will the new entries fit?
    //
    emptyItems = allocItems - numItems;
    if (newItems > emptyItems) {

        // allocate the number of entries needed or the hint, whichever is more
        //
        allocItems = numItems + newItems;
        if (itemHint > allocItems) {
            allocItems = itemHint;
        }

        rcAlloc = (void *) realloc(*items, allocItems * itemSize);
        if (rcAlloc) {

            *items = rcAlloc;
            *itemAlloc = allocItems;

        } else {
            return -1;
        }
    }

    return 0;
}
