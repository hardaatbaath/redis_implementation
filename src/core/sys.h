// src/sys.h
#pragma once // to prevent multiple inclusion, as this header file is included in multiple files

void msg(const char *msg);
void msg_error(const char *msg);
[[noreturn]] void die(const char *msg);
void fd_set_nb(int fd);