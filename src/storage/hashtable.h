#pragma once

// C stdlib
#include <stdint.h>       // uint64_t (hash_code)
#include <stddef.h>       // size_t  (mask, size)

// C++ stdlib
#include <string>   // std::string (LookupKey key)

struct HashNode {
    HashNode *next = NULL;   // the next node in the chain
    uint64_t hash_code = 0;  // the hash code of the key
};

struct HashTable {
    HashNode **tab = NULL;  // array of slots; each slot points to the head of a linked list of HashNodes (chaining)
    size_t mask = 0;        // used for fast bucket indexing: hash & mask; mask = table_size - 1 (when size is power of 2)
    size_t size = 0;        // total number of keys currently stored in the table
};

/**
 * HashMap is a collection of HashTables, used to store the keys and values
 * Does progressive rehashing of the keys and values from the older table to the newer table
 */
struct HashMap {
    HashTable older;
    HashTable newer;
    size_t migration_pos = 0;    // the position of the migration in the newer table
};

struct LookupKey {
    struct HashNode node;  // hashtable node
    std::string key;
};

// HashMap functions
HashNode *hm_lookup(HashMap *hmap, HashNode *key, bool (*eq)(HashNode *, HashNode *));
 void   hm_insert(HashMap *hmap, HashNode *node);
HashNode *hm_delete(HashMap *hmap, HashNode *key, bool (*eq)(HashNode *, HashNode *));
void   hm_clear(HashMap *hmap);
size_t hm_size(HashMap *hmap);
void hm_foreach(HashMap *hmap, bool (*f)(HashNode *, void *), void *args); // invoke the callback on each node until it returns false