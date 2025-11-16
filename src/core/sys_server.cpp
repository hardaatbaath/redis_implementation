// C stdlib
#include <stdio.h>   // fprintf
#include <stdint.h>

// local
#include "sys_server.h"
#include "constants.h"         // k_idle_timeout_ms, k_max_works
#include "common.h"            // container_of
#include "sys.h"               // get_current_time_ms
#include "../storage/commands.h" // server_data, Entry
#include "../net/netio.h"      // Connection, handle_destroy
#include "../storage/heap.h"   // heap_delete

static bool hnode_same(HNode *node, HNode *key) {
    return node == key;
}


int32_t next_timer_ms() {
    uint64_t now_ms = get_current_time_ms();
    uint64_t next_ms = (uint64_t)-1;

    // Idle connection timers (ordered list)
    if (!dlist_empty(&server_data.idle_conn_list)) {
        Connection *conn = container_of(server_data.idle_conn_list.next, Connection, idle_node);
        next_ms = conn->last_activity_ms + k_idle_timeout_ms;
    }

    // Get the next expiration time from the heap
    if (!server_data.heap.empty() && server_data.heap[0].val < next_ms) {
        next_ms = server_data.heap[0].val;
    }

    // If there is no next expiration time, return -1
    if (next_ms == (uint64_t)-1) { return -1; }

    // If the next expiration time is in the past, return 0
    if (next_ms <= now_ms) { return 0; }

    // Return the time until the next expiration time
    return (int32_t)(next_ms - now_ms);
}

/**
 * First delete the idle connections according to the current time and expiration time.
 * Then delete the expired entries from the heap.
 */
void process_timers() {
    uint64_t now_ms = get_current_time_ms();

    // Idle connection timers (ordered list)
    while (!dlist_empty(&server_data.idle_conn_list)) {
        Connection *conn = container_of(server_data.idle_conn_list.next, Connection, idle_node);
        uint64_t next_ms = conn->last_activity_ms + k_idle_timeout_ms;
        if(next_ms >= now_ms) { break;} // if the next connection is not expired, break the loop
        
        fprintf(stderr, "[server] closing idle connection fd=%d\n", (int)conn->socket_fd);
        handle_destroy(conn); // close the connection
    }

    // Key TTL timers (min-heap by expiration)
    size_t num_works = 0;
    while (!server_data.heap.empty() && server_data.heap[0].val <= now_ms) {
        // Take head and proactively remove it so we do not re-observe the same head.
        Entry *entry = container_of(server_data.heap[0].ref, Entry, heap_idx);
        heap_delete(server_data.heap, 0);
        entry->heap_idx = (size_t)-1;

        HNode *node = hm_delete(&server_data.db, &entry->node, &hnode_same);
        
        if (node != &entry->node) {
            if (node == NULL) {
                // The entry was already removed from the hash map (e.g. a DEL or explicit delete happened),
                // the heap contained a stale reference. This is not fatal — just free the entry.
                fprintf(stderr, "[server] warning: heap referred to an entry already removed from db for key '%s'\n", entry->key.c_str());
            } else {
                // Unexpected mismatch: log and continue rather than aborting the server.
                fprintf(stderr, "[server] warning: hash node mismatch for key '%s' (node=%p, expected=%p)\n",
                        entry->key.c_str(), (void*)node, (void*)&entry->node);
            }
            // Do not assert here; proceed to free the entry owned by the heap.
        }

        // Free entry (will not touch heap since heap_idx is already -1)
        entry_del(entry);
        if (++num_works >= k_max_works) { break; } // Don't stall the server if too many entries need to be deleted at once
    }
}