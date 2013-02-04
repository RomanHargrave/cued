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

#include <stdio.h>
#include <limits.h> // INT_MAX


// FC_TREE_NODE_BLACK must be zero unless code is added to initialize
// the sentinel's color
//
#define FC_TREE_NODE_BLACK 0
#define FC_TREE_NODE_RED   1


cc_begin_method(FcTree, init)
    cc_msg_super0("init");
    FcCheckArgcRange(0, 1);
    my->cmpMethod = argc ? (FcCompareFn) as_ptr(argv[0]) : FcObjCompare;
    my->root = &my->sentinel;
    return by_obj(my);
cc_end_method


cc_begin_method(FcTree, isEmpty)
    int rc;
    rc = (&my->sentinel == my->root) ? 1 : 0;
    return by_int(rc);
cc_end_method


static void FcTreeEmpty(cc_vars_FcTree *tree, FcTreeNode *node, FcEmptyHow how)
{
    if (&tree->sentinel == node)
        return;

    FcTreeEmpty(tree, node->left,  how);
    FcTreeEmpty(tree, node->right, how);

    switch (how) {
        case FcEmptyNone:
            break;
        case FcEmptyFreeObject:
            cc_msg0(as_obj(node->item), "free");
            break;
        case FcEmptyFreePointer:
            free(as_ptr(node->item));
            break;
    }
    free(node);
}


cc_begin_method(FcTree, empty)
    FcEmptyHow how;
    FcCheckArgcRange(0, 1);
    how = argc ? (FcEmptyHow) as_int(argv[0]) : FcEmptyNone;
    FcTreeEmpty(my, my->root, how);
    return cc_msg0(my, "init");
cc_end_method


static void FcTreeApply(cc_vars_FcTree *tree, FcTreeNode *node, FcApplyFn applyFn, const char *msg, int argc, cc_arg_t *argv)
{
    if (&tree->sentinel == node)
        return;

    FcTreeApply(tree, node->left,  applyFn, msg, argc, argv);
    if (msg) {
        _cc_send(as_obj(node->item),        msg, argc, argv);
    } else {
        applyFn(        node->item,              argc, argv);
    }

    // what happens if apply deletes the node?  are we better off if we save node->right
    // before calling apply for the node?  what happens when we delete the successor
    // rather than the node itself?   node->right could be the successor;
    // for now, disallow removing nodes during an apply;  if it's really necessary,
    // get rid of the optimization where we delete the successor, by instead replacing
    // the deleted node by the successor?  but then what about rotations during a remove?
    //
    FcTreeApply(tree, node->right, applyFn, msg, argc, argv);
}


cc_begin_method(FcTree, apply)
    FcCheckArgcRange(1, INT_MAX);
    FcTreeApply(my, my->root,
                is_ptr(argv[0]) ? (FcApplyFn) as_ptr(argv[0]) : NULL,
                is_str(argv[0]) ?             as_str(argv[0]) : NULL,
                argc - 1, &argv[1]);
    return by_obj(my);
cc_end_method


static FcTreeNode *TreeFindEqual(cc_vars_FcTree *tree, cc_arg_t key)
{
    int cmpResult;

    FcTreeNode *node = tree->root;
    while (node != &tree->sentinel) {
        cmpResult = (tree->cmpMethod)(node->item, key);
        if (cmpResult < 0) {
            node = node->right;
        } else if (cmpResult > 0) {
            node = node->left;
        } else {
            // found
            break;
        }
    }

    return node;
}


static FcTreeNode *TreeFindGreater(cc_vars_FcTree *tree, cc_arg_t key)
{
    int cmpResult;
    FcTreeNode *greaterNode = &tree->sentinel;

    FcTreeNode *node = tree->root;
    while (node != &tree->sentinel) {
        cmpResult = (tree->cmpMethod)(node->item, key);
        if (cmpResult < 0) {
            node = node->right;
        } else if (cmpResult > 0) {
            greaterNode = node;
            node = node->left;
        } else {
            // if the data items are equal, check for a greater descendant
            // tree;  if there is a greater descendant tree, find its 
            // least member
            //
            if (node->right != &tree->sentinel) {
                greaterNode = node->right;
                while (greaterNode->left != &tree->sentinel)
                    greaterNode = greaterNode->left;
            }
            break;
        }
    }

    return greaterNode;
}


static FcTreeNode *TreeFindLesser(cc_vars_FcTree *tree, cc_arg_t key)
{
    int cmpResult;
    FcTreeNode *lesserNode = &tree->sentinel;

    FcTreeNode *node = tree->root;
    while (node != &tree->sentinel) {
        cmpResult = (tree->cmpMethod)(node->item, key);
        if (cmpResult < 0) {
            lesserNode = node;
            node = node->right;
        } else if (cmpResult > 0) {
            node = node->left;
        } else {
            if (node->left != &tree->sentinel) {
                lesserNode = node->left;
                while (lesserNode->right != &tree->sentinel)
                    lesserNode = lesserNode->right;
            }
            break;
        }
    }

    return lesserNode;
}


static FcTreeNode *TreeFindLesserOrEqual(cc_vars_FcTree *tree, cc_arg_t key)
{
    int cmpResult;
    FcTreeNode *lesserNode = &tree->sentinel;

    FcTreeNode *node = tree->root;
    while (node != &tree->sentinel) {
        cmpResult = (tree->cmpMethod)(node->item, key);
        if (cmpResult < 0) {
            lesserNode = node;
            node = node->right;
        } else if (cmpResult > 0) {
            node = node->left;
        } else {
            return node;
        }
    }

    return lesserNode;
}


static FcTreeNode *TreeFindGreaterOrEqual(cc_vars_FcTree *tree, cc_arg_t key)
{
    int cmpResult;
    FcTreeNode *greaterNode = &tree->sentinel;

    FcTreeNode *node = tree->root;
    while (node != &tree->sentinel) {
        cmpResult = (tree->cmpMethod)(node->item, key);
        if (cmpResult < 0) {
            node = node->right;
        } else if (cmpResult > 0) {
            greaterNode = node;
            node = node->left;
        } else {
            return node;
        }
    }

    return greaterNode;
}


cc_begin_method(FcTree, findEqual)
    FcTreeNode *node;
    FcCheckArgc(1);
    node = TreeFindEqual(my, argv[0]);
    return node->item;
cc_end_method


cc_begin_method(FcTree, findGreater)
    FcTreeNode *node;
    FcCheckArgc(1);
    node = TreeFindGreater(my, argv[0]);
    return node->item;
cc_end_method


cc_begin_method(FcTree, findLesser)
    FcTreeNode *node;
    FcCheckArgc(1);
    node = TreeFindLesser(my, argv[0]);
    return node->item;
cc_end_method


cc_begin_method(FcTree, findLesserOrEqual)
    FcTreeNode *node;
    FcCheckArgc(1);
    node = TreeFindLesserOrEqual(my, argv[0]);
    return node->item;
cc_end_method


cc_begin_method(FcTree, findGreaterOrEqual)
    FcTreeNode *node;
    FcCheckArgc(1);
    node = TreeFindGreaterOrEqual(my, argv[0]);
    return node->item;
cc_end_method


cc_begin_method(FcTree, cursor)
    return cc_msg(&FcTreeCursor, "new", by_obj(my));
cc_end_method


static void TreeLeftRotate(cc_vars_FcTree *tree, FcTreeNode *subtree)
{
    FcTreeNode *rightChild = subtree->right;

    subtree->right = rightChild->left;

    if (rightChild->left != &tree->sentinel)
        rightChild->left->parent = subtree;

    rightChild->parent = subtree->parent;
    if (&tree->sentinel == subtree->parent) {
        tree->root = rightChild;
    } else if (subtree == subtree->parent->left) {
        subtree->parent->left  = rightChild;
    } else {
        subtree->parent->right = rightChild;
    }
    rightChild->left = subtree;
    subtree->parent = rightChild;
}


static void TreeRightRotate(cc_vars_FcTree *tree, FcTreeNode *subtree)
{
    FcTreeNode *leftChild = subtree->left;

    subtree->left = leftChild->right;

    if (leftChild->right != &tree->sentinel)
        leftChild->right->parent = subtree;

    leftChild->parent = subtree->parent;
    if (&tree->sentinel == subtree->parent) {
        tree->root = leftChild;
    } else if (subtree == subtree->parent->right) {
        subtree->parent->right = leftChild;
    } else {
        subtree->parent->left  = leftChild;
    }
    leftChild->right = subtree;
    subtree->parent = leftChild;
}


static inline FcTreeNode *TreeUnbalancedInsert
    (
    cc_vars_FcTree *tree,
    cc_arg_t item,
    FcTreeNode *newSubtree
    )
{
    int cmpResult;
    FcTreeNode *subtree = tree->root;

    if (&tree->sentinel == subtree) {
        tree->root = newSubtree;
    } else {
        for (;;) {
            cmpResult = (tree->cmpMethod)(item, subtree->item);
            if (cmpResult > 0) {
                if (&tree->sentinel == subtree->right) {
                    subtree->right = newSubtree;
                    break;
                } else {
                    subtree = subtree->right;
                }
            } else if (cmpResult < 0) {
                if (&tree->sentinel == subtree->left) {
                    subtree->left = newSubtree;
                    break;
                } else {
                    subtree = subtree->left;
                }
            } else {
                // found
                return subtree;
            }
        }
    }

    newSubtree->item   = item;
    newSubtree->left   = &tree->sentinel;
    newSubtree->right  = &tree->sentinel;
    newSubtree->parent = subtree;

    return newSubtree;    
}


static inline FcTreeNode *TreeInsert(cc_vars_FcTree *tree, cc_arg_t item, FcTreeNode *newSubtree)
{
    FcTreeNode *uncle;

    FcTreeNode *subtree = TreeUnbalancedInsert(tree, item, newSubtree);
    if (subtree != newSubtree) {
        return subtree;
    }

    subtree->color = FC_TREE_NODE_RED;
    while (subtree != tree->root && FC_TREE_NODE_RED == subtree->parent->color) {
        if (subtree->parent == subtree->parent->parent->left) {
            uncle = subtree->parent->parent->right;
            if (FC_TREE_NODE_RED == uncle->color) {
                subtree->parent->color         = FC_TREE_NODE_BLACK;
                uncle->color                   = FC_TREE_NODE_BLACK;
                subtree->parent->parent->color = FC_TREE_NODE_RED;
                subtree = subtree->parent->parent;
            } else {
                if (subtree == subtree->parent->right) {
                    subtree = subtree->parent;
                    TreeLeftRotate(tree, subtree);
                }               
                subtree->parent->color         = FC_TREE_NODE_BLACK;
                subtree->parent->parent->color = FC_TREE_NODE_RED;
                TreeRightRotate(tree, subtree->parent->parent);
            }
        } else {
            uncle = subtree->parent->parent->left;
            if (FC_TREE_NODE_RED == uncle->color) {
                subtree->parent->color         = FC_TREE_NODE_BLACK;
                uncle->color                   = FC_TREE_NODE_BLACK;
                subtree->parent->parent->color = FC_TREE_NODE_RED;
                subtree = subtree->parent->parent;
            } else {
                if (subtree == subtree->parent->left) {
                    subtree = subtree->parent;
                    TreeRightRotate(tree, subtree);
                }               
                subtree->parent->color         = FC_TREE_NODE_BLACK;
                subtree->parent->parent->color = FC_TREE_NODE_RED;
                TreeLeftRotate(tree, subtree->parent->parent);
            }           
        }
    }
    tree->root->color = FC_TREE_NODE_BLACK;

    return newSubtree;
}


cc_begin_method(FcTree, insert)
    FcTreeNode *newSubtree;
    FcTreeNode *subtree = &my->sentinel;
    for (int i = 0;  i < argc;  ++i) {
        newSubtree = (FcTreeNode *) malloc(sizeof(FcTreeNode));
        if (!newSubtree) {
            return cc_msg(my, "error", by_str("out of memory allocating tree node"));
        }

        subtree = TreeInsert(my, argv[i], newSubtree);
        if (subtree != newSubtree) {
            free(newSubtree);
        }
    }
    return subtree->item;
cc_end_method


static inline void TreeRemoveFixup(cc_vars_FcTree *tree, FcTreeNode *subtree)
{
    FcTreeNode *sibling;

    while (subtree != tree->root && FC_TREE_NODE_BLACK == subtree->color) {
        if (subtree == subtree->parent->left) {
            sibling = subtree->parent->right;
            if (FC_TREE_NODE_RED == sibling->color) {
                sibling->color            = FC_TREE_NODE_BLACK;
                subtree->parent->color    = FC_TREE_NODE_RED;
                TreeLeftRotate(tree, subtree->parent);
                sibling = subtree->parent->right;
            }
            if (   FC_TREE_NODE_BLACK == sibling->left->color
                && FC_TREE_NODE_BLACK == sibling->right->color)
            {
                sibling->color = FC_TREE_NODE_RED;
                subtree = subtree->parent;
            } else {
                if (FC_TREE_NODE_BLACK == sibling->right->color) {
                    sibling->left->color  = FC_TREE_NODE_BLACK;
                    sibling->color        = FC_TREE_NODE_RED;
                    TreeRightRotate(tree, sibling);
                    sibling = subtree->parent->right;
                }
                sibling->color = subtree->parent->color;
                subtree->parent->color    = FC_TREE_NODE_BLACK;
                sibling->right->color     = FC_TREE_NODE_BLACK;
                TreeLeftRotate(tree, subtree->parent);

                break;
            }
        }
        else
        {
            sibling = subtree->parent->left;
            if (FC_TREE_NODE_RED == sibling->color) {
                sibling->color            = FC_TREE_NODE_BLACK;
                subtree->parent->color    = FC_TREE_NODE_RED;
                TreeRightRotate(tree, subtree->parent);
                sibling = subtree->parent->left;
            }
            if (   FC_TREE_NODE_BLACK == sibling->right->color 
                && FC_TREE_NODE_BLACK == sibling->left->color)
            {
                sibling->color = FC_TREE_NODE_RED;
                subtree = subtree->parent;
            } else {
                if (FC_TREE_NODE_BLACK == sibling->left->color) {
                    sibling->right->color = FC_TREE_NODE_BLACK;
                    sibling->color        = FC_TREE_NODE_RED;
                    TreeLeftRotate(tree, sibling);
                    sibling = subtree->parent->left;
                }
                sibling->color = subtree->parent->color;
                subtree->parent->color    = FC_TREE_NODE_BLACK;
                sibling->left->color      = FC_TREE_NODE_BLACK;
                TreeRightRotate(tree, subtree->parent);

                break;
            }
        }
    }
    subtree->color = FC_TREE_NODE_BLACK;
}


FcTreeNode *TreeRemoveNode(cc_vars_FcTree *tree, FcTreeNode *subtree)
{
    FcTreeNode *child, *removedNode;

    if (&tree->sentinel == subtree->left) {
        //
        // promote right child
        //
        removedNode = subtree;
        child = removedNode->right;
    } else if (&tree->sentinel == subtree->right) {
        //
        // promote left child
        //
        removedNode = subtree;
        child = removedNode->left;
    } else {
        //
        // the node containing the deleted item has two children;
        // its successor will be in the deleted item's subtree and
        // lack a left child (because the deleted item would be the
        // successor's left child, if it were not higher in the tree);
        // remove the successor's node and replace the deleted item
        // with its successor
        //
        removedNode = subtree->right;
        while (removedNode->left != &tree->sentinel) {
            removedNode = removedNode->left;
        }
        subtree->item = removedNode->item;
        child = removedNode->right;
    }

    child->parent = removedNode->parent;
    if (removedNode->parent == &tree->sentinel) {
        tree->root = child;
    } else if (removedNode->parent->left == removedNode) {
        removedNode->parent->left  = child;
    } else {
        removedNode->parent->right = child;
    }

    if (FC_TREE_NODE_BLACK == removedNode->color) {
        TreeRemoveFixup(tree, child);
    }

    return removedNode;
}


cc_begin_method(FcTree, remove)
    cc_arg_t rc = cc_null;
    FcTreeNode *node;
    for (int i = 0;  i < argc;  ++i) {
        node = TreeFindEqual(my, argv[i]);
        rc = node->item;
        if (&my->sentinel != node) {
            //
            // an optimization removes the successor rather than replacing it;
            // consequently, a different node may be removed from the tree
            //
            node = TreeRemoveNode(my, node);
            free(node);
        }
    }
    return rc;
cc_end_method


cc_class_object(FcTree)

cc_class(FcTree,
    cc_method("init",               initFcTree),
    cc_method("isEmpty",            isEmptyFcTree),
    cc_method("empty",              emptyFcTree),
    cc_method("free",               FcContainerFree),

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
    cc_method("apply",              applyFcTree),
    )


cc_begin_method(FcTreeCursor, findEqual)
    FcTreeNode *node;
    FcCheckArgc(1);
    node = TreeFindEqual(my->tree, argv[0]);
    my->curr = node;
    return node->item;
cc_end_method


cc_begin_method(FcTreeCursor, findGreater)
    FcTreeNode *node;
    FcCheckArgc(1);
    node = TreeFindGreater(my->tree, argv[0]);
    my->curr = node;
    return node->item;
cc_end_method


cc_begin_method(FcTreeCursor, findLesser)
    FcTreeNode *node;
    FcCheckArgc(1);
    node = TreeFindLesser(my->tree, argv[0]);
    my->curr = node;
    return node->item;
cc_end_method


cc_begin_method(FcTreeCursor, findLesserOrEqual)
    FcTreeNode *node;
    FcCheckArgc(1);
    node = TreeFindLesserOrEqual(my->tree, argv[0]);
    my->curr = node;
    return node->item;
cc_end_method


cc_begin_method(FcTreeCursor, findGreaterOrEqual)
    FcTreeNode *node;
    FcCheckArgc(1);
    node = TreeFindGreaterOrEqual(my->tree, argv[0]);
    my->curr = node;
    return node->item;
cc_end_method


static inline FcTreeNode *TreeMinimum(cc_vars_FcTree *tree)
{
    FcTreeNode *minimumNode = &tree->sentinel;
    FcTreeNode *node = tree->root;

    // find the left-most node of the tree
    //
    while (node != &tree->sentinel) {
        minimumNode = node;
        node = node->left;
    }

    return minimumNode;
}


static inline FcTreeNode *TreeMaximum(cc_vars_FcTree *tree)
{
    FcTreeNode *maximumNode = &tree->sentinel;
    FcTreeNode *node = tree->root;

    // find the right-most node of the tree
    //
    while (node != &tree->sentinel) {
        maximumNode = node;
        node = node->right;
    }

    return maximumNode;
}


static inline FcTreeNode *TreeSuccessor(cc_vars_FcTree *tree, FcTreeNode *node)
{
    if (&tree->sentinel != node) {

        // if the tree has a right subtree,
        // then the successor is in the right subtree
        //
        if (node->right != &tree->sentinel) {
            node = node->right;
            while (node->left != &tree->sentinel)
                node = node->left;

        } else {
            FcTreeNode *ancestor;

            // otherwise, search up the tree until an ancestor right
            // of the child is found (if any)
            //
            while (&tree->sentinel != (ancestor = node->parent)) {
                if (ancestor->left == node)
                    break;

                node = ancestor;
            }

            return ancestor;
        }
    }

    return node;
}


static inline FcTreeNode *TreePredecessor(cc_vars_FcTree *tree, FcTreeNode *node)
{
    if (&tree->sentinel != node) {

        // if the tree has a left subtree,
        // then the predecessor is in the left subtree
        //
        if (node->left != &tree->sentinel) {
            node = node->left;
            while (node->right != &tree->sentinel)
                node = node->right;

        } else {
            FcTreeNode *ancestor;

            // otherwise, search up the tree until an ancestor left
            // of the child is found (if any)
            //
            while (&tree->sentinel != (ancestor = node->parent)) {
                if (ancestor->right == node)
                    break;

                node = ancestor;
            }

            return ancestor;
        }
    }

    return node;
}


cc_begin_method(FcTreeCursor, first)
    FcTreeNode *node = TreeMinimum(my->tree);
    my->curr = node;
    return node->item;
cc_end_method


cc_begin_method(FcTreeCursor, last)
    FcTreeNode *node = TreeMaximum(my->tree);
    my->curr = node;
    return node->item;
cc_end_method


cc_begin_method(FcTreeCursor, next)
    FcTreeNode *node = TreeSuccessor(my->tree, my->curr);
    my->curr = node;
    return node->item;
cc_end_method


cc_begin_method(FcTreeCursor, previous)
    FcTreeNode *node = TreePredecessor(my->tree, my->curr);
    my->curr = node;
    return node->item;
cc_end_method


cc_begin_method(FcTreeCursor, current)
    // this is equivalent to cc_null when my->curr points to sentinel
    return my->curr->item;
cc_end_method


cc_begin_method(FcTreeCursor, init)
    cc_msg_super0("init");
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

    //  what do you set the cursor to after a remove?  next...
    //cc_method("remove",             removeFcTreeCursor),

    //  do we need these to align with list?
    //cc_method("prefix",     prefixFcTreeCursor),
    //cc_method("affix",      affixFcTreeCursor),
    )
