#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

// local
#include "sorted_set.h"          // ZSet, ZNode, zset_*
#include "../core/common.h"      // container_of, string_hash
#include "../net/serialize.h"    // out_*, ERR_*
#include "avl_tree.h"            // avl_init, avl_delete, avl_offset
#include "hashtable.h"           // hm_lookup, hm_insert, hm_delete, hm_clear
#include "commands.h"            // Entry, TYPE_ZSET

// Return the minimum of two values
static size_t min(size_t lhs, size_t rhs) {
    return lhs < rhs ? lhs : rhs;
}

// Create a new ZNode
static ZNode *znode_new(const char *name, size_t len, double score) {
    ZNode *node = (ZNode *)malloc(sizeof(ZNode) + len);
    assert(node);   // not a good idea in real projects

    // Initialize the AVL tree and hash table
    avl_init(&node->tree);
    node->hmap.next = NULL;
    node->hmap.hash_code = string_hash((uint8_t *)name, len);
    node->score = score;       // score of the node
    node->len = len;           // length of the name
    
    // copy the name (len bytes) and add a terminating NUL so it's safe as a C string.
    if (len > 0) { memcpy(&node->name[0], name, len); }
    node->name[len] = '\0';

    return node;
}

// Delete a ZNode
static void znode_del(ZNode *node) {
    free(node);
}

// Compare by the (score, name) tuple
static bool zless(AVLNode *lhs, double score, const char *name, size_t len) {
    // Get the ZNode from the AVLNode
    ZNode *zl = container_of(lhs, ZNode, tree);
    if (zl->score != score) { return zl->score < score; }
    
    // Compare the name
    int rv = memcmp(zl->name, name, min(zl->len, len));
    if (rv != 0) { return rv < 0; }         // return true if the name is less than the other name

    return zl->len < len;      // return true if the length is less than the other length
}

// Compare two AVL nodes directly (wrapper around zless).
static bool zless(AVLNode *lhs, AVLNode *rhs) {
    ZNode *zr = container_of(rhs, ZNode, tree);
    return zless(lhs, zr->score, zr->name, zr->len);
}

// Insert into the AVL tree
static void tree_insert(ZSet *zset, ZNode *node) {
    AVLNode *parent = NULL;         // insert under this node
    AVLNode **from = &zset->root;   // the incoming pointer to the next node
    
    // Tree search for insertion
    while (*from) {                 
        parent = *from;
        from = zless(&node->tree, parent) ? &parent->left : &parent->right;
    }
    
    // Attach the new node to the parent and rebalance the tree
    *from = &node->tree;            
    node->tree.parent = parent;
    zset->root = avl_fix_tree(&node->tree);
}

// update the score of an existing node
static void zset_update(ZSet *zset, ZNode *node, double score) {
    if (node->score == score) { return; }   // If the score is the same, do nothing

    // Detach the tree node
    zset->root = avl_delete(&node->tree);
    avl_init(&node->tree);

    // Update the score and reinsert the tree node
    node->score = score;
    tree_insert(zset, node);
}

// Compare two hash nodes
static bool hcmp(HashNode *node, HashNode *key) {
    ZNode *znode = container_of(node, ZNode, hmap);
    HashKey *hkey = container_of(key, HashKey, node);
    
    // Compare the length
    if (znode->len != hkey->len) { return false; }    // If the length is not the same, return false
    return 0 == memcmp(znode->name, hkey->name, znode->len); // If the name is the same, return true
}

// Lookup by name
ZNode *zset_lookup(ZSet *zset, const char *name, size_t len) {
    if (!zset->root) { return NULL; }   // If the root is NULL, return NULL

    HashKey key;
    key.node.hash_code = string_hash((uint8_t *)name, len);
    key.name = name;
    key.len = len;

    HashNode *found = hm_lookup(&zset->hmap, &key.node, &hcmp);
    return found ? container_of(found, ZNode, hmap) : NULL;
}

// Add a new (score, name) tuple, or update the score of the existing tuple
bool zset_insert(ZSet *zset, const char *name, size_t len, double score) {
    // If the node exists, update the score
    ZNode *node = zset_lookup(zset, name, len);
    if (node) {
        zset_update(zset, node, score);
        return false;
    } 
    
    // Create a new node
    node = znode_new(name, len, score);
    hm_insert(&zset->hmap, &node->hmap);
    tree_insert(zset, node);
    return true;
}

// Delete a node
void zset_delete(ZSet *zset, ZNode *node) {
    // Copy the node details 
    HashKey key;
    key.node.hash_code = node->hmap.hash_code;
    key.name = node->name;
    key.len = node->len;

    // Remove from the hash table
    HashNode *found = hm_delete(&zset->hmap, &key.node, &hcmp);
    assert(found);

    // Remove from the tree
    zset->root = avl_delete(&node->tree);
    
    // Deallocate the node
    znode_del(node);
}

// Find the first (score, name) tuple that is >= key.
ZNode *zset_seek_greater_equal(ZSet *zset, double score, const char *name, size_t len) {
    AVLNode *found = NULL;
    for (AVLNode *node = zset->root; node; ) {
        if (zless(node, score, name, len)) {
            node = node->right; // node < key
        } 
        
        else {
            found = node;       // candidate
            node = node->left;
        }
    }
    return found ? container_of(found, ZNode, tree) : NULL;
}

// Get node offset by N positions in sorted order.
ZNode *znode_offset(ZNode *node, int64_t offset) {
    AVLNode *tnode = node ? avl_offset(&node->tree, offset) : NULL;
    return tnode ? container_of(tnode, ZNode, tree) : NULL;
}

// Recursively delete the AVL tree.
static void tree_dispose(AVLNode *node) {
    if (!node) { return; }
    
    // Recursively delete the AVL tree
    tree_dispose(node->left);
    tree_dispose(node->right);
    znode_del(container_of(node, ZNode, tree));
}

// Destroy all nodes and clear the ZSet.
void zset_clear(ZSet *zset) {
    // Clear the hash table
    hm_clear(&zset->hmap);
    // Clear the AVL tree
    tree_dispose(zset->root);
    // Set the root to NULL
    zset->root = NULL;
}

// Convert string to double
static bool str2dbl(const std::string &s, double &out) {
    char *endp = NULL;
    out = strtod(s.c_str(), &endp);
    return endp == s.c_str() + s.size() && !isnan(out);
}

// Convert string to integer
static bool str2int(const std::string &s, int64_t &out) {
    char *endp = NULL;
    out = strtoll(s.c_str(), &endp, 10);
    return endp == s.c_str() + s.size();
}

// Compare two keys
static bool entry_key_equals(HashNode *node, HashNode *k) {
    Entry *entry = container_of(node, Entry, node);
    LookupKey *lkey = container_of(k, LookupKey, node);
    return entry->key == lkey->key;
}

// Lookup or validate a ZSet entry in the DB.
static ZSet *expect_zset(std::string &s) {
    // Create a new key
    LookupKey key;
    key.key.swap(s);
    key.node.hash_code = string_hash((uint8_t *)key.key.data(), key.key.size());

    // Lookup the key in the hash table
    HashNode *hnode = hm_lookup(&server_data.db, &key.node, &entry_key_equals);
    if (!hnode) { return (ZSet *)&k_empty_zset; } // a non-existent key is treated as an empty zset
    
    // Get the entry from the hash node
    Entry *ent = container_of(hnode, Entry, node);
    return ent->type == TYPE_ZSET ? &ent->zset : NULL;
}

/**
 * Command: ZADD <key> <score> <member>
 * Add or update a member’s score in a ZSet.
 */
void zcmd_add(std::vector<std::string> &cmd, Buffer &resp) {
    // Convert the score to a double
    double score = 0;
    if (!str2dbl(cmd[2], score)) {
        return out_err(resp, ERR_BAD_ARG, "expect float");
    }

    // look up or create the zset
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hash_code = string_hash((uint8_t *)key.key.data(), key.key.size());
    HashNode *hnode = hm_lookup(&server_data.db, &key.node, &entry_key_equals);

    Entry *ent = NULL;
    if (!hnode) {   // insert a new key
        ent = entry_new(TYPE_ZSET);
        ent->key.swap(key.key);
        ent->node.hash_code = key.node.hash_code;
        hm_insert(&server_data.db, &ent->node);
    } 
    else {           // If the key exists, check the type
        ent = container_of(hnode, Entry, node);
        if (ent->type != TYPE_ZSET) {
            return out_err(resp, ERR_BAD_TYP, "expect zset");
        }
    }

    // Insert the score and name into the ZSet
    const std::string &name = cmd[3];
    bool added = zset_insert(&ent->zset, name.data(), name.size(), score);
    return out_int(resp, (int64_t)added);
}

/**
 * Command: ZREM <key> <member>
 * Remove a member from a ZSet.
 */
void zcmd_remove(std::vector<std::string> &cmd, Buffer &resp) {
    // Lookup or validate a ZSet entry in the DB.
    ZSet *zset = expect_zset(cmd[1]);
    if (!zset) { return out_err(resp, ERR_BAD_TYP, "expect zset"); }
    
    // Get the name from the command
    const std::string &name = cmd[2];
    ZNode *znode = zset_lookup(zset, name.data(), name.size());
    
    // Delete the node if it exists
    if (znode) { zset_delete(zset, znode); }
    return out_int(resp, znode ? 1 : 0);
}

/**
 * Command: ZSCORE <key> <member>
 * Get the score of a member in a ZSet.
 */
void zcmd_score(std::vector<std::string> &cmd, Buffer &resp) {
    ZSet *zset = expect_zset(cmd[1]);
    if (!zset) { return out_err(resp, ERR_BAD_TYP, "expect zset"); }
    
    // Get the name from the command
    const std::string &name = cmd[2];
    ZNode *znode = zset_lookup(zset, name.data(), name.size());

    // Return the score
    return znode ? out_dbl(resp, znode->score) : out_nil(resp);
}

/**
 * Command: ZQUERY <key> <score> <name> <offset> <limit>
 * Range query on a ZSet.
 */
void zcmd_query(std::vector<std::string> &cmd, Buffer &resp) {
    // Convert the score to a double
    double score = 0;
    if (!str2dbl(cmd[2], score)) { return out_err(resp, ERR_BAD_ARG, "expect fp number"); }

    // Get the name from the command
    const std::string &name = cmd[3];

    // Convert the offset and limit to integers
    int64_t offset = 0, limit = 0;
    if (!str2int(cmd[4], offset) || !str2int(cmd[5], limit)) { return out_err(resp, ERR_BAD_ARG, "expect int"); }

    ZSet *zset = expect_zset(cmd[1]);
    if (!zset) { return out_err(resp, ERR_BAD_TYP, "expect zset"); }
    
    // Return an empty array if the limit is less than or equal to 0
    if (limit <= 0) { return out_arr(resp, 0); }

    // Get the first node that is >= the score and name
    ZNode *znode = zset_seek_greater_equal(zset, score, name.data(), name.size());
    if (!znode) {
        out_arr(resp, 0);
        return;
    }

    // Get the node at the offset
    znode = znode_offset(znode, offset);
    if (!znode) {
        out_arr(resp, 0);
        return;
    }

    // Count the number of nodes
    size_t n = 0;
    for (ZNode *iter = znode; iter && n < (size_t)limit; iter = znode_offset(iter, +1)) {
        n += 2;
    }

    // Output the number of nodes
    out_arr(resp, (uint32_t)n);

    // Output the nodes
    for (uint32_t i = 0; i < n / 2 && znode; ++i) {
        out_str(resp, znode->name, znode->len);
        out_dbl(resp, znode->score);
        znode = znode_offset(znode, +1);
    }
}