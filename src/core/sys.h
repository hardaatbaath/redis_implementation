// src/sys.h
#pragma once // to prevent multiple inclusion, as this header file is included in multiple files

// C stdlib
#include <stddef.h>  // size_t (append_buffer, consume_buffer)
#include <stdint.h>  // uint8_t (append_buffer)

// C++ stdlib
#include <vector>    // std::vector (append_buffer, consume_buffer)
#include <string>    // std::string (append_buffer)
#include <map>       // std::map (append_buffer)

typedef std::vector<uint8_t> Buffer;

void msg(const char *msg);
void msg_error(const char *msg);
[[noreturn]] void die(const char *msg);
void fd_set_nb(int fd);

// Basic buffer utilities
void append_buffer(Buffer& buffer, const uint8_t* data, size_t len);
void consume_buffer(Buffer& buffer, size_t len);

void append_buffer_u8(Buffer& buffer, uint8_t value);
void append_buffer_u32(Buffer& buffer, uint32_t value);
void append_buffer_i64(Buffer& buffer, int64_t value);
void append_buffer_f64(Buffer& buffer, double value);
void append_buffer_bool(Buffer& buffer, bool value);
void append_buffer_string(Buffer& buffer, const std::string& value);
void append_buffer_array(Buffer& buffer, const std::vector<uint8_t>& value);
void append_buffer_map(Buffer& buffer, const std::map<uint8_t, uint8_t>& value);