// C stdlib
#include <assert.h>       // assert (h_init)
#include <stddef.h>       // size_t (interfaces)
#include <stdlib.h>       // calloc(), free() (h_init, hm_help_rehashing, hm_clear)

// local
#include "hashtable.h"         // HashNode/HashTable/HashMap declarations
#include "../core/constants.h" // k_max_load_factor, k_rehashing_work
/**
 * Initialize the hash table, give memory for "size" number of HashNode pointers
*/
static void h_init(HashTable *ht, size_t size) {
    assert (size > 0 && (size & (size - 1)) == 0);                          // size must be a power of 2, modulo is expensive
    ht->tab = (HashNode **)calloc(size, sizeof(HashNode *));   // give memory for "size" number of HashNode pointers
    ht->mask = size - 1;
    ht->size = 0;
}

/** 
 * Lookup a node in the hash table
 * Returns pointer to the pointer that links to the target node - useful for deletion
*/
static HashNode **h_lookup(HashTable *ht, HashNode *key, bool (*eq)(HashNode *, HashNode *)) {
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
 * Insert a node into the hash table
*/
static void h_insert(HashTable *ht, HashNode *node) {
    size_t pos = node->hash_code & ht->mask;     // node->hash_code & (table_size - 1); mask = table_size - 1 (when size is power of 2)
    HashNode *next = ht->tab[pos];               // get the head of the chain
    node->next = next;                           // set the next node as the head of the chain
    ht->tab[pos] = node;                         // set the node as the head of the chain
    ht->size++;                                  // increment the number of keys in the table
}

/**
 * Detach a node from the hash table
*/
static HashNode *h_detach(HashTable *ht, HashNode **node) {
    HashNode *target = *node;   // Get the node to detach
    *node = target->next;       // Update the incoming pointer to the target's next node
    ht->size--;
    return target;
}

static bool h_foreach(HashTable *ht, bool (*f) (HashNode *, void *), void *args) {
    for (size_t i = 0; i <= ht->mask; i++) {
        for (HashNode *node = ht->tab[i]; node; node = node->next) {
            if (!f(node, args)) { return false; }
        }
    }
    return true;
}

/**
 * Trigger the rehashing of the hash table
*/
static void hm_trigger_rehash(HashMap *hmap) {
    assert(hmap->older.tab == NULL);
    hmap->older = hmap->newer;                                 // (newer, older) <- (new_table, newer)
    h_init(&hmap->newer, (hmap->newer.size + 1) * 2); // Double the size of the newer table
    hmap->migration_pos = 0;
}

/**
 * Help migrate the keys from the older table to the newer table
*/
static void hm_help_rehashing(HashMap *hmap) {
    size_t work = 0;
    while (work < k_rehashing_work && hmap->older.size > 0) {

        // Find an empty slot
        HashNode **from = &hmap->older.tab[hmap->migration_pos];
        if (!*from) {
            hmap->migration_pos++;
            continue; // Skip this slot
        }

        // Move the first list item to the newer table
        h_insert(&hmap->newer, h_detach(&hmap->older, from));
        work++;
    }

    // If the old table is empty, discard
    if (hmap->older.size == 0 && hmap->older.tab) {
        free(hmap->older.tab);
        hmap->older = HashTable{};
    }
}

/**
 * Lookup a node in the hash table via hashmap
*/
HashNode *hm_lookup(HashMap *hmap, HashNode *key, bool (*eq)(HashNode *, HashNode *)) {   
    // Help migrate the keys
    hm_help_rehashing(hmap);

    // First search in the newer table
    HashNode **from = h_lookup(&hmap->newer, key, eq);
    // If not found, search in the older table
    if (!from) { from = h_lookup(&hmap->older, key, eq); }

    return from? *from : NULL;
}

/**
 * Insert a node into the hash table via hashmap
*/
void hm_insert(HashMap *hmap, HashNode *node) {
    // If the newer table is not initialized, initialize it with a size of 4
    if (!hmap->newer.tab) { h_init(&hmap->newer, 4); }

    h_insert(&hmap->newer, node); // Always insert into the newer table

    // If the older table is not initialized, this means that we are currently in the process of rehashing
    // So we need to trigger the rehashing if the load factor is exceeded
    if (!hmap->older.tab) { 
        size_t threshold = (hmap->newer.mask + 1) * k_max_load_factor;
        if (hmap->newer.size >= threshold) { hm_trigger_rehash(hmap); }
    }

    // Help migrate the keys
    hm_help_rehashing(hmap);
}

/**
 * Delete a node from the hash table via hashmap
*/
HashNode *hm_delete(HashMap *hmap, HashNode *key, bool (*eq)(HashNode *, HashNode *)) {
    hm_help_rehashing(hmap);

    // First delete from the older table
    if (HashNode **from = h_lookup(&hmap->older, key, eq)) { return h_detach(&hmap->older, from); }
    // If not found, delete from the newer table
    if (HashNode **from = h_lookup(&hmap->newer, key, eq)) { return h_detach(&hmap->newer, from); }
    return NULL;
}

/**
 * Clear the hash table via hashmap
*/
void hm_clear(HashMap *hmap) {
    free(hmap->newer.tab);
    free(hmap->older.tab);
    *hmap = HashMap{};
}

/**
 * Get the size of the hash table via hashmap
*/
size_t hm_size(HashMap *hmap) {
    return hmap->newer.size + hmap->older.size;
}

void hm_foreach(HashMap *hmap, bool (*f)(HashNode *, void *), void *args) {
    h_foreach(&hmap->newer, f, args) && h_foreach(&hmap->older, f, args);
}