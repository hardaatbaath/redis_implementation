// src/utils.cpp
#include "utils.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

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