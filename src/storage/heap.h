#pragma once

// C stdlib
#include <stdint.h>
#include <stddef.h>

// C++ stdlib
#include <vector> // std::vector
#include <string> // std::string

// local
#include "commands.h" // Entry
#include "../core/sys.h" // get_monotonic_time

struct HeapItem {
    uint64_t val = 0;
    size_t *ref = NULL;
};

inline void entry_set_ttl(Entry *entry, uint64_t ttl_ms) {
    if (ttl_ms < 0 && entry->heap_idx != (size_t)-1) {
        // Setting a -1 ttl means that the key is deleted
        heap_delete(server_data.heap, entry->heap_idx);
        entry->heap_idx = (size_t) -1;
    }
    else if (ttl_ms >= 0) {
        // Add or update the heap item
        uint64_t expires_at = get_current_time_ms() + (uint64_t)ttl_ms;
        HeapItem item = { expires_at, &entry->heap_idx};
        heap_upsert(server_data.heap, entry->heap_idx, item);
    }
}

// Basic heap operations
void heap_update(HeapItem *item, uint64_t pos, size_t len);
void heap_upsert(std::vector<HeapItem> &heap, size_t pos, HeapItem item);
void heap_delete(std::vector<HeapItem> &heap, size_t pos, HeapItem item);

// Commands for the heap
