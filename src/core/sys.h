// src/sys.h
#pragma once // to prevent multiple inclusion, as this header file is included in multiple files

// C stdlib
#include <stddef.h>  // size_t (append_buffer, consume_buffer)
#include <stdint.h>  // uint8_t (append_buffer)

// C++ stdlib
#include <vector>    // std::vector (append_buffer, consume_buffer)

void msg(const char *msg);
void msg_error(const char *msg);
[[noreturn]] void die(const char *msg);
void fd_set_nb(int fd);

// Buffer utilities
void append_buffer(std::vector<uint8_t>& buffer, const uint8_t* data, size_t len);
void consume_buffer(std::vector<uint8_t>& buffer, size_t len);