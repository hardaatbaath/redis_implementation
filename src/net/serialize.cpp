// C stdlib
#include <stdint.h>  // int32_t
#include <stddef.h>  // size_t
#include <string.h>  // memcpy
#include <stdio.h>   // printf

// local
#include "serialize.h" // TAG_NIL, TAG_ERR, TAG_STR, TAG_INT, TAG_DBL, TAG_BOOL, TAG_ARR, TAG_MAP, print_response
#include "../core/buffer_io.h" // Buffer append/consume helpers



// append serialized data types to the back
void out_nil(Buffer &out) {
    append_buffer_u8(out, TAG_NIL);
}

void out_err(Buffer &out, uint32_t code, const std::string &msg) {
    append_buffer_u8(out, TAG_ERR);
    append_buffer_u32(out, code);
    append_buffer_u32(out, (uint32_t)msg.size());
    append_buffer(out, (const uint8_t *)msg.data(), msg.size());
}

void out_str(Buffer &out, const char *s, size_t size) {
    append_buffer_u8(out, TAG_STR);
    append_buffer_u32(out, (uint32_t)size);
    append_buffer(out, (const uint8_t *)s, size);
}

void out_int(Buffer &out, int64_t val) {
    append_buffer_u8(out, TAG_INT);
    append_buffer_i64(out, val);
}

void out_dbl(Buffer &out, double val) {
    append_buffer_u8(out, TAG_DBL);
    append_buffer_f64(out, val);
}

void out_bool(Buffer &out, bool val) {
    append_buffer_u8(out, TAG_BOOL);
    append_buffer_bool(out, val);
}

void out_arr(Buffer &out, uint32_t n) {
    append_buffer_u8(out, TAG_ARR);
    append_buffer_u32(out, n);
}

void out_map(Buffer &out, uint32_t n) {
    append_buffer_u8(out, TAG_MAP);
    append_buffer_u32(out, n);
}