// C stdlib
#include <errno.h>   // errno  (die, fd_set_nb)
#include <stdio.h>   // fprintf (msg, msg_error, die)
#include <stdlib.h>  // abort (die)

// POSIX / system
#include <unistd.h>  // read, write (read_all, write_all)
#include <fcntl.h>   // fcntl (fd_set_nb)
#include <time.h>    // clock_gettime (get_monotonic_msec)

// local
#include "sys.h" // msg, msg_error, die, fd_set_nb

// Print a message to stderr
void msg(const char* message) {
    fprintf(stderr, "%s\n", message);
}

// Print an error message to stderr
void msg_error(const char* message) {
    fprintf(stderr, "[ERROR] %s\n", message);
}

// Print an error message to stderr and exit the program
[[noreturn]] void die(const char* context) {
    const int currentErrno = errno;
    fprintf(stderr, "[%d] %s\n", currentErrno, context);
    abort();
}

// Set the file descriptor to non-blocking mode
void fd_set_nb(int fd) {
    errno = 0;
    int flags = fcntl(fd, F_GETFL, 0);
    if (errno) { die("fcntl() read error"); return; }

    flags |= O_NONBLOCK;
    errno = 0;
    (void)fcntl(fd, F_SETFL, flags);
    if (errno) { die("fcntl() write error"); return; }
}

// Get the monotonic time in milliseconds
static uint64_t get_monotonic_msec() {
    // Create a timespec struct to store the time
    struct timespec tv = {0, 0};
    // Get the monotonic time
    clock_gettime(CLOCK_MONOTONIC, &tv);
    // Return the time in milliseconds
    return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_nsec / 1000 / 1000;
}

// Get the current time in milliseconds
uint64_t get_current_time_ms() {
    return get_monotonic_msec();
}