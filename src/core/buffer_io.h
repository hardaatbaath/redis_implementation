#pragma once

// C stdlib
#include <stddef.h>  // size_t (append_buffer, consume_buffer)
#include <stdint.h>  // uint8_t (append_buffer)

// C++ stdlib
#include <vector>    // std::vector (append_buffer, consume_buffer)
#include <string>    // std::string (append_buffer)
#include <map>       // std::map (append_buffer)

typedef std::vector<uint8_t> Buffer;

// Append data to the buffer
inline void append_buffer(Buffer& buffer, const uint8_t* data, size_t len) {
    buffer.insert(buffer.end(), data, data + len);
}

// Consume data from the buffer
inline void consume_buffer(Buffer& buffer, size_t len) {
    if (len > buffer.size()) { len = buffer.size(); }
    buffer.erase(buffer.begin(), buffer.begin() + len);
}

// Append a uint8_t to the buffer
inline void append_buffer_u8(Buffer& buffer, uint8_t value) {
    buffer.push_back(value);
}

// Append a uint32_t to the buffer
inline void append_buffer_u32(Buffer& buffer, uint32_t value) {
    append_buffer(buffer, (const uint8_t*)&value, 4);
}

// Append a int64_t to the buffer
inline void append_buffer_i64(Buffer& buffer, int64_t value) {
    append_buffer(buffer, (const uint8_t*)&value, 8);
}

// Append a double to the buffer
inline void append_buffer_f64(Buffer& buffer, double value) {
    append_buffer(buffer, (const uint8_t*)&value, 8);
}

// Append a bool to the buffer
inline void append_buffer_bool(Buffer& buffer, bool value) {
    append_buffer(buffer, (const uint8_t*)&value, 1);
}

// Append a string to the buffer
inline void append_buffer_string(Buffer& buffer, const std::string& value) {
    append_buffer_u32(buffer, value.size());
    append_buffer(buffer, (const uint8_t*)value.data(), value.size());
}

// Append an array to the buffer
inline void append_buffer_array(Buffer& buffer, const std::vector<uint8_t>& value) {
    append_buffer_u32(buffer, value.size());
    append_buffer(buffer, value.data(), value.size());
}

// Append a map to the buffer
inline void append_buffer_map(Buffer& buffer, const std::map<uint8_t, uint8_t>& value) {
    append_buffer_u32(buffer, value.size());
    for (const auto& item : value) {
        append_buffer_u8(buffer, item.first);
        append_buffer_u8(buffer, item.second);
    }
}

