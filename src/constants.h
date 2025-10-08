#pragma once

#include <stddef.h>

// Maximum message size (header+payload safety bound)
static const size_t k_max_msg = 32u << 20; // 32 MiB