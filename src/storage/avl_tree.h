#pragma once

#include <stddef.h> // NULL
#include <stdint.h> // uint32_t

struct AVLNode {
    AVLNode *parent = NULL; // added
    AVLNode *left = NULL; // left child
    AVLNode *right = NULL; // right child
    uint32_t height = 0; // height of the node
    uint32_t cnt = 0; // number of nodes in the subtree
};

inline void avl_init(AVLNode *node) {
    node->left = node->right = node->parent = NULL;
    node->height = 1;
    node->cnt = 1;
}

// helpers
inline uint32_t avl_height(AVLNode *node) { return node ? node->height : 0; }
inline uint32_t avl_cnt(AVLNode *node) { return node ? node->cnt : 0; }

// functions
AVLNode *avl_fix_tree(AVLNode *node);
AVLNode *avl_delete(AVLNode *node);

// Insertion and deletion helpers
void avl_search_and_insert(AVLNode **root, AVLNode *new_node, bool (*less)(AVLNode *, AVLNode *));
AVLNode *avl_search_and_delete(AVLNode **root, void *key, bool (*cmp)(AVLNode *, void *));