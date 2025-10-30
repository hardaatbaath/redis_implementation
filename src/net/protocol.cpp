// C stdlib
#include <string.h>      // memcpy (read_header/read_string)

// C++ stdlib
#include <vector>        // std::vector (parse_request)
#include <string>        // std::string (read_string)

// local
#include "protocol.h"          // Response, API declarations
#include "../core/constants.h" // k_max_args
#include "../core/buffer_io.h" // Buffer, append_buffer

/** 
 * Parse one request:
 *      +----------+-----+------+-----+------+-----+-----+------+
 *      | num_args | len | cmd1 | len | cmd2 | ... | len | cmdn |
 *      +----------+-----+------+-----+------+-----+-----+------+
*/
int32_t parse_request(const uint8_t *data, size_t size, std::vector<std::string> &cmd) {
    const uint8_t *cursor = data;
    const uint8_t *end = cursor + size;
    uint32_t num_args = 0;

    // Read the request header
    if (!read_header(cursor, end , num_args)) { return -1; } // Read the total number of requests
    if (num_args > k_max_args) { return -1; }

    // Read the requests
    while (cmd.size() < num_args) {
        uint32_t len = 0;

        // Read the length of the request
        if (!read_header(cursor, end , len)) { return -1; }

        cmd.push_back(std::string()); // Push back an empty string

        // Read the request
        if (!read_string(cursor, end, len, cmd.back())) { return -1; }
    }

    // Check if there is any remaining data
    if (cursor != end) { return -1; }

    return 0;
}

// Generate the response
void generate_response(const Response &resp, std::vector<uint8_t> &out) {
    // 4 bytes for the status code + the size of the data
    uint32_t resp_len = 4 + (uint32_t)resp.data.size();

    // append the length of the response
    append_buffer(out, (const uint8_t*)&resp_len, 4);

    // append the status code
    append_buffer(out, (const uint8_t*)&resp.status, 4);

    // append the data
    append_buffer(out, resp.data.data(), resp.data.size());
}

