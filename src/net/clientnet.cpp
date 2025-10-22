// C stdlib
#include <stdint.h>  // int32_t
#include <stddef.h>  // size_t
#include <unistd.h>  // read, write
#include <assert.h>  // assert

// local
#include "clientnet.h" // read_all, write_all
#include "../core/sys.h" // msg

/**
 * Read exactly n bytes from the file descriptor
 * size_t is unsigned int, used for memory allocation
 * ssize_t is signed int, used for file descriptor
*/
int32_t read_all(int fd, char* buf, size_t n){
    while (n > 0){
        ssize_t rv = read(fd, buf, n); // possible that we get less than n bytes
        if (rv <= 0) {
            return -1; // error or unexpected EOF happened
        }
        
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

/**
 * Write exactly n bytes to the file descriptor
*/
int32_t write_all(int fd, char* buf, size_t n){
    while (n > 0){
        ssize_t rv = write(fd, buf, n); // possible that we write less than n bytes
        if (rv <= 0) {
            return -1; // error in writing
        }

        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

/**
 * Print the response
*/
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
                int32_t len = 0;
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
                int32_t len = 0;
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
                int32_t len = 0;
                memcpy(&len, &data[1], 4);
                printf("array length: %d: ", len);

                size_t arr_bytes = 1 + 4;
                for (uint32_t i = 0; i < len; i++) {
                    int32_t rv = print_response(&data[arr_bytes], size - arr_bytes);
                    if (rv <= 0) { msg("bad response"); return -1; }
                    arr_bytes += rv;
                }
                
                printf("array end\n");
                return arr_bytes;
            }

        // 1 byte: for the tag
        // 4 bytes: for the map length
        // map length bytes: for the map
        case TAG_MAP:
            if (size < 1 + 4) { msg("bad response"); return -1; }

            {
                int32_t len = 0;
                memcpy(&len, &data[1], 4);
                printf("map length: %d: ", len);

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