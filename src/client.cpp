#include <assert.h> // for assert
#include <stdint.h> // for buffer size, and int types
#include <stdlib.h> // for malloc, free, exit
#include <string.h> // for strlen, memset, strcpy
#include <stdio.h> // for printf, perror
#include <errno.h> // for errno, strerror
#include <unistd.h> // for close, read, write
#include <arpa/inet.h> // for inet_addr, inet_pton
#include <sys/socket.h> // for socket, connect, bind
#include <netinet/ip.h> // for IPPROTO_TCP,struct iphdr

#include "utils.h"

const size_t k_max_msg = 4096;

static int32_t query(int fd, const char *text){
    uint32_t len = (uint32_t)strlen(text); // used to send and receive the length of the message
    if (len > k_max_msg) {
        msg("message too long");
        return -1;
    }

    //request buffer
    char wbuf[4 + k_max_msg];
    memcpy(wbuf, &len, 4); // copies the 4 bytes from len to wbuf
    memcpy(&wbuf[4], text, len); // copies the text to wbuf after the header
    
    if (int32_t err = write_all(fd, wbuf, 4 + len)) {
        msg("write() error");
        return err;
    }

    //response buffer
    char rbuf[4 + k_max_msg] = {};
    errno = 0;
    if (int32_t err = read_full(fd, rbuf, 4)) {
        msg(errno == 0 ? "unexpected EOF" : "read() error");
        return err;
    }
    
    // Storing the read values
    memcpy(&len, rbuf, 4); // get the length of the response
    if (len > k_max_msg) {
        msg("response too long");
        return -1;
    }

    //response buffer
    if (int32_t err = read_full(fd, &rbuf[4], len)) {
        msg("read() error");
        return err;
    }

    rbuf[4 + len] = '\0'; // explicitely adding the null terminator
    printf("Server says: %s\n", &rbuf[4]);
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
    char msg[] = "Hello, this is a test message designed to exceed sixty-four bytes in length for buffer validation purposes.";
    int32_t err = query(fd, msg);
    if (err) { goto L_DONE; }
    
    // interactive input loop
    while (true) {
        char input[k_max_msg + 1] = {};
        fprintf(stderr, "> ");
        if (!fgets(input, sizeof(input), stdin)) {
            fprintf(stderr, "[client] stdin closed, exiting.\n");
            break;
        }
        size_t input_len = strlen(input);
        if (input_len > 0 && input[input_len - 1] == '\n') {
            input[input_len - 1] = '\0';
        }
        if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
            fprintf(stderr, "[client] exiting on user request.\n");
            break;
        }
        if (input[0] == '\0') {
            continue;
        }
        int32_t errr = query(fd, input);
        if (errr) { break; }
    }

    L_DONE:
        // close the connection
        close(fd);
        return 0;
}