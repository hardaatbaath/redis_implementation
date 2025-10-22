// C stdlib
#include <stdint.h>  // int32_t
#include <stddef.h>  // size_t
#include <unistd.h>  // read, write
#include <assert.h>  // assert

// local
#include "clientnet.h" // read_all, write_all

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