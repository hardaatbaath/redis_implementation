#pragma once

// C stdlib
#include <stdint.h>  // int32_t
#include <stddef.h>  // size_t

enum {
    TAG_NIL = 0,     // nil
    TAG_ERR = 1,     // error code + message
    TAG_STR = 2,     // string
    TAG_INT = 3,     // int64
    TAG_DBL = 4,     // double
    TAG_BOOL = 5,    // boolean
    TAG_ARR = 6,     // array
    TAG_MAP = 7,     // map
};

/**
 * Read exactly n bytes from the file descriptor
*/
int32_t read_all(int fd, char *buf, size_t n);
int32_t write_all(int fd, char *buf, size_t n);
int32_t print_response(const uint8_t *data, size_t size);