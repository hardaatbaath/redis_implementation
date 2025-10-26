#include <assert.h> // assert
#include <stdint.h> // uint32_t

#include "avl_tree.h"

// Helper function to find the maximum of two uint32_t values
static uint32_t max(uint32_t lhs, uint32_t rhs) {
    return lhs > rhs ? lhs : rhs;
}

// Update the height and count of the node
static void avl_update_stats (AVLNode *node) {
    node->height = 1 + max(avl_height(node->left), avl_height(node->right));
    node->cnt = 1 + avl_cnt(node->left) + avl_cnt(node->right);
}

// Left rotation
static AVLNode *rot_left(AVLNode *node) {
    AVLNode *parent = node->parent;
    AVLNode *new_node = node->right;
    AVLNode *inner = new_node->left;
    
    // node <-> inner
    node->right = inner;
    if (inner) { inner->parent = node; } // if inner is not NULL, set its parent to node
    
    // parent <- new_node
    new_node->parent = parent;
    
    // new_node <-> node
    new_node->left = node;
    node->parent = new_node;
    
    // auxiliary data
    avl_update_stats(node);
    avl_update_stats(new_node);
    return new_node;
}

// Right rotation
static AVLNode *rot_right(AVLNode *node) {
    AVLNode *parent = node->parent;
    AVLNode *new_node = node->left;
    AVLNode *inner = new_node->right;

    // node <-> inner
    node->left = inner;
    if (inner) { inner->parent = node; } // if inner is not NULL, set its parent to node

    // parent <- new_node
    new_node->parent = parent;
    
    // new_node <-> node
    new_node->right = node;
    node->parent = new_node;
    
    // auxiliary data
    avl_update_stats(node);
    avl_update_stats(new_node);
    return new_node;
}

// the left subtree is taller by 2
static AVLNode *avl_fix_left(AVLNode *node) {
    if (avl_height(node->left->left) < avl_height(node->left->right)) {
        node->left = rot_left(node->left);  // Transformation 2
    }
    return rot_right(node);                 // Transformation 1
}

// the right subtree is taller by 2
static AVLNode *avl_fix_right(AVLNode *node) {
    if (avl_height(node->right->right) < avl_height(node->right->left)) {
        node->right = rot_right(node->right);  // Transformation 1
    }
    return rot_left(node);                     // Transformation 2
}

// Fix imbalanced nodes and maintain invariants until the root is reached, loops until the root is reached
AVLNode *avl_fix_tree(AVLNode *node) {
    while (true) {
        AVLNode **from = &node; // save the fixed subtree here
        AVLNode *parent = node->parent;

        if (parent) {
            // attach the fixed subtree to the parent
            from = parent->left == node ? &parent->left : &parent->right;
        }   // else: save to the local variable `node`

        // auxiliary data
        avl_update_stats(node);

        // Get the height of the left and right subtrees
        uint32_t left_height = avl_height(node->left);
        uint32_t right_height = avl_height(node->right);
        
        // Fix the height difference of 2
        if (left_height == right_height + 2) { *from = avl_fix_left(node); } // fix the left subtree
        else if (left_height + 2 == right_height) { *from = avl_fix_right(node); } // else: fix the right subtree

        // This was the root node, stop
        if (!parent) {
            return *from;
        }

        // Continue to the parent node because its height may be changed
        node = parent;
    }
}

// detach a node where 1 of its children is empty
static AVLNode *avl_delete_one_child(AVLNode *node) {
    assert(!node->left || !node->right);    // At most 1 child
    AVLNode *child = node->left ? node->left : node->right; // can be NULL
    AVLNode *parent = node->parent;

    // update the child's parent pointer
    if (child) { child->parent = parent; } // can be NULL 
    
    // Attach the child to the grandparent
    if (!parent) { return child; } // removing the root node

    // Attach the child to the grandparent, and save the pointer to the parent's child
    AVLNode **from = parent->left == node ? &parent->left : &parent->right;
    *from = child;
    
    // Rebalance the updated tree
    return avl_fix_tree(parent);
}

// detach a node and returns the new root of the tree
AVLNode *avl_delete(AVLNode *node) {
    // The easy case of 0 or 1 child
    if (!node->left || !node->right) { return avl_delete_one_child(node); }
    
    // Find the successor
    AVLNode *victim = node->right;
    while (victim->left) { victim = victim->left; }
    
    // Detach the successor
    AVLNode *root = avl_delete_one_child(victim);
    
    // Swap with the successor
    *victim = *node;    // left, right, parent
    if (victim->left) { victim->left->parent = victim; }
    if (victim->right) { victim->right->parent = victim; }
    
    // Attach the successor to the parent, or update the root pointer
    AVLNode **from = &root;
    AVLNode *parent = node->parent;
    if (parent) { from = parent->left == node ? &parent->left : &parent->right; }
    
    // Attach the successor to the parent, or update the root pointer
    *from = victim;
    return root;
}