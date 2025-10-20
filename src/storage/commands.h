#pragma once

// C++ stdlib
#include <string>   // std::string (Entry keys/values)
#include <vector>   // std::vector (command args)
#include <stdint.h> // uint64_t

// local
#include "hashtable.h" // HashNode, HashMap, hm_*
#include "../net/protocol.h"  // Response

// KV pair storage for the server
struct Entry {
    struct HashNode node;    // embedded hashnode node
    std::string key;
    std::string value;
};


/**
 * Equality comparitor for 'struct Entry'
 * container_of is used to recover the address of a parent struct from the address of one of its members. 
*/ 
bool entry_equals(HashNode *lhs, HashNode *rhs);

/**
 * FNV hash function
*/
uint64_t string_hash(const uint8_t *data, size_t len);

// Request handlers
void set_key(std::vector<std::string> &cmd, Response &resp);
void get_key(std::vector<std::string> &cmd, Response &resp);
void del_key(std::vector<std::string> &cmd, Response &resp);
void all_keys(std::vector<std::string> &, Response &resp);

/** 
 * Run one request
*/
void run_request(std::vector<std::string> &cmd, Response &resp);

