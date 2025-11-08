// C stdlib
#include <stdio.h>   // fprintf
#include <stdint.h>

// local
#include "sys_server.h"
#include "constants.h"         // k_idle_timeout_ms
#include "common.h"            // container_of
#include "sys.h"               // get_current_time_ms
#include "../storage/commands.h" // server_data
#include "../net/netio.h"      // Connection, handle_destroy

int32_t next_timer_ms() {
    if (dlist_empty(&server_data.idle_conn_list)) { return -1; }

    uint64_t now_ms = get_current_time_ms();
    Connection *conn = container_of(server_data.idle_conn_list.next, Connection, idle_node);
    uint64_t next_ms = conn->last_activity_ms + k_idle_timeout_ms;
    if (next_ms <= now_ms) { return 0; }
    return (int32_t)(next_ms - now_ms);
}

void process_timers() {
    uint64_t now_ms = get_current_time_ms();

    while (!dlist_empty(&server_data.idle_conn_list)) {
        Connection *conn = container_of(server_data.idle_conn_list.next, Connection, idle_node);
        uint64_t next_ms = conn->last_activity_ms + k_idle_timeout_ms;

        if (next_ms > now_ms) { break; }
        fprintf(stderr, "[server] closing idle connection fd=%d\n", (int)conn->socket_fd);
        handle_destroy(conn);
    }
}


