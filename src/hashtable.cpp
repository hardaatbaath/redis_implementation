#include <assert.h>       // assert
#include <cstddef>
#include <stddef.h>       // size_t


#include "hashtable.h"



/**
 * Initialize the hash table, give memory for "size" number of HashNode pointers
 */
void h_init(HashTable *ht, size_t size) {
    assert (size > 0 && (size & (size - 1)) == 0);                          // size must be a power of 2, modulo is expensive
    ht->tab = (HashNode **)calloc(size, sizeof(HashNode *));   // give memory for "size" number of HashNode pointers
    ht->mask = size - 1;
    ht->size = 0;
}

/**
 * Insert a node into the hash table
 */
void h_insert(HashTable *ht, HashNode *node) {
    size_t pos = node->hash_code & ht->mask;     // node->hash_code & (table_size - 1); mask = table_size - 1 (when size is power of 2)
    HashNode *next = ht->tab[pos];               // get the head of the chain
    node->next = next;                           // set the next node as the head of the chain
    ht->tab[pos] = node;                         // set the node as the head of the chain
    ht->size++;                                  // increment the number of keys in the table
}

/** 
 * Lookup a node in the hash table
 */
HashNode **h_lookup(HashTable *ht, HashNode *key, bool (*eq)(HashNode *, HashNode *)) {
    if (!ht->tab) { return NULL; } // table not initialized

    size_t pos = key->hash_code & ht->mask;     // get the bucket index of the key
    HashNode **from = &ht->tab[pos];            // get the pointer of the pointer to the head of the chain

    // Make a node, in each iter set it to the from, then increment the from to the next node in the chain
    for (HashNode *curr; (curr = *from) != NULL; from = &curr->next) {
        if (curr->hash_code == key->hash_code && eq(curr, key)) { return from; } // curr is the node, from is the pointer to the node
    }
    return NULL; // key not found
}

/**
 * Detach a node from the hash table
 */
HashNode *h_detach(HashTable *ht, HashNode **node) {
    HashNode *target = *node;   // Get the node to detach
    *node = target->next;       // Update the incoming pointer to the target's next node
    ht->size--;
    return target;
}