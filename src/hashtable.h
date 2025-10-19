#include <stdint.h>       // uint64_t
#include <stddef.h>       // size_t

struct HashNode {
    HashNode *next = NULL;   // the next node in the chain
    uint64_t hash_code = 0;  // the hash code of the key
};

struct HashTable {
    HashNode **tab = NULL;  // array of slots; each slot points to the head of a linked list of HashNodes (chaining)
    size_t mask = 0;        // used for fast bucket indexing: hash & mask; mask = table_size - 1 (when size is power of 2)
    size_t size = 0;        // total number of keys currently stored in the table
};

struct HashMap {
    HashTable older;
    HashTable newer;
    size_t migration_pos = 0;    // the position of the migration in the newer table
};

// HashTable functions
void h_init(HashTable *ht, size_t size);
HashNode **h_lookup(HashTable *ht, HashNode *key, bool (*eq)(HashNode *, HashNode *));
void h_insert(HashTable *ht, HashNode *node);
HashNode *h_detach(HashTable *ht, HashNode **node);

// HashMap functions
void   hm_trigger_rehash(HashMap *hmap);
HashNode *hm_lookup(HashMap *hmap, HashNode *key, bool (*eq)(HashNode *, HashNode *));
void   hm_insert(HashMap *hmap, HashNode *node);
HashNode *hm_delete(HashMap *hmap, HashNode *key, bool (*eq)(HashNode *, HashNode *));
void   hm_clear(HashMap *hmap);
size_t hm_size(HashMap *hmap);