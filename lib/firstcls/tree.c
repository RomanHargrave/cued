/*
** tree.c
**
** Copyright (C) 1996-2012 Robert William Fuller <hydrologiccycle@gmail.com>
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


// TREE_NODE_BLACK must be zero unless code is added
// to TreeInitInZeroedMemory() to set tree_t.sentinel.color to TREE_NODE_BLACK;
// formerly, these definitions were private to the implementation file,
// but they're needed for the TREE_DECLARE macro
//
#define	TREE_NODE_BLACK	0
#define	TREE_NODE_RED	1

#if 0

cc_class_object(FcTree)

cc_class(FcTree,
    cc_method("init",               initFcTree),
    cc_method("isEmpty",            isEmptyFcTree),

    // these are from list
    cc_method("prefix",             insertFcTree),
    cc_method("affix",              insertFcTree),
//  cc_method("removePrefix",       removePrefixFcTree),
//  cc_method("removeAffix",        removeAffixFcTree),


    // should list gain an insert/remove that do a comparison?
    cc_method("insert",             insertFcTree),
    cc_method("remove",             removeFcTree),

    // should list gain a findEqual that does a comparison?
    cc_method("findEqual",          findEqualFcTree),

    // list should have a free?  should this do other things than free?  take a method?
    cc_method("delete",             deleteFcTree),


    // these are specific to tree
    //
    // should these return a cursor?
    //
    cc_method("findGreater",        findGreaterFcTree),
    cc_method("findLesser",         findLesserFcTree),
    cc_method("findLesserOrEqual",  findLesserOrEqualFcTree),
    cc_method("findGreaterOrEqual", findGreaterOrEqualFcTree),
    cc_method("find",               findFcTree),

    cc_method("cursor",             cursorFcTree),
    )


cc_class_object(FcTreeCursor)

cc_class(FcTreeCursor,
    cc_method("current",    currentFcTreeCursor),
    cc_method("first",      firstFcTreeCursor),
    cc_method("last",       lastFcTreeCursor),
    cc_method("next",       nextFcTreeCursor),
    cc_method("previous",   prevFcTreeCursor),

//  cc_method("prefix",     prefixFcTreeCursor),
//  cc_method("affix",      affixFcTreeCursor),

    cc_method("remove",     removeFcTreeCursor),
    )

#endif