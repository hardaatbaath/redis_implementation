// src/utils.h
#pragma once // to prevent multiple inclusion, as this header file is included in multiple files
#include <stddef.h>
#include <stdint.h>

void msg(const char *msg);
[[noreturn]] void die(const char *msg); // to prevent the function from returning
int32_t read_full(int fd, char *buf, size_t n); // static functions limit the visibility of the function
int32_t write_all(int fd, char *buf, size_t n);