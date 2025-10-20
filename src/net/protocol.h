#pragma once

// C stdlib
#include <stdint.h>      // uint8_t, uint32_t
#include <string>        // std::string (read_string)
#include <vector>        // std::vector (parse_request)

struct Response {
    uint32_t status;
    std::vector<uint8_t> data;
};

enum ResponseStatus {
    RES_OK = 200,
    RES_NX = 404,
    RES_ERR = 500,
};

// protocol helpers
bool read_header(const uint8_t *&cursor, const uint8_t *end, uint32_t &value);
bool read_string(const uint8_t *&cursor, const uint8_t *end, uint32_t len, std::string &output);
int32_t parse_request(const uint8_t *data, size_t size, std::vector<std::string> &cmd);
void generate_response(const Response &resp, std::vector<uint8_t> &out); // kept original name for compatibility

