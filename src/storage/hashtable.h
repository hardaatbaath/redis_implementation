#pragma once

// C stdlib
#include <stdint.h>       // uint64_t (hash_code)
#include <stddef.h>       // size_t  (mask, size)

// C++ stdlib
#include <string>   // std::string (LookupKey key)

struct HNode {
    HNode *next = NULL;   // the next node in the chain
    uint64_t hash_code = 0;  // the hash code of the key
};

struct HTable {
    HNode **tab = NULL;  // array of slots; each slot points to the head of a linked list of HNodes (chaining)
    size_t mask = 0;        // used for fast bucket indexing: hash & mask; mask = table_size - 1 (when size is power of 2)
    size_t size = 0;        // total number of keys currently stored in the table
};

/**
 * HMap is a collection of HTables, used to store the keys and values
 * Does progressive rehashing of the keys and values from the older table to the newer table
 */
struct HMap {
    HTable older;
    HTable newer;
    size_t migration_pos = 0;    // the position of the migration in the newer table
};

struct LookupKey {
    struct HNode node;  // hashtable node
    std::string key;
};

// HMap functions
HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
 void   hm_insert(HMap *hmap, HNode *node);
HNode *hm_delete(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
void   hm_clear(HMap *hmap);
size_t hm_size(HMap *hmap);
void hm_foreach(HMap *hmap, bool (*f)(HNode *, void *), void *args); // invoke the callback on each node until it returns false