# redis-from-scratch-c

Minimal Redis-like TCP server/client (C++17) focused on learning networking, framing, and a simple in-memory key-value store with incremental rehashing.

## Prerequisites
- macOS or Linux
- make
- g++ (C++17)

Verify:
```bash
g++ --version
make --version
```

## Build
From the repo root:
```bash
make
```
- Binaries are placed in `bin/`
- Objects and dependency files go to `build/`

## Run
Use two terminals:

- Terminal 1 (server):
```bash
make run-server
```

- Terminal 2 (client):
```bash
make run-client
```

You should see logs from both processes indicating connection, send, and receive events. The client is an interactive REPL.

Examples in the client REPL:
```text
> ping
pong
> set greeting hello
> get greeting
hello
> all keys
greeting hello
> del greeting
> get greeting
  # status 404 (printed to stderr)
> quit
```

## Clean / Rebuild
```bash
make clean      # remove build/ and bin/
make rebuild    # clean + build
```

## Configuration
- Default port: 8080
- To change the port, edit both:
  - `src/server.cpp` (server bind port)
  - `src/client.cpp` (client connect port)
- Rebuild after changes: `make`

## Project Structure
```bash
src/
  client.cpp                  # client entrypoint (interactive REPL)
  server.cpp                  # server entrypoint (poll-based event loop)

  core/
    sys.h / sys.cpp           # logging, die(), non-blocking fd, read_all/write_all, buffer helpers
    constants.h               # k_max_msg, k_max_args, load factor, rehashing work

  net/
    netio.h / netio.cpp       # Connection type and I/O state machine (read/parse/execute/write)
    protocol.h / protocol.cpp # argv-style request framing + response framing

  storage/
    hashtable.h / .cpp        # chaining hash table with incremental rehashing (older/newer tables)
    commands.h / .cpp         # key/value commands and command dispatcher (run_request)

Makefile                      # build rules and run targets
README.md                     # this file
```

## Wire Protocol
The client sends argv-framed requests; the server replies with a framed response.

- Request (client → server):
  - Frame: `[payload_len:u32][payload_bytes...]`
  - Payload: `[num_args:u32][len:u32 arg0][bytes...][len:u32 arg1][bytes...]...`

- Response (server → client):
  - Frame: `[payload_len:u32][payload_bytes...]`
  - Payload: `[status:u32][data:bytes...]`

Limits and safety checks:
- `k_max_msg` caps frame sizes (default 32 MiB)
- `k_max_args` caps number of argv items per request

## Supported Commands
- `ping` → returns `pong`
- `set <key> <value>` → stores/updates a value
- `get <key>` → prints the value; status `404` if missing
- `del <key>` → deletes the key; status `404` if missing
- `all keys` → prints all `key value` pairs (one per line)

## Architecture Overview
- Non-blocking server using `poll(2)` to multiplex connections.
- Each connection has input/output buffers and readiness flags (`want_read`, `want_write`).
- Read side: bytes are appended to the input buffer; when a full frame is available it is parsed and executed.
- Write side: responses are queued to the output buffer; partial writes are handled and the remainder is retried when writable.

### Modules
- `core/` utilities: logging, error handling, `fd_set_nb`, `read_all`/`write_all`, and small vector helpers (`append_buffer`, `consume_buffer`).
- `net/protocol.*`: request parsing and response generation (no business logic here).
- `net/netio.*`: orchestrates parsing, command execution, and response generation per connection.
- `storage/hashtable.*`: open-addressed chaining table with two tables (`older`, `newer`) for incremental rehashing; controlled by `k_max_load_factor` and `k_rehashing_work`.
- `storage/commands.*`: command handlers (`get`, `set`, `del`, `all keys`, `ping`) operating against a global `HashMap`.

### Incremental Rehashing
When load factor exceeds a threshold, a new table is allocated and keys are gradually migrated from `older` to `newer` over subsequent operations. Reads/writes:
- Always insert into `newer`.
- Lookups check `newer` then `older`.
- Deletions attempt `older` then `newer`.
- `all keys` enumerates both tables to avoid missing entries mid-migration.

## Development Notes
- Code style favors clarity and explicitness. Short helper functions for buffer operations reduce duplication.
- Logging: `core/sys.*` provides `msg` and `msg_error` for uniform logs. The client prints responses to stdout and status to stderr.

## Troubleshooting
- Address already in use: stop any previous server on port 8080 or change the port in both `src/server.cpp` and `src/client.cpp` then `make rebuild`.
- Compiler not found: install Xcode Command Line Tools on macOS (`xcode-select --install`) or your distro's build tools on Linux.
- No output from `get`: ensure you `set` the key first; a missing key returns status `404` (logged to stderr in the client).