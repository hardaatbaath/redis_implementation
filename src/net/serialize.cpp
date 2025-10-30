// C stdlib
#include <stdint.h>  // int32_t
#include <stddef.h>  // size_t
#include <string.h>  // memcpy
#include <stdio.h>   // printf

// local
#include "serialize.h" // TAG_NIL, TAG_ERR, TAG_STR, TAG_INT, TAG_DBL, TAG_BOOL, TAG_ARR, TAG_MAP, print_response
#include "../core/sys.h" // msg, msg_error, die, fd_set_nb
#include "../core/buffer_io.h" // Buffer append/consume helpers

// Print the response
int32_t print_response(const uint8_t *data, size_t size){
    if (size < 1) { msg("bad response"); return -1; }

    switch (data[0]) {
        case TAG_NIL:
            printf("nil\n");
            return 1;

        // 1 byte: for the tag
        // 4 bytes: for the error code
        // 4 bytes: for the message length
        // message length bytes: for the message
        case TAG_ERR:
            if (size < 1 + 8) { msg("bad response"); return -1; }

            {
                int32_t code = 0;
                uint32_t len = 0;
                memcpy(&code, &data[1], 4);
                memcpy(&len, &data[1 + 4], 4);

                if (size < 1 + 8 + len) { msg("bad response"); return -1; }

                printf("error %d: %.*s\n", code, len, &data[1 + 8]);
                return 1 + 8 + len;
            }

        // 1 byte: for the tag
        // 4 bytes: for the string length
        // string length bytes: for the string
        case TAG_STR:
            if (size < 1 + 4) { msg("bad response"); return -1; }

            {
                uint32_t len = 0;
                memcpy(&len, &data[1], 4);

                if (size < 1 + 4 + len) { msg("bad response"); return -1; }
            
                printf("%.*s\n", len, &data[1 + 4]);
                return 1 + 4 + len;
            }

        // 1 byte: for the tag
        // 8 bytes: for the integer
        case TAG_INT:
            if (size < 1 + 8) { msg("bad response"); return -1; }

            {
                int64_t value = 0;
                memcpy(&value, &data[1], 8);

                printf("%lld\n", value); // long long because the size of long varies between different architectures
                return 1 + 8;
            }

        // 1 byte: for the tag
        // 8 bytes: for the double
        case TAG_DBL:
            if (size < 1 + 8) { msg("bad response"); return -1; }

            {
                double value = 0;
                memcpy(&value, &data[1], 8);
                
                printf("%g\n", value);
                return 1 + 8;
            }

        // 1 byte: for the tag
        // 1 byte: for the boolean value
        case TAG_BOOL:
            if (size < 1 + 1) { msg("bad response"); return -1; }

            {
                uint8_t value = 0;
                memcpy(&value, &data[1], 1);

                printf("%s\n", value ? "true" : "false");
                return 1 + 1;
            }

        // 1 byte: for the tag
        // 4 bytes: for the array length
        // array length bytes: for the array
        case TAG_ARR:
            if (size < 1 + 4) { msg("bad response"); return -1; }

            {
                uint32_t len = 0;
                memcpy(&len, &data[1], 4);
                // printf("array length: %d\n", len);

                size_t arr_bytes = 1 + 4;
                for (uint32_t i = 0; i < len; i++) {
                    int32_t rv = print_response(&data[arr_bytes], size - arr_bytes);
                    if (rv <= 0) { msg("bad response"); return -1; }
                    arr_bytes += rv;
                }
                
                // printf("array end\n");
                return arr_bytes;
            }

        // 1 byte: for the tag
        // 4 bytes: for the map length
        // map length bytes: for the map
        case TAG_MAP:
            if (size < 1 + 4) { msg("bad response"); return -1; }

            {
                uint32_t len = 0;
                memcpy(&len, &data[1], 4);
                // printf("map length: %d: ", len);

                size_t map_bytes = 1 + 4;
                for (uint32_t i = 0; i < len; i++) {
                    int32_t rv = print_response(&data[map_bytes], size - map_bytes);
                    if (rv <= 0) { msg("bad response"); return -1; }
                    map_bytes += rv;
                }

                printf("map end\n");
                return map_bytes;
            }

        default:
            msg("bad response");
            return -1;
    }
}

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