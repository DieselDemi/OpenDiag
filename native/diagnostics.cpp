#include "diagnostics.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace diag {
static const Operation ops[] = {{"ping", {0x22, 0xd1, 0x00}}, {"module", {0x22, 0xf1, 0x88}},
    {"vin", {0x22, 0xf1, 0x90}}, {"dtc", {0x03}}, {"rpm", {0x01, 0x0c}}};
const Operation &operation(const std::string &name) {
    for (auto &op : ops) if (name == op.name) return op;
    throw std::runtime_error("Unknown diagnostic operation");
}
const Operation &operation(const Bytes &request) {
    for (auto &op : ops) if (request == op.request) return op;
    throw std::runtime_error("Unrecognized diagnostic request in trace");
}

static std::string hex(unsigned value, int width) {
    std::ostringstream s;
    s << std::hex << std::uppercase << std::setw(width) << std::setfill('0') << value;
    return s.str();
}

std::string decode(const Operation &op, const Bytes &reply) {
    const auto &q = op.request;
    if (reply.size() == 3 && reply[0] == 0x7f && reply[1] == q[0])
        throw std::runtime_error("ECU negative response 0x" + hex(reply[2], 2));
    if (reply.size() <= q.size() || reply[0] != q[0] + 0x40 ||
        !std::equal(q.begin() + 1, q.end(), reply.begin() + 1))
        throw std::runtime_error("Unexpected or incomplete ECU response");
    const Bytes data(reply.begin() + q.size(), reply.end());
    if (std::string_view(op.name) == "ping") {
        if (data.size() != 1) throw std::runtime_error("Invalid D100 length");
        const auto level = data.front();
        const char *type = level >= 1 && level <= 3 ? "GGDS" :
            level == 0x81 || level == 0x85 || level == 0x87 ? "GDS" : "unknown CAN level";
        return std::string(type) + " (D100 = 0x" + hex(level, 2) + ")";
    }
    if (q[0] == 0x03) {
        if (data.size() != 1u + data.front() * 2u) throw std::runtime_error("Invalid OBD-II DTC count");
        std::string codes;
        for (size_t i = 1; i < data.size(); i += 2) {
            if (!codes.empty()) codes += ", ";
            codes += "PCBU"[data[i] >> 6];
            codes += hex(((data[i] & 63) << 8) | data[i+1], 4);
        }
        return codes.empty() ? "No stored OBD-II DTCs" : codes;
    }
    if (q[0] == 0x01) {
        if (data.size() != 2) throw std::runtime_error("Invalid RPM length");
        std::ostringstream s;
        s << ((data[0] << 8) | data[1]) / 4.0 << " rpm";
        return s.str();
    }
    if (q[2] == 0x90 && (data.size() != 17 || !std::all_of(data.begin(), data.end(), [](int c) {
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z' && c != 'I' && c != 'O' && c != 'Q');
    }))) throw std::runtime_error("Invalid VIN response");
    if (std::all_of(data.begin(), data.end(), [](int c) { return c >= 32 && c <= 126; }))
        return std::string(data.begin(), data.end());
    std::string result = "hex:";
    for (auto v : data) result += " " + hex(v, 2);
    return result;
}
}
