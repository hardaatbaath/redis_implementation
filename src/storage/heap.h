#pragma once

// C stdlib
#include <stdint.h>
#include <stddef.h>

// C++ stdlib
#include <vector> // std::vector

// Heap item used for TTL scheduling
struct HeapItem {
    uint64_t val = 0;   // expiration timestamp (ms)
    size_t *ref = NULL; // back-reference to the owner's index in the heap
};

// Basic heap operations (min-heap)
void heap_update(HeapItem *heap, size_t pos, size_t len);
void heap_upsert(std::vector<HeapItem> &heap, size_t pos, HeapItem item);
void heap_delete(std::vector<HeapItem> &heap, size_t pos);
