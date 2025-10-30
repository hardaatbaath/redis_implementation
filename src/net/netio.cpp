// C stdlib
#include <errno.h>       // errno (handle_read, handle_write)
#include <string.h>      // memcpy (handle_one_request)
#include <assert.h>      // assert (handle_write)

// POSIX / system
#include <unistd.h>      // read, write (handle_read, handle_write)

// C++ stdlib
#include <vector>        // std::vector (Connection buffers)

// local
#include "netio.h"              // Connection, handle_* declarations
#include "protocol.h"           // parse_request, generate_response, Response
#include "../storage/commands.h" // run_request
#include "../core/constants.h"  // k_max_msg
#include "../core/sys.h"        // msg_error
#include "../core/buffer_io.h"  // Buffer, append_buffer, consume_buffer
#include "../net/serialize.h"   // out_err, append_buffer_u32

// Begin the response
static void response_begin(Buffer &out, size_t *header) {
    *header = out.size();       // messege header position
    append_buffer_u32(out, 0);     // reserve space
}

// Get the size of the response
static size_t response_size(Buffer &out, size_t header) {
    return out.size() - header - 4;
}

// End the response
static void response_end(Buffer &out, size_t header) {
    size_t msg_size = response_size(out, header);
    if (msg_size > k_max_msg) {
        out.resize(header + 4);
        out_err(out, ERR_TOO_BIG, "response is too big.");
        msg_size = response_size(out, header);
    }
    // message header
    uint32_t len = (uint32_t)msg_size;
    memcpy(&out[header], &len, 4);
}

// Process one request when there is enough data
bool handle_one_request(Connection *conn) {
    // try to parse the protocol: message header
    if (conn->incoming.size() < 4) { return false; } // we don't even know the size of the message

    uint32_t frame_len = 0;
    memcpy(&frame_len, conn->incoming.data(), 4);
    if (frame_len > k_max_msg) {
        msg_error("too long");
        conn->want_close = true;
        return false;
    }
    
    // Get the message payload
    if (4 + frame_len > conn->incoming.size()) { return false; } // size of the payload is incorrect
    const uint8_t *request = conn->incoming.data() + 4;

    // Parse the request and apply application logic
    std::vector<std::string> cmd;
    if (parse_request(request, frame_len, cmd) < 0) {
        // Malformed request: respond with an error instead of closing the connection
        size_t header = 0;
        response_begin(conn->outgoing, &header);
        out_err(conn->outgoing, ERR_UNKNOWN, "malformed request");
        response_end(conn->outgoing, header);
        consume_buffer(conn->incoming, 4 + frame_len);
        return true;
    }

    // Begin the response
    size_t header = 0;
    response_begin(conn->outgoing, &header);

    // Run the request
    run_request(cmd, conn->outgoing);

    // End the response
    response_end(conn->outgoing, header);

    // Consume the incoming data
    consume_buffer(conn->incoming, 4 + frame_len);
    return true;
}

// Application callback when the socket is writable
void handle_write(Connection *conn) {
    assert(!conn->outgoing.empty()); // check if there is any outgoing data

    // write the response to the socket, outgoing[0] is the pointer to the buffer
    ssize_t rv = write(conn->socket_fd, &conn->outgoing[0], conn->outgoing.size());
    if (rv < 0) {
        if (errno == EAGAIN) { return; } // actually not ready to write as the buffer is full

        msg_error("write() error"); // write() error
        conn->want_close = true;
        return;
    }

    // remove the written data from the outgoing buffer
    consume_buffer(conn->outgoing, (size_t)rv);

    // update the readiness flag
    if (conn->outgoing.size() == 0) { // if there is no outgoing data, we want to read
        conn->want_read = true;
        conn->want_write = false;
    }
}

/**
 * Application callback when the socket is readable
 * Read the request and parse it
 * Generate the response
 * Write the response to the socket
*/
void handle_read(Connection *conn) {
    // read the request [4b header + payload]
    uint8_t buf[64 * 1024]; // 64KB buffer
    ssize_t rv = read(conn->socket_fd, buf, sizeof(buf));
    if (rv < 0) {
        if (errno == EAGAIN) { return; } // actually not ready

        // handle IO errors
        msg_error("read() error");
        conn->want_close = true;
        return; // want to close the connection
    }

    // Handle EOF, closing as we are done with the connection
    if (rv == 0) {
        if (conn->incoming.empty()) { msg("[server] client closed connection"); }
        else { msg("unexpected EOF"); }
        conn->want_close = true;
        return;
    }
    
    // append the incoming data to the buffer
    append_buffer(conn->incoming, buf, (size_t)rv);

    // parse the request and generate response, in a while loop as there may be multiple requests in the buffer
    while (handle_one_request(conn)) {}

    // update the readiness flag
    if (!conn->outgoing.empty()) { // if there is outgoing data, we want to write
        conn->want_read = false;
        conn->want_write = true;

        // The socket is likely ready to write in a request-response protocol,
        // try to write it without waiting for the next iteration.
        return handle_write(conn);
    }
}