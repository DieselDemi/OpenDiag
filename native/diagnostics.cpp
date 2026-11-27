#include "diagnostics.h"

#include <algorithm>
#include <charconv>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace diag {
static const Operation ops[] = {{"ping", {0x22, 0xd1, 0x00}}, {"module", {0x22, 0xf1, 0x88}},
                                {"vin", {0x22, 0xf1, 0x90}},  {"dtc", {0x03}},
                                {"rpm", {0x01, 0x0c}},        {"pending-dtc", {0x07}},
                                {"permanent-dtc", {0x0a}},    {"ford-dtc", {0x19, 0x02, 0xff}},
                                {"obd-vin", {0x09, 0x02}},    {"calid", {0x09, 0x04}},
                                {"cvn", {0x09, 0x06}},        {"lookup-id", {0x22, 0xe6, 0x11}}};
static unsigned number(const std::string &s, int base) {
    unsigned value{};
    auto r = std::from_chars(s.data(), s.data() + s.size(), value, base);
    if (r.ec != std::errc{} || r.ptr != s.data() + s.size())
        throw std::runtime_error("Invalid numeric identifier: " + s);
    return value;
}
Link Link::parse(const std::string &tx, const std::string &rx, const std::string &baud) {
    Link link{number(tx, 16), number(rx, 16), number(baud, 10)};
    if (link.tx == link.rx || link.tx > 0x1fffffff || link.rx > 0x1fffffff ||
        link.extended() != (link.rx > 0x7ff) || link.tx == 0x7df || link.tx == 0x18db33f1 ||
        (link.baud != 125000 && link.baud != 250000 && link.baud != 500000 && link.baud != 1000000))
        throw std::runtime_error("Expected a physical CAN address pair and a supported bitrate");
    return link;
}
std::string Link::addresses() const { return hex(tx, extended() ? 8 : 3) + "/" + hex(rx, extended() ? 8 : 3); }
Operation operation(const std::string &name) {
    for (auto &op : ops)
        if (name == op.name)
            return op;
    auto sep = name.find(':');
    if (sep != name.npos) {
        auto kind = name.substr(0, sep), token = name.substr(sep + 1);
        auto id = number(token, 16);
        if (kind == "ford-dtc" && token.size() == 2 && id)
            return {name, {0x19, 0x02, std::uint8_t(id)}};
        if (kind == "did" && token.size() == 4)
            return {name, {0x22, std::uint8_t(id >> 8), std::uint8_t(id)}};
        if (kind == "obd" && token.size() == 2)
            return {name, {0x01, std::uint8_t(id)}};
        if (kind == "freeze" && token.size() == 2)
            return {name, {0x02, std::uint8_t(id), 0}};
        if (kind == "mode6" && token.size() == 2)
            return {name, {0x06, std::uint8_t(id)}};
        if (kind == "mode9" && token.size() == 2)
            return {name, {0x09, std::uint8_t(id)}};
    }
    throw std::runtime_error("Unknown diagnostic operation");
}
Operation operation(const Bytes &request) {
    for (auto &op : ops)
        if (request == op.request)
            return op;
    if (request.size() == 3 && request[0] == 0x22)
        return operation("did:" + hex(request[1] * 256 + request[2], 4));
    if (request.size() == 3 && request[0] == 0x19 && request[1] == 2)
        return operation("ford-dtc:" + hex(request[2], 2));
    if (request.size() == 2 && request[0] == 1)
        return operation("obd:" + hex(request[1], 2));
    if (request.size() == 2 && request[0] == 6)
        return operation("mode6:" + hex(request[1], 2));
    if (request.size() == 2 && request[0] == 9)
        return operation("mode9:" + hex(request[1], 2));
    if (request.size() == 3 && request[0] == 2 && request[2] == 0)
        return operation("freeze:" + hex(request[1], 2));
    throw std::runtime_error("Unrecognized diagnostic request in trace");
}

std::string hex(unsigned value, int width) {
    std::ostringstream s;
    s << std::hex << std::uppercase << std::setw(width) << std::setfill('0') << value;
    return s.str();
}

bool matches(const Operation &op, const Bytes &reply) {
    auto n = op.request[0] == 0x19 ? 2u : op.request.size();
    return reply.size() >= n && reply[0] == op.request[0] + 0x40 &&
           std::equal(op.request.begin() + 1, op.request.begin() + n, reply.begin() + 1);
}

std::string decode(const Operation &op, const Bytes &reply) {
    const auto &q = op.request;
    if (reply.size() == 3 && reply[0] == 0x7f && reply[1] == q[0])
        throw std::runtime_error("ECU negative response 0x" + hex(reply[2], 2));
    if (!matches(op, reply) || reply.size() <= (q[0] == 0x19 ? 2u : q.size()))
        throw std::runtime_error("Unexpected or incomplete ECU response");
    if (q[0] == 0x19) {
        if ((reply.size() - 3) % 4)
            throw std::runtime_error("Invalid UDS DTC record length");
        std::string text = "Status availability 0x" + hex(reply[2], 2);
        for (size_t i = 3; i < reply.size(); i += 4)
            text += "; " + hex(reply[i], 2) + hex(reply[i + 1], 2) + hex(reply[i + 2], 2) + " status 0x" +
                    hex(reply[i + 3], 2);
        return text + (reply.size() == 3 ? "; no matching DTCs" : "");
    }
    const Bytes data(reply.begin() + q.size(), reply.end());
    if (op.name.find(':') != op.name.npos) {
        std::string raw = "raw=";
        for (auto b : data)
            raw += hex(b, 2);
        return raw;
    }
    if (std::string_view(op.name) == "ping") {
        if (data.size() != 1)
            throw std::runtime_error("Invalid D100 length");
        const auto level = data.front();
        const char *type = level >= 1 && level <= 3                          ? "GGDS"
                           : level == 0x81 || level == 0x85 || level == 0x87 ? "GDS"
                                                                             : "unknown CAN level";
        return std::string(type) + " (D100 = 0x" + hex(level, 2) + ")";
    }
    if (q[0] == 0x03 || q[0] == 7 || q[0] == 0x0a) {
        if (data.size() != 1u + data.front() * 2u)
            throw std::runtime_error("Invalid OBD-II DTC count");
        std::string codes;
        for (size_t i = 1; i < data.size(); i += 2) {
            if (!codes.empty())
                codes += ", ";
            codes += "PCBU"[data[i] >> 6];
            codes += hex(((data[i] & 63) << 8) | data[i + 1], 4);
        }
        return codes.empty() ? (q[0] == 3   ? "No stored OBD-II DTCs"
                                : q[0] == 7 ? "No pending OBD-II DTCs"
                                            : "No permanent OBD-II DTCs")
                             : codes;
    }
    if (q[0] == 9) {
        size_t width = q[1] == 2 ? 17 : q[1] == 4 ? 16 : 4;
        if (!data[0] || data.size() != 1 + data[0] * width || (q[1] == 2 && data[0] != 1))
            throw std::runtime_error("Invalid Mode 9 item count or length");
        std::string result;
        for (size_t i = 1; i < data.size(); i += width) {
            if (i > 1)
                result += ", ";
            if (q[1] == 6) {
                for (size_t j = 0; j < 4; ++j)
                    result += hex(data[i + j], 2);
            } else {
                auto end = i + width;
                while (end > i && data[end - 1] == 0)
                    --end;
                if (!std::all_of(data.begin() + i, data.begin() + end,
                                 [](int c) { return c >= 32 && c <= 126; }))
                    throw std::runtime_error("Invalid Mode 9 text");
                result.append(data.begin() + i, data.begin() + end);
            }
        }
        return result;
    }
    if (q[0] == 0x01) {
        if (data.size() != 2)
            throw std::runtime_error("Invalid RPM length");
        std::ostringstream s;
        s << ((data[0] << 8) | data[1]) / 4.0 << " rpm";
        return s.str();
    }
    if (q[2] == 0x90 && (data.size() != 17 || !std::all_of(data.begin(), data.end(), [](int c) {
                             return (c >= '0' && c <= '9') ||
                                    (c >= 'A' && c <= 'Z' && c != 'I' && c != 'O' && c != 'Q');
                         })))
        throw std::runtime_error("Invalid VIN response");
    if (std::all_of(data.begin(), data.end(), [](int c) { return c >= 32 && c <= 126; }))
        return std::string(data.begin(), data.end());
    std::string result = "hex:";
    for (auto v : data)
        result += " " + hex(v, 2);
    return result;
}
} // namespace diag
