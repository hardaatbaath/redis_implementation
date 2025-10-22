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
void append_buffer(Buffer& buffer, const uint8_t* data, size_t len) {
    buffer.insert(buffer.end(), data, data + len);
}

void consume_buffer(Buffer& buffer, size_t len) {
    if (len > buffer.size()) { len = buffer.size(); }
    buffer.erase(buffer.begin(), buffer.begin() + len);
}

// Serialisation Buffer utilities
void append_buffer_u8(Buffer& buffer, uint8_t value) {
    buffer.push_back(value);
}

void append_buffer_u32(Buffer& buffer, uint32_t value) {
    append_buffer(buffer, (const uint8_t*)&value, 4);
}

void append_buffer_i64(Buffer& buffer, int64_t value) {
    append_buffer(buffer, (const uint8_t*)&value, 8);
}

void append_buffer_f64(Buffer& buffer, double value) {
    append_buffer(buffer, (const uint8_t*)&value, 8);
}

void append_buffer_bool(Buffer& buffer, bool value) {
    append_buffer(buffer, (const uint8_t*)&value, 1);
}

void append_buffer_string(Buffer& buffer, const std::string& value) {
    append_buffer_u32(buffer, value.size());
    append_buffer(buffer, (const uint8_t*)value.data(), value.size());
}

void append_buffer_array(Buffer& buffer, const std::vector<uint8_t>& value) {
    append_buffer_u32(buffer, value.size());
    append_buffer(buffer, value.data(), value.size());
}

void append_buffer_map(Buffer& buffer, const std::map<uint8_t, uint8_t>& value) {
    append_buffer_u32(buffer, value.size());
    for (const auto& item : value) {
        append_buffer_u8(buffer, item.first);
        append_buffer_u8(buffer, item.second);
    }
}