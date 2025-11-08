## Overview
This project implements a minimal Redis-like client/server in GNU C++17. It focuses on:
- Non-blocking TCP I/O with `poll(2)` and per-connection state machines
- An argv-framed binary request protocol and typed binary responses
- A hash table with incremental (progressive) rehashing for KV storage
- A sorted set (ZSet) built on an AVL tree + name index

All code runs single-threaded; concurrency is achieved by multiplexing with `poll`.

## Key Modules and Responsibilities
### src/server.cpp
- Entry point for the server
- Creates and configures the listening socket on 0.0.0.0:8080 (non-blocking)
- Runs the main event loop:
  - Builds `pollfd` array with listening socket and all active client sockets
  - Waits for readiness with `poll(2)` using a timeout from timers
  - On readiness:
    - Accept new clients
    - For client sockets, dispatch to read/write handlers
    - Close on error or when requested by application logic
  - Manages idle timers via the idle DLL (doubly-linked list)

### src/client.cpp
- Interactive REPL client to exercise the server
- Connects to 127.0.0.1:8080, reads commands from stdin
- Encodes argv-framed requests, writes to server
- Reads typed responses and prints them

### src/core/sys.{h,cpp}
- Logging helpers: `msg`, `msg_error`, `die`
- Set file descriptor to non-blocking: `fd_set_nb`
- Monotonic clock in milliseconds: `get_current_time_ms`
- Contains no server-only symbols (so the client can link against it)

### src/core/sys_server.{h,cpp}
- Server-only timer APIs:
  - `next_timer_ms()`: returns milliseconds until the earliest idle connection times out, or -1 if none
  - `process_timers()`: closes expired idle connections in time order
- Uses global `server_data` to manage the idle connection list

### src/net/netio.{h,cpp}
- Defines `Connection` (per-client state), per-connection I/O handlers:
  - `handle_read(Connection*)`
  - `handle_write(Connection*)`
  - `handle_destroy(Connection*)`
  - `handle_one_request(Connection*)` (parse → execute → generate response)
- Orchestrates:
  - Partial reads: append to `incoming` buffer
  - Parse when full frame `[len:u32][payload]` arrives
  - Dispatch to `run_request` (storage/commands.cpp)
  - Serialize response into `outgoing`, write partially if needed

### src/net/protocol.{h,cpp}
- Request framing and argv payload parsing:
  - Request from client:
    - Frame: `[payload_len:u32][payload_bytes...]`
    - Payload: `[num_args:u32][len:u32 arg0][bytes...][len:u32 arg1][bytes...]...`
  - `parse_request(const uint8_t* data, size_t size, std::vector<std::string>& cmd)`

### src/net/serialize.{h,cpp}
- Typed response serialization (server → client) and client-side printing:
  - Tags: `NIL`, `ERR(code,msg)`, `STR`, `INT`, `DBL`, `BOOL`, `ARR`, `MAP`
  - Helpers to append encoded values into `Buffer` (`std::vector<uint8_t>`)

### src/storage/commands.{h,cpp}
- Defines global server state: `ServerData server_data` (single instance)
  - `HMap db` for KV and zset keys
  - `std::vector<Connection*> fd2conn` mapping fd → connection
  - `DList idle_conn_list` ordered by last activity (LRU-ish)
- Command handlers and dispatcher:
  - String KV: `set`, `get`, `del`, `keys`, plus `ping`
  - ZSet: `zadd`, `zrem`, `zscore`, `zquery`
  - `run_request(std::vector<std::string>& cmd, Buffer& resp)` routes to handlers

### src/storage/hashtable.{h,cpp}
- Chaining hash table with incremental rehashing:
  - Two tables: `newer` and `older`, and a `migration_pos`
  - Inserts always go into `newer`
  - Lookups check `newer` first, then `older`
  - Deletions attempt `older` then `newer`
  - A small fixed amount of rehash work is done on each operation (`k_rehashing_work`)
- Interfaces:
  - `hm_lookup`, `hm_insert`, `hm_delete`, `hm_clear`, `hm_size`, `hm_foreach`
- Structures:
  - `HNode`: intrusive node with `next` and `hash_code`
  - `HTable`: array of slots + mask + size
  - `HMap`: `{ older, newer, migration_pos }`

### src/storage/avl_tree.{h,cpp}
- Balanced AVL tree with parent pointers and subtree counts:
  - Supports O(log n) insert/delete
  - `avl_offset(node, rank_delta)` returns node by rank (order-statistics)
- Structures:
  - `AVLNode`: `{ parent, left, right, height, cnt }`

### src/storage/sorted_set.{h,cpp}
- ZSet: overlay of two indexes:
  - AVL tree ordered by `(score, name)` for sorted queries/range
  - Hash table by `name` for O(log n) lookups
- Structures:
  - `ZSet`: `{ root: AVLNode*, hmap: HMap }`
  - `ZNode`: `{ tree: AVLNode, hmap: HNode, score: double, len: size_t, name[] }`
- ZSet operations:
  - `zset_lookup(zset, name, len)` by hashtable
  - `zset_insert(zset, name, len, score)` add/update (update reindexes in the tree)
  - `zset_delete(zset, node)`
  - `zset_seek_greater_equal(zset, score, name, len)` find first tuple ≥ key
  - `znode_offset(node, offset)` walk by sorted order
- Command handlers:
  - `zcmd_add`, `zcmd_remove`, `zcmd_score`, `zcmd_query`

### src/storage/list.h
- Doubly-linked list for idle timer ordering:
  - `dlist_init(DList*)` makes a sentinel (prev/next → self)
  - `dlist_empty(DList*)` checks if list has no items (sentinel-only)
  - `dlist_detach(DList* node)` unlinks a node (assumes node is linked)
  - `dlist_insert_before(DList* target, DList* rookie)` inserts before target
- Used to maintain idle connections in ascending order of next timeout

### src/core/buffer_io.h
- Generic `Buffer = std::vector<uint8_t>`
- Helpers to append/consume bytes and encode primitive types (u8/u32/i64/f64/bool)

### src/core/constants.h
- Tunable constants:
  - `k_max_msg` (max frame size)
  - `k_max_args` (max argv per request)
  - Hashtable load and rehash work: `k_max_load_factor`, `k_rehashing_work`
  - `k_idle_timeout_ms` (idle connection timeout)

## Global State and Core Data Types
### ServerData (global)
```text
ServerData {
  HMap db;
  std::vector<Connection*> fd2conn;  // index by socket fd
  DList idle_conn_list;              // ordered by last activity, sentinel at head
}
```

### Connection (per-client)
```text
Connection {
  int socket_fd;
  bool want_read, want_write, want_close;   // readiness intents
  Buffer incoming;   // bytes read from socket awaiting parsing
  Buffer outgoing;   // bytes to be written back to socket
  uint64_t last_activity_ms;
  DList idle_node;   // node linked into server_data.idle_conn_list
}
```

### Hashtable hierarchy
```text
HMap {
  HTable older;           // may be empty; used during rehash
  HTable newer;           // main table receiving inserts
  size_t migration_pos;   // index in older.tab for incremental migration
}

HTable {
  HNode** tab;            // slot array (power-of-two sized)
  size_t mask;            // (size-1) for fast modulo
  size_t size;            // number of nodes in this table
}

HNode {
  HNode* next;            // chaining in buckets
  uint64_t hash_code;     // precomputed
}
```

### ZSet hierarchy
```text
ZSet {
  AVLNode* root;    // ordered by (score, name)
  HMap hmap;        // index by name for O(log n) lookups
}

ZNode {
  AVLNode tree;     // intrusive AVL node
  HNode   hmap;     // intrusive hash node
  double  score;
  size_t  len;
  char    name[];   // flexible array (name bytes, NUL-terminated)
}
```

## End-to-End Control Flow
### Server startup
1. `main()` (server.cpp):
   - Initialize idle list: `dlist_init(&server_data.idle_conn_list)`
   - Create listening socket (AF_INET/SOCK_STREAM), set `SO_REUSEADDR`
   - Bind to `0.0.0.0:8080`, set non-blocking with `fd_set_nb`
   - Listen with `SOMAXCONN`
2. Event loop:
   - Build `poll_args`:
     - index 0: listening socket (POLLIN)
     - indices 1..N: each active connection fd with `POLLERR` plus `POLLIN`/`POLLOUT` per `want_*`
   - Timeout: `timeout_ms = next_timer_ms()` from `sys_server.cpp`
   - `poll(poll_args, timeout_ms)`
   - If index 0 ready: `handle_accept(listen_fd)`
     - Accept, set non-blocking
     - Allocate `Connection`, set `want_read=true`, init timestamps
     - Insert into `server_data.fd2conn` at index = `socket_fd`
     - Link into `idle_conn_list` just before the sentinel tail (i.e., newest activity at tail)
   - For each ready client fd:
     - Refresh activity time and move idle node to tail (detach+insert_before)
     - If `POLLIN` and `want_read`: `handle_read(conn)`
     - If `POLLOUT` and `want_write`: `handle_write(conn)`
     - If `POLLERR` or `conn->want_close`: `handle_destroy(conn)`
   - Finally, `process_timers()` to close expired idle connections in time order

### Connection read side (netio.cpp)
1. `handle_read(conn)`:
   - `read()` up to 64KB → append to `conn->incoming`
   - On `EAGAIN` return early; on error set `want_close`
   - On EOF: if `incoming` empty → client closed; else unexpected EOF → `want_close`
   - While there is at least one complete frame in `incoming`, `handle_one_request(conn)`
   - If response data exists in `outgoing`, switch to write intent and opportunistically call `handle_write(conn)` once

2. `handle_one_request(conn)`:
   - Require at least 4 bytes to read `frame_len`
   - If `frame_len > k_max_msg` → mark `want_close`
   - If full payload not yet available → return false
   - Parse payload with `parse_request` (argv):
     - On parse error: generate `ERR_BAD_ARG "malformed request"`, consume the frame, return true
   - Begin response frame (`response_begin`)
   - Call `run_request(cmd, conn->outgoing)` (storage/commands.cpp)
   - `response_end` to finalize frame length
   - Consume the input frame from `incoming`
   - Return true

### Connection write side (netio.cpp)
1. `handle_write(conn)`:
   - `write()` as many bytes from `outgoing` as the kernel accepts
   - On partial write, remove the portion written and keep `want_write=true`
   - When fully written: `want_write=false`, `want_read=true`

### Request protocol (protocol.cpp)
- Client encodes argv:
  - `num_args:u32`, followed by `[len:u32][bytes...]` per argument in order
- The whole payload is then framed with a 4-byte length header

### Response protocol (serialize.cpp, client.cpp)
- Server serializes typed values with tag + payload
- Client reads the frame, peels off the typed payload, and prints in human-readable form
- Arrays are flat sequences of nested typed items (e.g., `["n1", 1.1, "n2", 2]`)

## Command Layer (storage/commands.cpp)
### Dispatcher
```text
run_request(cmd, resp):
  ping
  get <key>
  set <key> <value>
  del <key>
  keys
  zadd <key> <score> <member>
  zrem <key> <member>
  zscore <key> <member>
  zquery <key> <score> <name> <offset> <limit>
```

### String KV design
- Top-level `db` stores keys; each `Entry` holds:
  - `type` (string or zset)
  - `value` (string) or `zset`
- `set`: insert or update string value
- `get`: fetch string, type-check
- `del`: delete the whole entry (uses `entry_del` to free zset internals if needed)
- `keys`: array of strings `"key : value"` (for demo visibility)

### ZSet design
- Keys of type zset hold a `ZSet` (tree + by-name index)
- `zadd`: add or update a member’s score
- `zrem`: remove a member
- `zscore`: print the score or `nil`
- `zquery <zkey> <score> <name> <offset> <limit>`:
  - Find first tuple ≥ `(score, name)`
  - Offset by rank
  - Emit an array alternating `[name, score, name, score, ...]` up to limit pairs

## Timers and Idle Connections
### Why the double-linked list?
- We maintain an ordered list of connections by most recent activity (tail is newest)
- On activity, we move a connection to the tail
- The head’s next node is the oldest; its timeout determines the next poll timeout
- On `process_timers`, we iterate from oldest and close expired connections

### Timer APIs
- `next_timer_ms()`:
  - If list empty → -1 (no timeout)
  - Else compute deadline for oldest connection; return 0 if already overdue (immediate wake)
- `process_timers()`:
  - Repeatedly close oldest connections while their deadlines are ≤ now

## Memory and Lifetime
- Connections:
  - Allocated on accept (`new Connection`)
  - Freed in `handle_destroy`:
    - Close fd
    - Remove from `fd2conn`
    - Detach from idle list
    - `delete conn`
- ZSet nodes:
  - Allocated with `malloc` (flexible array for name)
  - Freed via `znode_del` (and tree free in `zset_clear`)
- Entries:
  - Freed via `entry_del` (important: clears zset internals if type is zset)

## Error Handling and Safety
- Networking:
  - Non-blocking I/O with `EAGAIN` handling
  - Frame size cap `k_max_msg` on both client and server
  - Malformed request yields typed error `ERR_BAD_ARG`
- Hashtable:
  - Progressive rehash ensures O(1) amortized operations without pauses
- Bounds:
  - ZSet names and scores validated; doubles parsed with `strtod`, ints with `strtoll`

## Wire-Level Notes
- Length fields are copied as raw 4-byte values; within this project both sides agree on representation (same architecture). For portability across architectures, convert to network byte order.
- Response tags are single-byte identifiers followed by type-specific payloads.

## Event Loop State Machine (High Level)
```
accept new client  →  create Connection  →  want_read=true
         │
         ▼
poll(POLLIN/POLLOUT) ──► if POLLIN ∧ want_read: handle_read
         │                              │
         │                              ├─ append incoming
         │                              ├─ while full frame:
         │                              │     parse → run_request → serialize → queue outgoing
         │                              └─ if outgoing: want_write=true; handle_write
         │
         └──► if POLLOUT ∧ want_write: handle_write
                                        ├─ write some bytes
                                        └─ if done: want_write=false; want_read=true
```

## Testing
- Unit-like tests for AVL tree and offset: `make test-avl`, `make test-offset`
- Integration test for commands using the client REPL: `make test-cmds`
  - Spawns server, runs `tests/test_cmds.py` which pushes REPL commands and asserts output

## Design Rationale
- Single-threaded poll-based I/O keeps the design simple and debuggable
- Intrusive nodes (`HNode`, `AVLNode`) reduce allocations and allow quick unlink/relink
- Progressive rehash avoids long pauses due to resizing
- Typed responses make the protocol robust and extensible

## Extensibility Notes
- Add more types (lists/hashes/sets) by extending `Entry` and `run_request`
- Expose TTL (expire) by augmenting `Connection` or adding per-key metadata
- Make framing portable (htonl/ntohl) for cross-architecture clients
