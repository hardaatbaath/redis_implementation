#pragma once

// C stdlib
#include <stdint.h>  // int32_t
#include <stddef.h>  // size_t

/**
 * Read exactly n bytes from the file descriptor
*/
int32_t read_all(int fd, char *buf, size_t n);
int32_t write_all(int fd, char *buf, size_t n);