#pragma once

#include <stddef.h>

// Maximum message size (header+payload safety bound)
const size_t k_max_msg = 32u << 20; // 32 MiB

// Maximum number of arguments for a command
const size_t k_max_args = 200 * 1000; // 200,000 arguments
