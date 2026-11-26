#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace diag {
using Bytes = std::vector<std::uint8_t>;
struct Operation { const char *name; Bytes request; };
const Operation &operation(const std::string &name);
const Operation &operation(const Bytes &request);
std::string decode(const Operation &, const Bytes &reply);
}
