// C stdlib
#include <assert.h>      // assert (get_key)

// C++ stdlib
#include <string>        // std::string (Entry keys/values)
#include <vector>        // std::vector (command args)

// local
#include "commands.h"           // Entry/LookupKey, run_request
#include "../core/constants.h" // k_max_msg
#include "../net/serialize.h"   // out_str, out_nil, out_err, out_int
#include "../core/sys.h"        // Buffer

/**
 * Macro to recover the address of a parent struct from the address of one of its members. 
 * As we are using intrusive data structure, T and ptr for the members are together in the same memory location.
 * So we can recover the address of the parent struct by subtracting the offset of the member from the address of the pointer.
*/
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

// FNV hash function

uint64_t string_hash(const uint8_t *data, size_t len){
    uint32_t base = 0x811C9DC5; // FNV-1a offset basis, Decimal: 2166136261
    uint32_t prime = 0x1000193; // FNV-1a prime, Decimal: 16777619

    for (size_t i = 0; i < len; i++) {
        base = (base + data[i]) * prime;
    }
    return base;
}

// Set the value of the key from the hash table
void set_key(std::vector<std::string> &cmd, Buffer &resp){
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
    return out_nil(resp);
}

// Get the value of the key from the hash table
void get_key(std::vector<std::string> &cmd, Buffer &resp){
    // A dummy 'Entry' just for the lookup
    Entry key;
    key.key.swap(cmd[1]);    // swap is more memory optimised, pointers are swapped instead of creating new overhead of assignment
    key.node.hash_code = string_hash((uint8_t *)key.key.data(), key.key.size());

    // Hashtable lookup
    HashNode *node = hm_lookup(&server_data.db, &key.node, &entry_equals);
    if (!node){
        return out_nil(resp);
    }
    
    // Copy the values
    const std:: string &val = container_of(node, Entry, node)->value;      // Returns the value of the HashNode
    assert(val.size() <= k_max_msg);
    return out_str(resp, val.data(), val.size());
}

// Delete the value of the key from the hash table
void del_key(std::vector<std::string> &cmd, Buffer &resp){
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hash_code = string_hash((uint8_t*) key.key.data(), key.key.size());

    // Hashtable delete
    HashNode *node = hm_delete(&server_data.db, &key.node, &entry_equals);
    if (node){
        // Key found, delete the entry via pointer to the entry
        delete container_of(node, Entry, node);
    }
    return out_int(resp, node ? 1 : 0);
}

// Callback function for the keys command
static bool cb_keys(HashNode *node, void *arg) {
    Buffer &resp = *(Buffer *)arg;
    const Entry *entry = container_of(node, Entry, node);
    // Emit one array element as a nested array: key : value
    std::string kV_pair = entry->key + " : " + entry->value;
    out_str(resp, kV_pair.data(), kV_pair.size());
    return true;
}

// Get all the keys from the hash table
void all_keys(std::vector<std::string> &, Buffer &resp) {
    out_arr(resp, (uint32_t)hm_size(&server_data.db));
    hm_foreach(&server_data.db, &cb_keys, (void *)&resp);
}

// Run one request
void run_request(std::vector<std::string> &cmd, Buffer &resp) {
    // ping request
    if (cmd.size() == 1 && cmd[0] == "ping") {
        return out_str(resp, "pong", 4);
    }

    // get request
    else if (cmd.size() == 2 && cmd[0] == "get") {
        return get_key(cmd, resp);
    }

    // set request
    else if (cmd.size() == 3 && cmd[0] == "set") {
        return set_key(cmd, resp);
    }

    // del request
    else if (cmd.size() == 2 && cmd[0] == "del") {
        return del_key(cmd, resp);
    }

    // all keys request
    else if (cmd.size() == 2 && cmd[0] == "all" && cmd[1] == "keys") {
        return all_keys(cmd, resp);
    }

    // unknown request
    else {
        // Error in executing command
        return out_err(resp, ERR_UNKNOWN, "unknown command");
    }
}

