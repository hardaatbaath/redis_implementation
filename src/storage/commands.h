#pragma once

// C++ stdlib
#include <string>   // std::string (Entry keys/values)
#include <vector>   // std::vector (command args)
#include <stdint.h> // uint64_t

// local
#include "hashtable.h" // HNode, HMap, hm_*
#include "../core/buffer_io.h" // Buffer
#include "sorted_set.h" // ZSet, ZNode, zset_*
#include "heap.h" // HeapItem, heap_* operations
#include "list.h" // DList
#include "../core/thread_pool.h" // TheadPool

// Forward declaration to avoid including netio.h here
struct Connection;

// Top level hashtable for the server, had to be changed from static to a struct to avoid redefinition errors
struct ServerData {
    HMap db;
    std::vector<Connection *> fd2conn; // a map of all the client connections, keyed by the file descriptor
    DList idle_conn_list; // list to store the timers for idle connections
    std::vector<HeapItem> heap; // heap to store the ttl values of the keys
    TheadPool thread_pool; // worker pool for heavy destructors
};

// Global instance of the server data
extern ServerData server_data;


// Supported value types in each entry
enum ValueType : uint8_t {
    TYPE_INIT  = 0,
    TYPE_STR   = 1,    // string
    TYPE_ZSET  = 2,    // sorted set
};

// KV pair storage for the server
struct Entry {
    struct HNode node;    // embedded hashnode node
    std::string key;        // key of the entry

    size_t heap_idx = (size_t)-1; // index of the item in the heap, this is for the ttl
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

// the error was that the ttl_ms was unsigned, but it should be signed, as we are using -1 to remove the ttl
void entry_set_ttl(Entry *entry, int64_t ttl);

// Heavy delete (may offload to thread pool)
void entry_del(Entry *entry);

// Request handlers
void set_key(std::vector<std::string> &cmd, Buffer &resp); // set the value of the key
void get_key(std::vector<std::string> &cmd, Buffer &resp); // get the value of the key
void del_key(std::vector<std::string> &cmd, Buffer &resp); // delete the value of the key
void all_keys(std::vector<std::string> &, Buffer &resp); // get all the keys

// Run one request
void run_request(std::vector<std::string> &cmd, Buffer &resp);

