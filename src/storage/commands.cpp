// C stdlib
#include <assert.h>      // assert (get_key)

// C++ stdlib
#include <string>        // std::string (Entry keys/values)
#include <vector>        // std::vector (command args)

// local
#include "commands.h"           // Entry/LookupKey, run_request
#include "../core/constants.h" // k_max_msg

#define container_of(ptr, T, member) \
    ((T *) ((char*) ptr - offsetof(T, member)))

// Top level hashtable for the server
static struct  {
    HashMap db;
} server_data;

/**
 * Equality comparitor for 'struct Entry'
 * container_of is used to recover the address of a parent struct from the address of one of its members. 
*/ 
bool entry_equals(HashNode *lhs, HashNode *rhs) {
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return le->key == re->key;
}

/**
 * FNV hash function
*/
uint64_t string_hash(const uint8_t *data, size_t len){
    uint32_t base = 0x811C9DC5; // FNV-1a offset basis, Decimal: 2166136261
    uint32_t prime = 0x1000193; // FNV-1a prime, Decimal: 16777619

    for (size_t i = 0; i < len; i++) {
        base = (base + data[i]) * prime;
    }
    return base;
}

/**
 * Set the value of the key from the hash table
*/
void set_key(std::vector<std::string> &cmd, Response &resp){
    // A dummy 'Entry' just for the lookup
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hash_code = string_hash((uint8_t*) key.key.data(), key.key.size());

    // Hashtable Lookup
    HashNode *node = hm_lookup(&server_data.db, &key.node, &entry_equals);
    if(node) {
        // Key already exists, update the value
        container_of(node, Entry, node)->value.swap(cmd[2]);
    }
    else {
        // Key does not exist, create a new entry
        Entry *key_entry = new Entry();
        key_entry->key.swap(key.key);
        key_entry->value.swap(cmd[2]);
        key_entry->node.hash_code = key.node.hash_code;
        hm_insert(&server_data.db, &key_entry->node);
    }
    resp.status = RES_OK;
}

/**
 * Get the value of the key from the hash table
*/
void get_key(std::vector<std::string> &cmd, Response &resp){
    // A dummy 'Entry' just for the lookup
    Entry key;
    key.key.swap(cmd[1]);    // swap is more memory optimised, pointers are swapped instead of creating new overhead of assignment
    key.node.hash_code = string_hash((uint8_t *)key.key.data(), key.key.size());

    // Hashtable lookup
    HashNode *node = hm_lookup(&server_data.db, &key.node, &entry_equals);
    if (!node){
        resp.status = RES_NX;
        return;
    }
    
    // Copy the values
    const std:: string &val = container_of(node, Entry, node)->value;      // Returns the value of the HashNode
    assert(val.size() <= k_max_msg);
    resp.status = RES_OK;
    resp.data.assign(val.begin(), val.end());
}

/**
 * Delete the value of the key from the hash table
*/
void del_key(std::vector<std::string> &cmd, Response &resp){
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hash_code = string_hash((uint8_t*) key.key.data(), key.key.size());

    // Hashtable delete
    HashNode *node = hm_delete(&server_data.db, &key.node, &entry_equals);
    if (node){
        // Key found, delete the entry via pointer to the entry
        delete container_of(node, Entry, node);
        resp.status = RES_OK;
    }
    else {
        // Key not found, return not found
        resp.status = RES_NX;
    }
}

/**
 * Get all the keys from the hash table
*/
void all_keys(std::vector<std::string> &, Response &resp){
    bool first = true;
    auto append_table = [&](const HashTable &table) {
        if (!table.tab) { return; }
        for (size_t i = 0; i <= table.mask; ++i) {
            for (HashNode *node = table.tab[i]; node; node = node->next) {
                const Entry *entry = container_of(node, Entry, node);
                const std::string line = (first ? "" : "\n") + entry->key + " " + entry->value;
                first = false;
                resp.data.insert(resp.data.end(), line.begin(), line.end());
            }
        }
    };
    // include keys from both tables to avoid missing entries mid-migration
    append_table(server_data.db.newer);
    append_table(server_data.db.older);
    resp.status = RES_OK;
}

/** 
 * Run one request
*/
void run_request(std::vector<std::string> &cmd, Response &resp) {
    resp.status = RES_OK;        // Successfully executed command
    
    // ping request
    if (cmd.size() == 1 && cmd[0] == "ping") {
        const uint8_t *p = (const uint8_t*)"pong";
        resp.data.assign(p, p + 4);
    }

    // get request
    else if (cmd.size() == 2 && cmd[0] == "get") {
        get_key(cmd, resp);
    }

    // set request
    else if (cmd.size() == 3 && cmd[0] == "set") {
        set_key(cmd, resp);
    }

    // del request
    else if (cmd.size() == 2 && cmd[0] == "del") {
        del_key(cmd, resp);
    }

    // all keys request
    else if (cmd.size() == 2 && cmd[0] == "all" && cmd[1] == "keys") {
        all_keys(cmd, resp);
    }

    // unknown request
    else {
        resp.status = RES_ERR;     // Error in executing command
        return;
    }
}

