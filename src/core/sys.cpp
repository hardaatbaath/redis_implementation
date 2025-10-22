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

// Buffer utilities
void append_buffer(std::vector<uint8_t>& buffer, const uint8_t* data, size_t len) {
    buffer.insert(buffer.end(), data, data + len);
}

void consume_buffer(std::vector<uint8_t>& buffer, size_t len) {
    if (len > buffer.size()) { len = buffer.size(); }
    buffer.erase(buffer.begin(), buffer.begin() + len);
}

