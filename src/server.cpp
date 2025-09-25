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

// /**
//  * Print a message to stderr
//  */
// static void msg(const char *msg) {
//     fprintf(stderr, "%s\n", msg); 
// }

// /**
//  * Print an error message to stderr and exit the program
//  */
// static void die(const char *msg) {
//     int err = errno;
//     fprintf(stderr, "[%d] %s\n", err, msg);
//     abort();
// }

/**
 * Handle the client data handling
 */
static void do_something(int conn_fd) {
    char rbuf[64] = {};

    ssize_t n = read (conn_fd, rbuf, sizeof(rbuf) - 1); // can be replaced with recv(), requires additional flags
    if (n < 0) {
        msg("read() error");
        return;
    }
    printf("Client says: %s\n", rbuf);

    char wbuf[] = "Hello to you too, from the server."; // sizeof also includes the null terminator
    write(conn_fd, wbuf, strlen(wbuf)); // can be replaced with send(), requires additional flags
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

        // handle the client connection
        do_something(client_fd); 
        close(client_fd);
    }

    return 0;
}