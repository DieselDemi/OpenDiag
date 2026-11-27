#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace diag {
using Bytes = std::vector<std::uint8_t>;
struct Operation {
    std::string name;
    Bytes request;
};
Operation operation(const std::string &name);
Operation operation(const Bytes &request);
bool matches(const Operation &, const Bytes &reply);
std::string hex(unsigned value, int width);
struct Link {
    unsigned tx = 0x7e0, rx = 0x7e8, baud = 500000;
    bool extended() const { return tx > 0x7ff; }
    static Link parse(const std::string &tx, const std::string &rx, const std::string &baud);
    std::string addresses() const;
};
std::string decode(const Operation &, const Bytes &reply);
} // namespace diag
