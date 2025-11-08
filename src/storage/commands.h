#pragma once

// C++ stdlib
#include <string>   // std::string (Entry keys/values)
#include <vector>   // std::vector (command args)
#include <stdint.h> // uint64_t

// local
#include "hashtable.h" // HNode, HMap, hm_*
#include "../net/protocol.h"  // Response
#include "../core/buffer_io.h" // Buffer
#include "sorted_set.h" // ZSet, ZNode, zset_*
#include "../net/netio.h" // Connection


// Supported value types in each entry
enum ValueType : uint8_t {
    TYPE_INIT  = 0,
    TYPE_STR   = 1,    // string
    TYPE_ZSET  = 2,    // sorted set
};

// Top level hashtable for the server
static struct  {
    HMap db;
    std::vector<Connection *> fd2conn; // a map of all the client connections, keyed by the file descriptor
    DList idle_conn_list; // list to store the timers for idle connections
} server_data;

// KV pair storage for the server
struct Entry {
    struct HNode node;    // embedded hashnode node
    std::string key;        // key of the entry
    uint32_t type = TYPE_INIT; // type of the value

    // One of the following
    std::string value;      // value of the entry
    ZSet zset;
};

inline static Entry *entry_new(uint32_t type = TYPE_INIT) {
    Entry *entry = new Entry();
    entry->type = type;
    return entry;
}

inline static void entry_del(Entry *entry) {
    if (entry->type == TYPE_ZSET) {
        zset_clear(&entry->zset);
    }
    delete entry;
}

// Request handlers
void set_key(std::vector<std::string> &cmd, Buffer &resp); // set the value of the key
void get_key(std::vector<std::string> &cmd, Buffer &resp); // get the value of the key
void del_key(std::vector<std::string> &cmd, Buffer &resp); // delete the value of the key
void all_keys(std::vector<std::string> &, Buffer &resp); // get all the keys

// Run one request
void run_request(std::vector<std::string> &cmd, Buffer &resp);

