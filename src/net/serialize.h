#pragma once

// C stdlib
#include <stdint.h>  // int32_t, uint8_t
#include <stddef.h>  // size_t

// C++ stdlib
#include <string>    // std::string

#include "../core/buffer_io.h" // Buffer

// error code for TAG_ERR
enum {
    ERR_UNKNOWN = 1,    // unknown command
    ERR_TOO_BIG = 2,    // response too big
    ERR_BAD_TYP = 3,    // unexpected value type
    ERR_BAD_ARG = 4,    // bad arguments
};

// Tags for the response
enum {
    TAG_NIL = 0,     // nil
    TAG_ERR = 1,     // error code + message
    TAG_STR = 2,     // string
    TAG_INT = 3,     // int64
    TAG_DBL = 4,     // double
    TAG_BOOL = 5,    // boolean
    TAG_ARR = 6,     // array
    TAG_MAP = 7,     // map
};

// append serialized data types to the back
void out_nil(Buffer &out);
void out_err(Buffer &out, uint32_t code, const std::string &msg);
void out_str(Buffer &out, const char *s, size_t size);
void out_int(Buffer &out, int64_t val);
void out_dbl(Buffer &out, double val);
void out_bool(Buffer &out, bool val);
void out_arr(Buffer &out, uint32_t n);
void out_map(Buffer &out, uint32_t n);