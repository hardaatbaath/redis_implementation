// C stdlib
#include <string.h>      // memcpy (read_header/read_string)
#include <stdint.h>      // standard types

// C++ stdlib
#include <vector>        // std::vector (parse_request)
#include <string>        // std::string (read_string)

// local
#include "protocol.h"          // Response, API declarations
#include "../core/constants.h" // k_max_args
#include "../core/sys.h"       // append_buffer

/**
 * Read the header for the request (size or total number of requests)
*/
bool read_header(const uint8_t *&cursor, const uint8_t *end, uint32_t &value) {
    // Check if there is enough data to read the header`
    if (end - cursor < 4) { return false; }

    // Read the header, move the pointer to the next 4 bytes
    memcpy(&value, cursor, 4);
    cursor += 4;
    
    return true;
}

/**
 * Read the request
*/
bool read_string(const uint8_t *&cursor, const uint8_t *end, uint32_t len, std::string &output) {
    // Check if there is enough data to read the string
    if (end - cursor < len) { return false; }

    // Read the string, move the pointer to the next len bytes
    output = std::string((char*)cursor, len);
    cursor += len;

    return true;
}

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

        cmd.push_back(std::string()); // push back an empty string

        // Read the request
        if (!read_string(cursor, end, len, cmd.back())) { return -1; }
    }

    // Check if there is any remaining data
    if (cursor != end) { return -1; }

    return 0;
}

/**
 * Generate the response
*/
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

