#include "replay.h"
#include "diagnostics.h"
#include <charconv>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>

void replay(const std::filesystem::path &capture) {
    if (std::filesystem::file_size(capture) > 64 * 1024 * 1024)
        throw std::runtime_error("Capture exceeds 64 MiB");
    std::ifstream in(capture);
    if (!in)
        throw std::runtime_error("Cannot read capture");
    std::optional<diag::Operation> pending;
    std::string tx = "7E0", rx = "7E8";
    unsigned count = 0;
    std::string line;
    while (std::getline(in, line)) {
        // GUI exports prefix each worker record with an ISO timestamp and two spaces.
        auto sep = line.find("  ");
        if (line.size() > 10 && line[4] == '-' && line[7] == '-' && sep != line.npos)
            line.erase(0, sep + 2);
        std::istringstream fields(line);
        std::string tag, id, token;
        fields >> tag;
        if (tag == "OK" && line.rfind("OK Connected: ", 0) == 0) {
            std::string connected, module, pair;
            fields >> connected >> module >> pair;
            auto slash = pair.find('/');
            if (pending || slash == pair.npos || pair.back() != ',')
                throw std::runtime_error("Invalid session header in trace");
            pair.pop_back();
            tx = pair.substr(0, slash);
            rx = pair.substr(slash + 1);
            diag::Link::parse(tx, rx, "500000");
            continue;
        }
        if ((tag == "ERR" || tag == "ITEM_ERROR") && pending) {
            std::cout << pending->name << ": " << line << '\n';
            pending.reset();
            ++count;
            continue;
        }
        if (tag != "REQUEST" && tag != "RESPONSE")
            continue;
        fields >> id;
        if (id != (tag == "REQUEST" ? tx : rx))
            throw std::runtime_error("Unexpected module address in trace");
        diag::Bytes bytes;
        while (fields >> token) {
            unsigned value = 0;
            auto parsed = std::from_chars(token.data(), token.data() + token.size(), value, 16);
            if (token.size() != 2 || parsed.ec != std::errc{} || parsed.ptr != token.data() + 2 ||
                bytes.size() >= 4124)
                throw std::runtime_error("Invalid payload in trace");
            bytes.push_back(static_cast<std::uint8_t>(value));
        }
        if (tag == "REQUEST") {
            if (pending)
                throw std::runtime_error("Missing response before next request");
            pending = diag::operation(bytes);
        } else {
            if (!pending)
                throw std::runtime_error("Response without a request");
            std::string result;
            if (bytes.size() == 3 && bytes[0] == 0x7f && bytes[1] == pending->request[0]) {
                try {
                    diag::decode(*pending, bytes);
                } catch (const std::runtime_error &e) {
                    result = e.what();
                }
            } else
                result = diag::decode(*pending, bytes);
            std::cout << pending->name << ": " << result << '\n';
            pending.reset();
            ++count;
        }
    }
    if (in.bad() || pending || !count)
        throw std::runtime_error("Incomplete or empty diagnostic capture");
}
