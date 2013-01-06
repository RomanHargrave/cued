/*
** tree.c
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


// TREE_NODE_BLACK must be zero unless code is added
// to TreeInitInZeroedMemory() to set tree_t.sentinel.color to TREE_NODE_BLACK;
// formerly, these definitions were private to the implementation file,
// but they're needed for the TREE_DECLARE macro
//
#define TREE_NODE_BLACK 0
#define TREE_NODE_RED   1


cc_begin_method(FcTree, init)
    cc_msg_super("init");
    my->root = &my->sentinel;
    my->cmpMethod = (argc >= 1) ? (FcCompare) as_ptr(argv[0]) : FcObjCompare;
    return by_obj(my);
cc_end_method


cc_begin_method(FcTree, isEmpty)
    int rc;
    rc = (&my->sentinel == my->root) ? 1 : 0;
    return by_int(rc);
cc_end_method


static FcTreeNode *TreeFindEqual(cc_vars_FcTree *theTree, cc_arg_t theKey)
{
    int aCmpResult;

    FcTreeNode *aTreeNode = theTree->root;
    while (aTreeNode != &theTree->sentinel) {
        aCmpResult = (theTree->cmpMethod)(aTreeNode->item, theKey);
        if (aCmpResult < 0) {
            aTreeNode = aTreeNode->right;
        } else if (aCmpResult > 0) {
            aTreeNode = aTreeNode->left;
        } else {
            // found
            break;
        }
    }

    return aTreeNode;
}


static FcTreeNode *TreeFindGreater(cc_vars_FcTree *theTree, cc_arg_t theKey)
{
    int aCmpResult;
    FcTreeNode *aGreaterNode = &theTree->sentinel;

    FcTreeNode *aTreeNode = theTree->root;
    while (aTreeNode != &theTree->sentinel) {
        aCmpResult = (theTree->cmpMethod)(aTreeNode->item, theKey);
        if (aCmpResult < 0) {
            aGreaterNode = aTreeNode;
            aTreeNode = aTreeNode->right;
        } else if (aCmpResult > 0) {
            aTreeNode = aTreeNode->left;
        } else {
            // if the data items are equal, check for a greater descendant
            // tree;  if there is a greater descendant tree, find its 
            // least member
            //
            if (aTreeNode->right != &theTree->sentinel) {
                aGreaterNode = aTreeNode->right;
                while (aGreaterNode->left != &theTree->sentinel)
                    aGreaterNode = aGreaterNode->left;
            }
            break;
        }
    }

    return aGreaterNode;
}


static FcTreeNode *TreeFindLesser(cc_vars_FcTree *theTree, cc_arg_t theKey)
{
    int aCmpResult;
    FcTreeNode *aLesserNode = &theTree->sentinel;

    FcTreeNode *aTreeNode = theTree->root;
    while (aTreeNode != &theTree->sentinel) {
        aCmpResult = (theTree->cmpMethod)(aTreeNode->item, theKey);
        if (aCmpResult < 0) {
            aTreeNode = aTreeNode->right;
        } else if (aCmpResult > 0) {
            aLesserNode = aTreeNode;
            aTreeNode = aTreeNode->left;
        } else {
            if (aTreeNode->left != &theTree->sentinel) {
                aLesserNode = aTreeNode->left;
                while (aLesserNode->right != &theTree->sentinel)
                    aLesserNode = aLesserNode->right;
            }
            break;
        }
    }

    return aLesserNode;
}


static FcTreeNode *TreeFindLesserOrEqual(cc_vars_FcTree *theTree, cc_arg_t theKey)
{
    int aCmpResult;
    FcTreeNode *aLesserNode = &theTree->sentinel;

    FcTreeNode *aTreeNode = theTree->root;
    while (aTreeNode != &theTree->sentinel) {
        aCmpResult = (theTree->cmpMethod)(aTreeNode->item, theKey);
        if (aCmpResult < 0) {
            aTreeNode = aTreeNode->right;
        } else if (aCmpResult > 0) {
            aLesserNode = aTreeNode;
            aTreeNode = aTreeNode->left;
        } else {
            return aTreeNode;
        }
    }

    return aLesserNode;
}


static FcTreeNode *TreeFindGreaterOrEqual(cc_vars_FcTree *theTree, cc_arg_t theKey)
{
    int aCmpResult;
    FcTreeNode *aGreaterNode = &theTree->sentinel;

    FcTreeNode *aTreeNode = theTree->root;
    while (aTreeNode != &theTree->sentinel) {
        aCmpResult = (theTree->cmpMethod)(aTreeNode->item, theKey);
        if (aCmpResult < 0) {
            aGreaterNode = aTreeNode;
            aTreeNode = aTreeNode->right;
        } else if (aCmpResult > 0) {
            aTreeNode = aTreeNode->left;
        } else {
            return aTreeNode;
        }
    }

    return aGreaterNode;
}


cc_begin_method(FcTree, findEqual)
    FcTreeNode *aTreeNode = TreeFindEqual(my, argv[0]);
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTree, findGreater)
    FcTreeNode *aTreeNode = TreeFindGreater(my, argv[0]);
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTree, findLesser)
    FcTreeNode *aTreeNode = TreeFindLesser(my, argv[0]);
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTree, findLesserOrEqual)
    FcTreeNode *aTreeNode = TreeFindLesserOrEqual(my, argv[0]);
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTree, findGreaterOrEqual)
    FcTreeNode *aTreeNode = TreeFindGreaterOrEqual(my, argv[0]);
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTree, cursor)
    cc_vars_FcTreeCursor *cursor = (cc_vars_FcTreeCursor *) as_obj(cc_msg(&FcTreeCursor, "alloc"));

    // init is not called here or in list
    // TODO:  should cursor's init method take a tree?  then we would call new here
    cursor->tree = my;

    // start in the midst of the tree:  a useful diagnostic
    cursor->curr = my->root;

    return by_obj(cursor);
cc_end_method


cc_class_object(FcTree)

cc_class(FcTree,
    cc_method("init",               initFcTree),
    cc_method("isEmpty",            isEmptyFcTree),
    cc_method("findEqual",          findEqualFcTree),
    cc_method("findGreater",        findGreaterFcTree),
    cc_method("findLesser",         findLesserFcTree),
    cc_method("findLesserOrEqual",  findLesserOrEqualFcTree),
    cc_method("findGreaterOrEqual", findGreaterOrEqualFcTree),
    cc_method("cursor",             cursorFcTree),
    )


#if 0

    // these are from list
    cc_method("prefix",             insertFcTree),
    cc_method("affix",              insertFcTree),
//  cc_method("removePrefix",       removePrefixFcTree),
//  cc_method("removeAffix",        removeAffixFcTree),

    // should list gain a findEqual that does a comparison?

    // should list gain an insert/remove that do a comparison?
    cc_method("insert",             insertFcTree),
    cc_method("remove",             removeFcTree),

    //
    // list should have a free?  should this do other things than free?  take a method?
    //
    cc_method("delete",             deleteFcTree),

#endif


cc_begin_method(FcTreeCursor, findEqual)
    FcTreeNode *aTreeNode = TreeFindEqual(my->tree, argv[0]);
    my->curr = aTreeNode;
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTreeCursor, findGreater)
    FcTreeNode *aTreeNode = TreeFindGreater(my->tree, argv[0]);
    my->curr = aTreeNode;
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTreeCursor, findLesser)
    FcTreeNode *aTreeNode = TreeFindLesser(my->tree, argv[0]);
    my->curr = aTreeNode;
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTreeCursor, findLesserOrEqual)
    FcTreeNode *aTreeNode = TreeFindLesserOrEqual(my->tree, argv[0]);
    my->curr = aTreeNode;
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTreeCursor, findGreaterOrEqual)
    FcTreeNode *aTreeNode = TreeFindGreaterOrEqual(my->tree, argv[0]);
    my->curr = aTreeNode;
    return aTreeNode->item;
cc_end_method


static FcTreeNode *TreeMinimum(cc_vars_FcTree *theTree)
{
    FcTreeNode *aMinimumNode = &theTree->sentinel;
    FcTreeNode *aTreeNode = theTree->root;

    // find the left-most node of the tree
    //
    while (aTreeNode != &theTree->sentinel) {
        aMinimumNode = aTreeNode;
        aTreeNode = aTreeNode->left;
    }

    return aMinimumNode;
}


static FcTreeNode *TreeMaximum(cc_vars_FcTree *theTree)
{
    FcTreeNode *aMaximumNode = &theTree->sentinel;
    FcTreeNode *aTreeNode = theTree->root;

    // find the right-most node of the tree
    //
    while (aTreeNode != &theTree->sentinel) {
        aMaximumNode = aTreeNode;
        aTreeNode = aTreeNode->right;
    }

    return aMaximumNode;
}


static FcTreeNode *TreeSuccessor(cc_vars_FcTree *theTree, FcTreeNode *theTreeNode)
{
    if (&theTree->sentinel != theTreeNode) {

        // if the tree has a right subtree,
        // then the successor is in the right subtree
        //
        if (theTreeNode->right != &theTree->sentinel) {
            theTreeNode = theTreeNode->right;
            while (theTreeNode->left != &theTree->sentinel)
                theTreeNode = theTreeNode->left;

        } else {
            FcTreeNode *anAncestor;

            // otherwise, search up the tree until an ancestor right
            // of the child is found (if any)
            //
            while (&theTree->sentinel != (anAncestor = theTreeNode->parent)) {
                if (anAncestor->left == theTreeNode)
                    break;

                theTreeNode = anAncestor;
            }

            return anAncestor;
        }
    }

    return theTreeNode;
}


static FcTreeNode *TreePredecessor(cc_vars_FcTree *theTree, FcTreeNode *theTreeNode)
{
    if (&theTree->sentinel != theTreeNode) {

        // if the tree has a left subtree,
        // then the predecessor is in the left subtree
        //
        if (theTreeNode->left != &theTree->sentinel) {
            theTreeNode = theTreeNode->left;
            while (theTreeNode->right != &theTree->sentinel)
                theTreeNode = theTreeNode->right;

        } else {
            FcTreeNode *anAncestor;

            // otherwise, search up the tree until an ancestor left
            // of the child is found (if any)
            //
            while (&theTree->sentinel != (anAncestor = theTreeNode->parent)) {
                if (anAncestor->right == theTreeNode)
                    break;

                theTreeNode = anAncestor;
            }

            return anAncestor;
        }
    }

    return theTreeNode;
}


cc_begin_method(FcTreeCursor, first)
    FcTreeNode *aTreeNode = TreeMinimum(my->tree);
    my->curr = aTreeNode;
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTreeCursor, last)
    FcTreeNode *aTreeNode = TreeMaximum(my->tree);
    my->curr = aTreeNode;
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTreeCursor, next)
    FcTreeNode *aTreeNode = TreeSuccessor(my->tree, my->curr);
    my->curr = aTreeNode;
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTreeCursor, previous)
    FcTreeNode *aTreeNode = TreePredecessor(my->tree, my->curr);
    my->curr = aTreeNode;
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTreeCursor, current)
    // this is equivalent to cc_null if my->curr points to sentinel
    return my->curr->item;
cc_end_method


cc_class_object(FcTreeCursor)

cc_class(FcTreeCursor,
    cc_method("findEqual",          findEqualFcTreeCursor),
    cc_method("findGreater",        findGreaterFcTreeCursor),
    cc_method("findLesser",         findLesserFcTreeCursor),
    cc_method("findLesserOrEqual",  findLesserOrEqualFcTreeCursor),
    cc_method("findGreaterOrEqual", findGreaterOrEqualFcTreeCursor),
    cc_method("first",              firstFcTreeCursor),
    cc_method("last",               lastFcTreeCursor),
    cc_method("next",               nextFcTreeCursor),
    cc_method("previous",           previousFcTreeCursor),
    cc_method("current",            currentFcTreeCursor),
    )

#if 0

//  cc_method("prefix",     prefixFcTreeCursor),
//  cc_method("affix",      affixFcTreeCursor),

    cc_method("remove",     removeFcTreeCursor),
    )
#endif
