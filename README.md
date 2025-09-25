# redis-from-scratch-c

Minimal TCP server/client (C++17) for learning the building blocks behind Redis-like networking. The server listens on port 8080 and replies "world" to a client greeting.

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

You should see logs from both processes indicating connection, send, and receive events.

## Clean / Rebuild
```bash
make clean      # remove build/ and bin/
make rebuild    # clean + build
```

## Configuration
- Default port: 8080
- To change the port, edit both:
  - `src/server.cpp` (the server bind port)
  - `src/client.cpp` (the client connect port)
- Rebuild after changes: `make`

## Project Structure
```bash
src/
  client.cpp        # client entrypoint
  server.cpp        # server entrypoint
  utils.h           # shared logging/error helpers
  utils.cpp
Makefile            # builds and runs server/client
README.md
```

## Troubleshooting
- Address already in use: stop any previous server using port 8080, or change the port in both `src/server.cpp` and `src/client.cpp`, then `make rebuild`.
- Compiler not found: install Xcode Command Line Tools on macOS (`xcode-select --install`) or your distro's build tools on Linux.