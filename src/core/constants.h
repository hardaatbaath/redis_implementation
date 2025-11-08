#pragma once

#include <stddef.h>
#include <stdint.h>

// Maximum message size (header+payload safety bound)
const size_t k_max_msg = 32u << 20; // 32 MiB

// Maximum number of arguments for a command
const size_t k_max_args = 200 * 1000; // 200,000 arguments

// Maximum load factor for the hash table
const size_t k_max_load_factor = 8;

// Constant workload for rehashing
const size_t k_rehashing_work = 128;

// Constant for the idle timeout in milliseconds
const uint64_t k_idle_timeout_ms = 5 * 1000; // 5 seconds