// C stdlib
#include <stdint.h>      // uint8_t, uint32_t (framing)
#include <string.h>      // memcpy (response parsing)
#include <stdio.h>       // printf, fprintf (logging)
#include <errno.h>       // errno (read_response)

// POSIX / system (socket API, inet helpers, read/write)
#include <unistd.h>      // read, write, close
#include <arpa/inet.h>   // htons, htonl, ntohs, ntohl
#include <netinet/in.h>  // sockaddr_in
#include <sys/socket.h>  // socket, connect, bind

// C++ stdlib
#include <string>        // std::string
#include <vector>        // std::vector
#include <sstream>       // std::istringstream
#include <iostream>      // std::getline

// local
#include "core/sys.h"        // msg, die
#include "core/buffer_io.h"  // Buffer, append_buffer
#include "net/netio.h"   // read_all, write_all
#include "net/serialize.h"   // out_nil, out_err, out_str, out_int, out_dbl, out_bool, out_arr, out_map
#include "core/constants.h"  // k_max_msg

/**
 * Send argv-framed request to the server
 * payload = [num_args:u32][len:u32 arg0][bytes...][len:u32 arg1][bytes...] ...
 * frame   = [payload_len:u32][payload_bytes]
*/
static int32_t send_request(int fd, const std::vector<std::string> &cmd){
    std::vector<uint8_t> payload;
    uint32_t num_args = (uint32_t)cmd.size();

    // append the number of arguments
    append_buffer(payload, (const uint8_t*)&num_args, 4);

    // append the arguments
    for (const std::string &arg : cmd) {
        uint32_t arg_len = (uint32_t)arg.size();
        append_buffer(payload, (const uint8_t*)&arg_len, 4);
        if (arg_len) { append_buffer(payload, (const uint8_t*)arg.data(), arg_len); }
    }

    uint32_t payload_len = (uint32_t)payload.size();
    if (payload_len > k_max_msg) { return -1; }

    std::vector<uint8_t> frame;
    append_buffer(frame, (const uint8_t*)&payload_len, 4);
    append_buffer(frame, payload.data(), payload.size());
    return write_all(fd, (char*) frame.data(), frame.size());
}

// Print the response
int32_t print_response(const uint8_t *data, size_t size){
    if (size < 1) { msg("bad response"); return -1; }

    switch (data[0]) {
        case TAG_NIL:
            printf("nil\n");
            return 1;

        // 1 byte: for the tag
        // 4 bytes: for the error code
        // 4 bytes: for the message length
        // message length bytes: for the message
        case TAG_ERR:
            if (size < 1 + 8) { msg("bad response"); return -1; }

            {
                int32_t code = 0;
                uint32_t len = 0;
                memcpy(&code, &data[1], 4);
                memcpy(&len, &data[1 + 4], 4);

                if (size < 1 + 8 + len) { msg("bad response"); return -1; }

                printf("error %d: %.*s\n", code, len, &data[1 + 8]);
                return 1 + 8 + len;
            }

        // 1 byte: for the tag
        // 4 bytes: for the string length
        // string length bytes: for the string
        case TAG_STR:
            if (size < 1 + 4) { msg("bad response"); return -1; }

            {
                uint32_t len = 0;
                memcpy(&len, &data[1], 4);

                if (size < 1 + 4 + len) { msg("bad response"); return -1; }
            
                printf("%.*s\n", len, &data[1 + 4]);
                return 1 + 4 + len;
            }

        // 1 byte: for the tag
        // 8 bytes: for the integer
        case TAG_INT:
            if (size < 1 + 8) { msg("bad response"); return -1; }

            {
                int64_t value = 0;
                memcpy(&value, &data[1], 8);

                printf("%lld\n", value); // long long because the size of long varies between different architectures
                return 1 + 8;
            }

        // 1 byte: for the tag
        // 8 bytes: for the double
        case TAG_DBL:
            if (size < 1 + 8) { msg("bad response"); return -1; }

            {
                double value = 0;
                memcpy(&value, &data[1], 8);
                
                printf("%g\n", value);
                return 1 + 8;
            }

        // 1 byte: for the tag
        // 1 byte: for the boolean value
        case TAG_BOOL:
            if (size < 1 + 1) { msg("bad response"); return -1; }

            {
                uint8_t value = 0;
                memcpy(&value, &data[1], 1);

                printf("%s\n", value ? "true" : "false");
                return 1 + 1;
            }

        // 1 byte: for the tag
        // 4 bytes: for the array length
        // array length bytes: for the array
        case TAG_ARR:
            if (size < 1 + 4) { msg("bad response"); return -1; }

            {
                uint32_t len = 0;
                memcpy(&len, &data[1], 4);
                printf("array length: %d\n", len);

                size_t arr_bytes = 1 + 4;
                for (uint32_t i = 0; i < len; i++) {
                    int32_t rv = print_response(&data[arr_bytes], size - arr_bytes);
                    if (rv <= 0) { msg("bad response"); return -1; }
                    arr_bytes += rv;
                }
                
                printf("array end\n");
                return arr_bytes;
            }

        // 1 byte: for the tag
        // 4 bytes: for the map length
        // map length bytes: for the map
        case TAG_MAP:
            if (size < 1 + 4) { msg("bad response"); return -1; }

            {
                uint32_t len = 0;
                memcpy(&len, &data[1], 4);
                // printf("map length: %d: ", len);

                size_t map_bytes = 1 + 4;
                for (uint32_t i = 0; i < len; i++) {
                    int32_t rv = print_response(&data[map_bytes], size - map_bytes);
                    if (rv <= 0) { msg("bad response"); return -1; }
                    map_bytes += rv;
                }

                printf("map end\n");
                return map_bytes;
            }

        default:
            msg("bad response");
            return -1;
    }
}

// Read a response from the server
static int32_t read_response(int fd){
    // Read frame header (payload length)
    std::vector<uint8_t> rbuf(4);
    errno = 0;
    int32_t err = read_all(fd, (char*)rbuf.data(), 4);
    if (err) {
        if (errno == 0) { msg("unexpected EOF"); }
        else { msg("read() error"); }
        return err;
    }

    uint32_t payload_len = 0;
    memcpy(&payload_len, rbuf.data(), 4); // Assuming little endian
    if (payload_len > k_max_msg) { msg("bad response length"); return -1; }

    // Resize buffer to hold header + payload, then read payload
    rbuf.resize(4 + payload_len);
    err = read_all(fd, (char*)rbuf.data() + 4, payload_len);
    if (err) { msg("read() error"); return err; }

    // Parse the response
    int32_t rv = print_response((uint8_t *) &rbuf[4], payload_len);
    if (rv > 0 && (uint32_t)rv != payload_len) { msg("bad response"); return -1; }

    return rv;
}

/**
 * Main function
 * Connect to the server
 * Send a message to the server
 * Receive a message from the server
 * Close the connection
 * Return 0
*/
int main() {//int argc, char *argv[]) {

    // create a socket
    int fd = socket(AF_INET, SOCK_STREAM, 0); // 0 is the default protocol
    if (fd < 0) { die("socket()"); }
    msg("[client] socket created");

    // declare the server's socket connection
    struct sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // loopback address 127.0.0.1

    // declare the client's socket connection   
    struct sockaddr_in client_addr = {};
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(0); // ephemeral port
    client_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1

    // make the socket reusable
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)); // set the port to reusable when the program is restarted

    int rv = bind(fd, (const struct sockaddr *)&client_addr, sizeof(client_addr));
    if (rv < 0) { die("bind()"); }
    
    // log the actual bound ephemeral port
    struct sockaddr_in bound_addr = {};
    socklen_t bound_len = sizeof(bound_addr);
    
    // get the actual bound ephemeral port
    if (getsockname(fd, (struct sockaddr *)&bound_addr, &bound_len) == 0) {
        char ipbuf[INET_ADDRSTRLEN] = {0};

        // convert the IP address to a string
        const char *ipstr = inet_ntop(AF_INET, &bound_addr.sin_addr, ipbuf, sizeof(ipbuf));

        // format the IP address and port
        char buf[128];
        snprintf(buf, sizeof(buf), "[client] bind successful on %s:%u",
                 ipstr ? ipstr : "127.0.0.1", (unsigned)ntohs(bound_addr.sin_port));

        // log the actual bound ephemeral port
        msg(buf);
    } 
    else {
        msg("[client] bind successful on 127.0.0.1:<ephemeral>");
    }

    // bind, in `connect` the automatically bind the socket to the address
    // if port or addr not given, the automatically available port and addr would be used to bind the socket to the address
    rv = connect (fd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    if (rv < 0) { die("connect()"); }
    msg("[client] connected to 127.0.0.1:8080");

    // // send one argv-framed request from CLI args
    // std::vector<std::string> request;

    // for (int i = 1; i < argc; i++) { request.push_back(argv[i]); }

    // if (request.empty()) { 
    //     msg("usage: client CMD [ARGS...]"); 
    //     close(fd);
    //     return 0;
    // }

    // interactive REPL: read commands from stdin and send to server
    while (true) {
        fprintf(stderr, "> ");
        std::string line;
        if (!std::getline(std::cin, line)) {
            break; // EOF or input error
        }
        if (line.empty()) { continue; }

        // tokenize by whitespace
        std::istringstream iss(line);
        std::vector<std::string> request;
        for (std::string tok; iss >> tok; ) { request.push_back(tok); }
        if (request.empty()) { continue; }
        if (request[0] == "exit" || request[0] == "quit") { break; }

        // send the request to the server
        int32_t err = send_request(fd, request);
        if (err) { 
            msg("write() error"); 
            goto L_DONE; 
        }

        // read the response from the server
        err = read_response(fd);
        if (err < 0) { 
            msg("read() error"); 
            goto L_DONE; 
        }
    }

    L_DONE:
        // close the connection
        close(fd);
        return 0;
}