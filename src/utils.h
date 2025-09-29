// src/utils.h
#pragma once // to prevent multiple inclusion, as this header file is included in multiple files

// C stdlib
#include <stddef.h>
#include <stdint.h>

// C++ stdlib
#include <vector>

void msg(const char *msg);
void msg_error(const char *msg);
[[noreturn]] void die(const char *msg);
void fd_set_nb(int fd);

// Buffer utilities
void append_buffer(std::vector<uint8_t>& buffer, const uint8_t* data, size_t len);
void consume_buffer(std::vector<uint8_t>& buffer, size_t len);
int32_t read_full(int fd, char *buf, size_t n); // static functions limit the visibility of the function
int32_t write_all(int fd, char *buf, size_t n);