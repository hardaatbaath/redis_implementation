#pragma once

#include "avl_tree.h"
#include "hashtable.h"
#include "../core/buffer_io.h"

// C++ stdlib
#include <string>
#include <vector>

// Set used for storing the AVL tree and hash table
struct ZSet {
    AVLNode *root = NULL;   // index by (score, name)
    HMap hmap;              // index by name
};

// Node used for storing the (score, name) tuple
struct ZNode {
    AVLNode tree;
    HNode   hmap;
    double  score = 0;
    size_t  len = 0;
    char    name[0];        // flexible array
};

// Helper structure for the hashtable lookup
struct HashKey {
    HNode node;
    const char *name = NULL;
    size_t len = 0;
};

// Empty zset used for non-existent keys
static const ZSet k_empty_zset = {};

ZNode *zset_lookup(ZSet *zset, const char *name, size_t len);
bool   zset_insert(ZSet *zset, const char *name, size_t len, double score);
void   zset_delete(ZSet *zset, ZNode *node);
ZNode *zset_seek_greater_equal(ZSet *zset, double score, const char *name, size_t len);
void   zset_clear(ZSet *zset);
ZNode *znode_offset(ZNode *node, int64_t offset);

// Z-set command handlers (operate on top-level HMap `db`)
void zcmd_add(std::vector<std::string> &cmd, Buffer &resp);
void zcmd_remove(std::vector<std::string> &cmd, Buffer &resp);
void zcmd_score(std::vector<std::string> &cmd, Buffer &resp);
void zcmd_query(std::vector<std::string> &cmd, Buffer &resp);
