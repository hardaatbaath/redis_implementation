// C stdlib
#include <assert.h>      // assert (get_key)
#include <stdlib.h>      // strtod, strtoll
#include <math.h>        // isnan

// C++ stdlib
#include <string>        // std::string (Entry keys/values)
#include <vector>        // std::vector (command args)

// local
#include "commands.h"           // Entry/LookupKey, run_request
#include "../core/constants.h" // k_max_msg
#include "../net/serialize.h"   // out_str, out_nil, out_err, out_int
#include "../core/buffer_io.h"  // Buffer
#include "../core/common.h"     // container_of, string_hash

// Top level hashtable for the server
static struct  {
    HashMap db;
} server_data;

/**
 * Equality comparitor for 'struct Entry'
 * container_of is used to recover the address of a parent struct from the address of one of its members. 
*/ 
static bool entry_equals(HashNode *node, HashNode *key) {
    struct Entry *entry = container_of(node, struct Entry, node);
    struct LookupKey *lookup_key = container_of(key, struct LookupKey, node);
    return entry->key == lookup_key->key;
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

    // keys request
    else if (cmd.size() == 1 && cmd[0] == "keys") {
        return all_keys(cmd, resp);
    }

    // zset requests
    // zadd <key> <score> <member>     → adds/updates a member   e.g. zadd players 100 alice
    // zscore <key> <member>           → gets member’s score     e.g. zscore players alice
    // zrem <key> <member>             → removes a member        e.g. zrem players alice
    // zquery <key> <min> <max> <off> <limit> → range query     e.g. zquery players 0 200 0 3
    else if (cmd.size() == 4 && cmd[0] == "zadd") { return zcmd_add(&server_data.db, cmd, resp); }
    else if (cmd.size() == 3 && cmd[0] == "zrem") { return zcmd_remove(&server_data.db, cmd, resp); }
    else if (cmd.size() == 3 && cmd[0] == "zscore") { return zcmd_score(&server_data.db, cmd, resp); }
    else if (cmd.size() == 6 && cmd[0] == "zquery") { return zcmd_query(&server_data.db, cmd, resp); }

    // unknown request
    else {
        // Error in executing command
        return out_err(resp, ERR_UNKNOWN, "unknown command");
    }
}

