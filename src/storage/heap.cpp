// C++ stdlib
#include <vector> // std::vector

// local
#include "heap.h" // HeapItem, heap_update, heap_upsert
#include <stddef.h>


// This all is an implementation of a min heap

// Find parent's position
static size_t parent(size_t pos) {
    return (pos + 1) / 2 - 1;
}

// Find left child's position
static size_t left_child (size_t pos) {
    return pos * 2 + 1;
}

// Find right child's position
static size_t right_child(size_t pos) {
    return pos * 2 + 2;
}

// Move the item up the heap
static void heap_up(HeapItem *heap, size_t pos) {
    HeapItem temp = heap[pos];

    // While the item is not the root and the parent's value is greater than the item's value, iterate up the Heap
    while (pos > 0 && heap[parent(pos)].val > temp.val) {
        heap[pos] = heap[parent(pos)];
        *heap[pos].ref = pos;
        pos = parent(pos);
    }

    // Swap the item with the parent
    heap[pos] = temp;
    *heap[pos].ref = pos;
}

// Move the item down the heap, parent node is compared with it's children nodes
static void heap_down(HeapItem *heap, size_t pos, size_t len) {
    HeapItem temp = heap[pos];

    while(true) {
        // Get the left and right child positions
        size_t left_child_pos = left_child(pos);
        size_t right_child_pos = right_child(pos);

        // Get the smallest child position and value
        size_t smallest_child_pos = pos;
        size_t smallest_child_val = temp.val;

        // Check if the left child is smaller than the current smallest child, smallest_child_val is to compare with the right child later
        if (left_child_pos < len && heap[left_child_pos].val < smallest_child_val) {
            smallest_child_pos = left_child_pos;
            smallest_child_val = heap[left_child_pos].val;
        }

        // Check if the right child is smaller than the current smallest child
        if (right_child_pos < len && heap[right_child_pos].val < smallest_child_val) {
            smallest_child_pos = right_child_pos;
        }

        // If the smallest child position is the same as the current position, break the loop
        if (smallest_child_pos == pos) {
            break;
        }

        // Swap the item with the smallest child
        heap[pos] = heap[smallest_child_pos];
        *heap[pos].ref = pos;
        pos = smallest_child_pos;
    }

    // Place the item in the correct position
    heap[pos] = temp;
    *heap[pos].ref = pos;
}

// Update the item in the heap
void heap_update(HeapItem *item, size_t pos, size_t len) {
    if(pos > 0 && item[parent(pos)].val > item[pos].val) {
        heap_up(item, pos);
    } else {
        heap_down(item, pos, len);
    }
}

// Insert or update the item in the heap
void heap_upsert(std::vector<HeapItem> &heap, size_t pos, HeapItem item) {
    // If the position is less than the size of the heap, update the item
    if (pos < heap.size()) {
        heap[pos] = item;
    }
    else {
        pos = heap.size();
        heap.push_back(item);
    }

    // Ensure the item records the correct index even if no swaps occur
    *heap[pos].ref = pos;
    heap_update(heap.data(), pos, heap.size());
}

// Delete the item from the heap
void heap_delete(std::vector<HeapItem> &heap, size_t pos) {
    // Swap the erased item with the last one
    heap[pos] = heap.back();
    heap.pop_back();

    // Update the item
    if (pos < heap.size()) {
        // Fix the back-reference first in case no swaps are needed
        *heap[pos].ref = pos;
        heap_update(heap.data(), pos, heap.size());
    }
}
