// src/utils.cpp
#include "utils.h"

#include <assert.h> 
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for read, write

/**
 * Print a message to stderr
 */
void msg(const char* message) {
    fprintf(stderr, "%s\n", message);
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
 * Read exactly n bytes from the file descriptor
 * size_t is unsigned int, used for memory allocation
 * ssize_t is signed int, used for file descriptor
 */
int32_t read_full(int fd, char* buf, size_t n){
    while (n > 0){
        ssize_t rv = read(fd, buf, n);
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
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1; // error in writing
        }

        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}