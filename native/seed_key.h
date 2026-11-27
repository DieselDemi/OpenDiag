#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace seedkey {
using Bytes = std::vector<std::uint8_t>;

struct Input {
    std::string profile;
    std::uint8_t level = 0;
    Bytes seed;
    Bytes securityBytes;
};

struct Profile {
    std::string name;
    std::size_t seedLength;
    std::size_t securityBytesLength;
    std::size_t keyLength;
    bool builtIn;
};

const std::vector<Profile> &profiles();
Bytes key(const Input &input);
} // namespace seedkey
