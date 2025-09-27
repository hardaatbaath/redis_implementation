#include <stdint.h> // for buffer size, and int types
#include <stdlib.h> // for malloc, free, exit
#include <string.h> // for strlen, memset, strcpy
#include <stdio.h> // for printf, perror
#include <errno.h> // for errno, strerror
#include <unistd.h> // for close, read, write
#include <arpa/inet.h> // for inet_addr, inet_pton
#include <sys/socket.h>
#include <netinet/ip.h> // for IPPROTO_TCP,struct iphdr

#include "utils.h"


// /**
//  * Print an error message to stderr and exit the program
//  */
// static void die(const char *msg) {
//     int err = errno;
//     fprintf(stderr, "[%d] %s\n", err, msg);
//     abort();
// }

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

    // declare the socket connection
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(8080);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK); // loopback address 127.0.0.1

    // bind, in `connect` the automatically bind the socket to the address
    // if port or addr not given, the automatically available port and addr would be used to bind the socket to the address
    int rv = connect (fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv < 0) { die("connect()"); }
    msg("[client] connected to 127.0.0.1:8080");

    // send a message to the server
    // char msg[] = "Hello from the other side.";
    char msg[] = "Hello, this is a test message designed to exceed sixty-four bytes in length for buffer validation purposes.";
    write(fd, msg, strlen(msg)); //sizeof requires -1

    // receive a message from the server
    char rbuf[64] = {}; 

    // While read, we are doing sizeof(rbuf) - 1 to ensure that the buffer is not completely filled
    // If the buffer is completely filled, the buffer would not have null terminator in the end and while
    // printing the buffer, it would print beyond the buffer into garbage values.
    ssize_t n = read(fd, rbuf, sizeof(rbuf) - 1); 
    if (n < 0) { die("read()"); }

    rbuf[n] = '\0'; // explicitely adding the null terminator
    printf("Server says: %s\n", rbuf);

    // close the connection
    close(fd);
    return 0;
}