#include <assert.h> // for assert
#include <stdint.h> // for buffer size, and int types
#include <stdlib.h> // for malloc, free, exit
#include <string.h> // for strlen, memset, strcpy
#include <stdio.h> // for printf, perror
#include <errno.h> // for errno, strerror
#include <unistd.h> // for close, read, write
#include <arpa/inet.h> // for inet_addr, inet_pton
#include <sys/socket.h> // for socket, bind, listen, accept, setsockopt
#include <netinet/ip.h> // for IPPROTO_TCP,struct iphdr

#include "utils.h"

const size_t k_max_msg = 4096;

/**
 * Handle the client data handling multiple requests
 */
static int32_t one_request(int conn_fd) {
    // read the request [4b header + payload]
    char rbuf[4 + k_max_msg] = {};
    errno = 0;
    int32_t err = read_full(conn_fd, rbuf, 4); // read the 4 bytes header
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    // read the length of the payload
    uint32_t len = 0;
    memcpy(&len, rbuf, 4); //copies the 4 bytes from rbuf to len
    if (len > k_max_msg) {
        msg("too long");
        return -1;
    }

    // read the payload
    err = read_full(conn_fd, &rbuf[4], len); // reads from after the header
    if (err) {
        msg("read() error");
        return err;
    }
    fprintf(stderr, "[server] received %u bytes: %.*s\n", (unsigned)len, (int)len, &rbuf[4]);

    // write the response [4b header + payload]
    const char reply[] = "Hello to you too, from the server.";
    char wbuf[4 + sizeof(reply)]; //sizeof instead of strlen as strlen is static and strlen is compiled at runtime
    len = (uint)strlen(reply);

    memcpy(wbuf, &len, 4); // copies the 4 bytes from len to wbuf
    memcpy(wbuf + 4, reply, len); // copies the reply to wbuf after the header, wbuf + 4 == &wbuf[4]
    return write_all(conn_fd, wbuf, 4 + len);
}

/**
 * Main function
 * Listen to the port 8080 for client connections
 * Handle the client connection
 * Close the client connection
 * Return 0
 */
int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { die("socket()"); }
    msg("[server] socket created");

    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)); // set the port to reusable when the program is restarted

    // declare the socket connection
    struct sockaddr_in addr ={};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);  //port
    addr.sin_addr.s_addr = htonl(0); //wildcard ip 0.0.0.0    INADDR_ANY; // listen to all addresses

    // bind
    int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv) { die("bind()"); } 
    msg("[server] bind successful on 0.0.0.0:8080");

    // listen
    rv = listen(fd, SOMAXCONN);
    if (rv) {die("listen()");} 
    msg("[server] listen successful on 0.0.0.0:8080");

    while (true) {
        // accept
        struct sockaddr_in client_addr = {};
        socklen_t client_addr_len = sizeof(client_addr);

        // Here we pass the struct of sockaddr for client address, along with the length of the struct
        int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_addr_len); 
        if (client_fd < 0) { continue; /*error handling*/ }

        // get the client ip address
        char client_ip[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        fprintf(stderr, "[server] accepted connection from %s:%u\n", client_ip, (unsigned)ntohs(client_addr.sin_port));

        // handle the client connection
        while (true) {
            int32_t err = one_request(client_fd); // This takes multiple requests in the same connection
            if (err) { break; }
        }
        close(client_fd);
    }

    return 0;
}