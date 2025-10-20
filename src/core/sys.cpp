// C stdlib
#include <assert.h>  // assert (read_all, write_all)
#include <errno.h>   // errno  (die, fd_set_nb)
#include <stdint.h>  // standard types
#include <stdio.h>   // fprintf (msg, msg_error, die)
#include <stdlib.h>  // abort (die)

// POSIX / system
#include <unistd.h>  // read, write (read_all, write_all)
#include <fcntl.h>   // fcntl (fd_set_nb)

// C++ stdlib
#include <vector>    // std::vector (append_buffer, consume_buffer)

// local
#include "sys.h"    // public API declarations

/**
 * Print a message to stderr
*/
void msg(const char* message) {
    fprintf(stderr, "%s\n", message);
}

/**
 * Print an error message to stderr
*/
void msg_error(const char* message) {
    fprintf(stderr, "[ERROR] %s\n", message);
}

/**
 * Print an error message to stderr and exit the program
*/
[[noreturn]] void die(const char* context) {
    const int currentErrno = errno;
    fprintf(stderr, "[%d] %s\n", currentErrno, context);
    abort();
}

/**
 * Set the file descriptor to non-blocking mode
*/
void fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) { die("fcntl() read error"); return; }

    flags |= O_NONBLOCK;
    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno) { die("fcntl() write error"); return; }
}

/**
 * Read exactly n bytes from the file descriptor
 * size_t is unsigned int, used for memory allocation
 * ssize_t is signed int, used for file descriptor
*/
int32_t read_all(int fd, char* buf, size_t n){
    while (n > 0){
        ssize_t rv = read(fd, buf, n); // possible that we get less than n bytes
        if (rv <= 0) {
            return -1; // error or unexpected EOF happened
        }
        
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

/**
 * Write exactly n bytes to the file descriptor
*/
int32_t write_all(int fd, char* buf, size_t n){
    while (n > 0){
        ssize_t rv = write(fd, buf, n); // possible that we write less than n bytes
        if (rv <= 0) {
            return -1; // error in writing
        }

        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

// Buffer utilities
void append_buffer(std::vector<uint8_t>& buffer, const uint8_t* data, size_t len) {
    buffer.insert(buffer.end(), data, data + len);
}

void consume_buffer(std::vector<uint8_t>& buffer, size_t len) {
    if (len > buffer.size()) { len = buffer.size(); }
    buffer.erase(buffer.begin(), buffer.begin() + len);
}

