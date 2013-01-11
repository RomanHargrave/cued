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


// FC_TREE_NODE_BLACK must be zero unless code is added to initialize
// the sentinel's color
//
#define FC_TREE_NODE_BLACK 0
#define FC_TREE_NODE_RED   1


cc_begin_method(FcTree, init)
    cc_msg_super("init");
    FcCheckArgcRange(0, 1);
    my->cmpMethod = argc ? (FcCompare) as_ptr(argv[0]) : FcObjCompare;
    my->root = &my->sentinel;
    return by_obj(my);
cc_end_method


cc_begin_method(FcTree, isEmpty)
    int rc;
    rc = (&my->sentinel == my->root) ? 1 : 0;
    return by_int(rc);
cc_end_method


static void FcTreeWalk(cc_vars_FcTree *tree, FcTreeNode *node, FcEmptyHow how)
{
    if (&tree->sentinel == node)
        return;

    FcTreeWalk(tree, node->left,  how);
    FcTreeWalk(tree, node->right, how);

    switch (how) {
        case FcEmptyNone:
            break;
        case FcEmptyFreeObject:
            cc_msg(as_obj(node->item), "free");
            break;
        case FcEmptyFreePointer:
            free(as_ptr(node->item));
            break;
    }
    free(node);
}


cc_begin_method(FcTree, empty)
    FcEmptyHow how = FcEmptyNone;
    if (1 == argc) {
        how = (FcEmptyHow) as_int(argv[0]);
    }
    FcTreeWalk(my, my->root, how);
    return cc_msg(my, "init");
cc_end_method


// TODO:  common function?
cc_begin_method(FcTree, free)
    _cc_send(my, "empty", argc, argv);
    return cc_msg_super("free");
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
    FcTreeNode *aTreeNode;
    FcCheckArgc(1);
    aTreeNode = TreeFindEqual(my, argv[0]);
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTree, findGreater)
    FcTreeNode *aTreeNode;
    FcCheckArgc(1);
    aTreeNode = TreeFindGreater(my, argv[0]);
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTree, findLesser)
    FcTreeNode *aTreeNode;
    FcCheckArgc(1);
    aTreeNode = TreeFindLesser(my, argv[0]);
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTree, findLesserOrEqual)
    FcTreeNode *aTreeNode;
    FcCheckArgc(1);
    aTreeNode = TreeFindLesserOrEqual(my, argv[0]);
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTree, findGreaterOrEqual)
    FcTreeNode *aTreeNode;
    FcCheckArgc(1);
    aTreeNode = TreeFindGreaterOrEqual(my, argv[0]);
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTree, cursor)
    return cc_msg(&FcTreeCursor, "new", by_obj(my));
cc_end_method


static void TreeLeftRotate(cc_vars_FcTree *theTree, FcTreeNode *theSubTree)
{
    FcTreeNode *aRightChild = theSubTree->right;

    theSubTree->right = aRightChild->left;

    if (aRightChild->left != &theTree->sentinel)
        aRightChild->left->parent = theSubTree;

    aRightChild->parent = theSubTree->parent;
    if (&theTree->sentinel == theSubTree->parent) {
        theTree->root = aRightChild;
    } else if (theSubTree == theSubTree->parent->left) {
        theSubTree->parent->left  = aRightChild;
    } else {
        theSubTree->parent->right = aRightChild;
    }
    aRightChild->left = theSubTree;
    theSubTree->parent = aRightChild;
}


static void TreeRightRotate(cc_vars_FcTree *theTree, FcTreeNode *theSubTree)
{
    FcTreeNode *aLeftChild = theSubTree->left;

    theSubTree->left = aLeftChild->right;

    if (aLeftChild->right != &theTree->sentinel)
        aLeftChild->right->parent = theSubTree;

    aLeftChild->parent = theSubTree->parent;
    if (&theTree->sentinel == theSubTree->parent) {
        theTree->root = aLeftChild;
    } else if (theSubTree == theSubTree->parent->right) {
        theSubTree->parent->right = aLeftChild;
    } else {
        theSubTree->parent->left  = aLeftChild;
    }
    aLeftChild->right = theSubTree;
    theSubTree->parent = aLeftChild;
}


static inline FcTreeNode *TreeUnbalancedInsert
    (
    cc_vars_FcTree *theTree,
    cc_arg_t theItem,
    FcTreeNode *theNewSubTree
    )
{
    int aCmpResult;
    FcTreeNode *aSubTree = theTree->root;

    if (&theTree->sentinel == aSubTree) {
        theTree->root = theNewSubTree;
    } else {
        for (;;) {
            aCmpResult = (theTree->cmpMethod)(theItem, aSubTree->item);
            if (aCmpResult > 0) {
                if (&theTree->sentinel == aSubTree->right) {
                    aSubTree->right = theNewSubTree;
                    break;
                } else {
                    aSubTree = aSubTree->right;
                }
            } else if (aCmpResult < 0) {
                if (&theTree->sentinel == aSubTree->left) {
                    aSubTree->left = theNewSubTree;
                    break;
                } else {
                    aSubTree = aSubTree->left;
                }
            } else {
                // found
                return aSubTree;
            }
        }
    }

    theNewSubTree->item   = theItem;
    theNewSubTree->left   = &theTree->sentinel;
    theNewSubTree->right  = &theTree->sentinel;
    theNewSubTree->parent = aSubTree;

    return theNewSubTree;    
}


static inline FcTreeNode *TreeInsert(cc_vars_FcTree *theTree, cc_arg_t theItem, FcTreeNode *theNewSubTree)
{
    FcTreeNode *anUncle;

    FcTreeNode *aSubTree = TreeUnbalancedInsert(theTree, theItem, theNewSubTree);
    if (aSubTree != theNewSubTree) {
        return aSubTree;
    }

    aSubTree->color = FC_TREE_NODE_RED;
    while (aSubTree != theTree->root && FC_TREE_NODE_RED == aSubTree->parent->color) {
        if (aSubTree->parent == aSubTree->parent->parent->left) {
            anUncle = aSubTree->parent->parent->right;
            if (FC_TREE_NODE_RED == anUncle->color) {
                aSubTree->parent->color         = FC_TREE_NODE_BLACK;
                anUncle->color                  = FC_TREE_NODE_BLACK;
                aSubTree->parent->parent->color = FC_TREE_NODE_RED;
                aSubTree = aSubTree->parent->parent;
            } else {
                if (aSubTree == aSubTree->parent->right) {
                    aSubTree = aSubTree->parent;
                    TreeLeftRotate(theTree, aSubTree);
                }               
                aSubTree->parent->color         = FC_TREE_NODE_BLACK;
                aSubTree->parent->parent->color = FC_TREE_NODE_RED;
                TreeRightRotate(theTree, aSubTree->parent->parent);
            }
        } else {
            anUncle = aSubTree->parent->parent->left;
            if (FC_TREE_NODE_RED == anUncle->color) {
                aSubTree->parent->color         = FC_TREE_NODE_BLACK;
                anUncle->color                  = FC_TREE_NODE_BLACK;
                aSubTree->parent->parent->color = FC_TREE_NODE_RED;
                aSubTree = aSubTree->parent->parent;
            } else {
                if (aSubTree == aSubTree->parent->left) {
                    aSubTree = aSubTree->parent;
                    TreeRightRotate(theTree, aSubTree);
                }               
                aSubTree->parent->color         = FC_TREE_NODE_BLACK;
                aSubTree->parent->parent->color = FC_TREE_NODE_RED;
                TreeLeftRotate(theTree, aSubTree->parent->parent);
            }           
        }
    }
    theTree->root->color = FC_TREE_NODE_BLACK;

    return theNewSubTree;
}


cc_begin_method(FcTree, insert)
    FcTreeNode *aNewSubTree, *aSubTree;
    FcCheckArgc(1);

    aNewSubTree = (FcTreeNode *) malloc(sizeof(FcTreeNode));
    if (!aNewSubTree) {
        return cc_msg(my, "error", by_str("out of memory allocating list node"));
    }

    aSubTree = TreeInsert(my, argv[0], aNewSubTree);
    if (aSubTree != aNewSubTree) {
        free(aNewSubTree);
    }

    return aSubTree->item;
cc_end_method


static inline void TreeRemoveFixup(cc_vars_FcTree *theTree, FcTreeNode *theSubTree)
{
    FcTreeNode *aSibling;

    while (theSubTree != theTree->root && FC_TREE_NODE_BLACK == theSubTree->color) {
        if (theSubTree == theSubTree->parent->left) {
            aSibling = theSubTree->parent->right;
            if (FC_TREE_NODE_RED == aSibling->color) {
                aSibling->color             = FC_TREE_NODE_BLACK;
                theSubTree->parent->color   = FC_TREE_NODE_RED;
                TreeLeftRotate(theTree, theSubTree->parent);
                aSibling = theSubTree->parent->right;
            }
            if (   FC_TREE_NODE_BLACK == aSibling->left->color
                && FC_TREE_NODE_BLACK == aSibling->right->color)
            {
                aSibling->color = FC_TREE_NODE_RED;
                theSubTree = theSubTree->parent;
            } else {
                if (FC_TREE_NODE_BLACK == aSibling->right->color) {
                    aSibling->left->color   = FC_TREE_NODE_BLACK;
                    aSibling->color         = FC_TREE_NODE_RED;
                    TreeRightRotate(theTree, aSibling);
                    aSibling = theSubTree->parent->right;
                }
                aSibling->color = theSubTree->parent->color;
                theSubTree->parent->color   = FC_TREE_NODE_BLACK;
                aSibling->right->color      = FC_TREE_NODE_BLACK;
                TreeLeftRotate(theTree, theSubTree->parent);

                break;
            }
        }
        else
        {
            aSibling = theSubTree->parent->left;
            if (FC_TREE_NODE_RED == aSibling->color) {
                aSibling->color             = FC_TREE_NODE_BLACK;
                theSubTree->parent->color   = FC_TREE_NODE_RED;
                TreeRightRotate(theTree, theSubTree->parent);
                aSibling = theSubTree->parent->left;
            }
            if (   FC_TREE_NODE_BLACK == aSibling->right->color 
                && FC_TREE_NODE_BLACK == aSibling->left->color)
            {
                aSibling->color = FC_TREE_NODE_RED;
                theSubTree = theSubTree->parent;
            } else {
                if (FC_TREE_NODE_BLACK == aSibling->left->color) {
                    aSibling->right->color  = FC_TREE_NODE_BLACK;
                    aSibling->color         = FC_TREE_NODE_RED;
                    TreeLeftRotate(theTree, aSibling);
                    aSibling = theSubTree->parent->left;
                }
                aSibling->color = theSubTree->parent->color;
                theSubTree->parent->color   = FC_TREE_NODE_BLACK;
                aSibling->left->color       = FC_TREE_NODE_BLACK;
                TreeRightRotate(theTree, theSubTree->parent);

                break;
            }
        }
    }
    theSubTree->color = FC_TREE_NODE_BLACK;
}


FcTreeNode *TreeRemoveNode(cc_vars_FcTree *theTree, FcTreeNode *theSubTree)
{
    FcTreeNode *aChild, *aRemovedNode;

    if (&theTree->sentinel == theSubTree->left) {
        //
        // promote right child
        //
        aRemovedNode = theSubTree;
        aChild = aRemovedNode->right;
    } else if (&theTree->sentinel == theSubTree->right) {
        //
        // promote left child
        //
        aRemovedNode = theSubTree;
        aChild = aRemovedNode->left;
    } else {
        //
        // the node containing the deleted item has two children;
        // its successor will be in the deleted item's subtree and
        // lack a left child (because the deleted item would be the
        // successor's left child, if it were not higher in the tree);
        // remove the successor's node and replace the deleted item
        // with its successor
        //
        aRemovedNode = theSubTree->right;
        while (aRemovedNode->left != &theTree->sentinel) {
            aRemovedNode = aRemovedNode->left;
        }
        theSubTree->item = aRemovedNode->item;
        aChild = aRemovedNode->right;
    }

    aChild->parent = aRemovedNode->parent;
    if (aRemovedNode->parent == &theTree->sentinel) {
        theTree->root = aChild;
    } else if (aRemovedNode->parent->left == aRemovedNode) {
        aRemovedNode->parent->left  = aChild;
    } else {
        aRemovedNode->parent->right = aChild;
    }

    if (FC_TREE_NODE_BLACK == aRemovedNode->color) {
        TreeRemoveFixup(theTree, aChild);
    }

    return aRemovedNode;
}


cc_begin_method(FcTree, remove)
    cc_arg_t rc;
    FcTreeNode *aTreeNode;
    FcCheckArgc(1);
    aTreeNode = TreeFindEqual(my, argv[0]);
    rc = aTreeNode->item;
    if (&my->sentinel != aTreeNode) {
        //
        // an optimization removes the successor rather than replacing it;
        // consequently, a different node may be removed from the tree
        //
        aTreeNode = TreeRemoveNode(my, aTreeNode);
        free(aTreeNode);
    }
    return rc;
cc_end_method


cc_class_object(FcTree)

cc_class(FcTree,
    cc_method("init",               initFcTree),
    cc_method("isEmpty",            isEmptyFcTree),
    cc_method("empty",              emptyFcTree),
    cc_method("free",               freeFcTree),

    // should list gain an insert/remove that do a comparison?
    cc_method("insert",             insertFcTree),

    // prefix and affix also call insert in order to align with list
    cc_method("prefix",             insertFcTree),
    cc_method("affix",              insertFcTree),

    // should list gain a findEqual that does a comparison?
    cc_method("findEqual",          findEqualFcTree),
    cc_method("findGreater",        findGreaterFcTree),
    cc_method("findLesser",         findLesserFcTree),
    cc_method("findLesserOrEqual",  findLesserOrEqualFcTree),
    cc_method("findGreaterOrEqual", findGreaterOrEqualFcTree),
    cc_method("cursor",             cursorFcTree),
    cc_method("remove",             removeFcTree),

    // list should have a free?  should this do other things than free?  take a method?
    //cc_method("delete",             deleteFcTree),
    )


cc_begin_method(FcTreeCursor, findEqual)
    FcTreeNode *aTreeNode;
    FcCheckArgc(1);
    aTreeNode = TreeFindEqual(my->tree, argv[0]);
    my->curr = aTreeNode;
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTreeCursor, findGreater)
    FcTreeNode *aTreeNode;
    FcCheckArgc(1);
    aTreeNode = TreeFindGreater(my->tree, argv[0]);
    my->curr = aTreeNode;
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTreeCursor, findLesser)
    FcTreeNode *aTreeNode;
    FcCheckArgc(1);
    aTreeNode = TreeFindLesser(my->tree, argv[0]);
    my->curr = aTreeNode;
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTreeCursor, findLesserOrEqual)
    FcTreeNode *aTreeNode;
    FcCheckArgc(1);
    aTreeNode = TreeFindLesserOrEqual(my->tree, argv[0]);
    my->curr = aTreeNode;
    return aTreeNode->item;
cc_end_method


cc_begin_method(FcTreeCursor, findGreaterOrEqual)
    FcTreeNode *aTreeNode;
    FcCheckArgc(1);
    aTreeNode = TreeFindGreaterOrEqual(my->tree, argv[0]);
    my->curr = aTreeNode;
    return aTreeNode->item;
cc_end_method


static inline FcTreeNode *TreeMinimum(cc_vars_FcTree *theTree)
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


static inline FcTreeNode *TreeMaximum(cc_vars_FcTree *theTree)
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


static inline FcTreeNode *TreeSuccessor(cc_vars_FcTree *theTree, FcTreeNode *theTreeNode)
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


static inline FcTreeNode *TreePredecessor(cc_vars_FcTree *theTree, FcTreeNode *theTreeNode)
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
    // this is equivalent to cc_null when my->curr points to sentinel
    return my->curr->item;
cc_end_method


cc_begin_method(FcTreeCursor, init)
    cc_msg_super("init");
    FcCheckArgc(1);
    my->tree = (cc_vars_FcTree *) as_obj(argv[0]);
    my->curr = &my->tree->sentinel;
    return by_obj(my);
cc_end_method


cc_class_object(FcTreeCursor)

cc_class(FcTreeCursor,
    cc_method("init",               initFcTreeCursor),
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

    //  what do you set the cursor to after a remove?  next, previous?
    //cc_method("remove",             removeFcTreeCursor),

    //  do we need these to align with list?
    //cc_method("prefix",     prefixFcTreeCursor),
    //cc_method("affix",      affixFcTreeCursor),
    )
