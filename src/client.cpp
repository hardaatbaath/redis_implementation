// C stdlib
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

// POSIX / system (socket API, inet helpers, read/write)
#include <unistd.h>      // read, write, close
#include <arpa/inet.h>   // htons, htonl, ntohs, ntohl
#include <netinet/in.h>  // sockaddr_in
#include <sys/socket.h>  // socket, connect, bind

#include "utils.h"

const size_t k_max_msg = 32 << 20;

/**
 * Send a request to the server
 */
static int32_t send_request(int fd, const uint8_t *text, size_t len){
    if (len > k_max_msg) { return -1; }

    std::vector<uint8_t> wbuf;
    append_buffer(wbuf, (const uint8_t*)&len, 4);
    append_buffer(wbuf, text, len);

    return write_all(fd, (char*) wbuf.data(), wbuf.size());
}

/**
 * Read a response from the server
 */
static int32_t read_response(int fd){
    // 4 bit header
    std::vector<uint8_t> rbuf;
    rbuf.resize(4);
    errno = 0;

    int32_t err = read_full(fd, (char*) rbuf.data(), 4);
    if (err) { 
        if (errno == 0) {msg("unexpected EOF");} 
        else {msg("read() error");} 
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, rbuf.data(), 4);
    if (len > k_max_msg) { msg("response too long"); return -1; }

    rbuf.resize(4 + len);
    err = read_full(fd, (char*) rbuf.data() + 4, len);
    if (err) {
        msg("read() error"); 
        return err;
    }

    rbuf[4 + len] = '\0';
    printf("Server says: %s\n", (char*) rbuf.data() + 4);
    return 0;
}

/**
 * Main function
 * Connect to the server
 * Send a message to the server
 * Receive a message from the server
 * Close the connection
 * Return 0
 */
int main() {

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
    if (getsockname(fd, (struct sockaddr *)&bound_addr, &bound_len) == 0) {
        char ipbuf[INET_ADDRSTRLEN] = {0};
        const char *ipstr = inet_ntop(AF_INET, &bound_addr.sin_addr, ipbuf, sizeof(ipbuf));
        char buf[128];
        snprintf(buf, sizeof(buf), "[client] bind successful on %s:%u",
                 ipstr ? ipstr : "127.0.0.1", (unsigned)ntohs(bound_addr.sin_port));
        msg(buf);
    } else {
        msg("[client] bind successful on 127.0.0.1:<ephemeral>");
    }

    // bind, in `connect` the automatically bind the socket to the address
    // if port or addr not given, the automatically available port and addr would be used to bind the socket to the address
    rv = connect (fd, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    if (rv < 0) { die("connect()"); }
    msg("[client] connected to 127.0.0.1:8080");

    // send multiple pipelined requests to the server
    std::vector<std::string> requests = {
        "hello1", "hello2", "hello3",
        // a large message requires multiple event loop iterations
        std::string(k_max_msg, 'z'),
        "hello5",
    };
    
    for (const std::string& request : requests) {
        int32_t err = send_request(fd, (const uint8_t*) request.data(), request.size());
        if (err) { goto L_DONE; }
    }

    // read the response from the server
    for (size_t i = 0; i < requests.size(); i++) {
        int32_t err = read_response(fd);
        if (err) { goto L_DONE; }
    }

    L_DONE:
        // close the connection
        close(fd);
        return 0;
}