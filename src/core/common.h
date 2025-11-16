#pragma once

// C stdlib
#include <stdint.h>
#include <stddef.h>

// C++ stdlib
#include <string>
#include <math.h>

/**
 * Macro to recover the address of a parent struct from the address of one of its members. 
 * As we are using intrusive data structure, T and ptr for the members are together in the same memory location.
 * So we can recover the address of the parent struct by subtracting the offset of the member from the address of the pointer.
*/
#define container_of(ptr, type, member) ({               \
   const typeof(((type *)0)->member) *__mptr = (ptr);    \
   (type *)((char *)__mptr - offsetof(type, member));})

// #define container_of(ptr, type, member) (type *)((char *)(ptr) - offsetof(type, member))

// FNV hash function
inline uint64_t string_hash(const uint8_t *data, size_t len){
    uint32_t base = 0x811C9DC5; // FNV-1a offset basis, Decimal: 2166136261
    uint32_t prime = 0x1000193; // FNV-1a prime, Decimal: 16777619

    for (size_t i = 0; i < len; i++) {
        base = (base + data[i]) * prime;
    }
    return base;
}

// Convert string to double
inline bool str2dbl(const std::string &s, double &out) {
    char *endp = NULL;
    out = strtod(s.c_str(), &endp);
    return endp == s.c_str() + s.size() && !isnan(out);
}

// Convert string to integer
inline bool str2int(const std::string &s, int64_t &out) {
    char *endp = NULL;
    out = strtoll(s.c_str(), &endp, 10);
    return endp == s.c_str() + s.size();
}