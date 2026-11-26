#include "replay.h"
#include "diagnostics.h"
#include <charconv>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

void replay(const std::filesystem::path &capture) {
    if (std::filesystem::file_size(capture) > 64*1024*1024) throw std::runtime_error("Capture exceeds 64 MiB");
    std::ifstream in(capture);
    if (!in) throw std::runtime_error("Cannot read capture");
    const diag::Operation *pending = nullptr;
    unsigned count = 0;
    std::string line;
    while (std::getline(in,line)) {
        // GUI exports prefix each worker record with an ISO timestamp and two spaces.
        auto sep = line.find("  ");
        if (line.size() > 10 && line[4]=='-' && line[7]=='-' && sep != line.npos) line.erase(0,sep+2);
        std::istringstream fields(line);
        std::string tag, id, token;
        fields >> tag;
        if (tag == "ERR" && pending) {
            std::cout << pending->name << ": " << line << '\n'; pending = nullptr; ++count; continue;
        }
        if (tag != "REQUEST" && tag != "RESPONSE") continue;
        fields >> id;
        if (id != (tag == "REQUEST" ? "7E0" : "7E8")) throw std::runtime_error("Unexpected PCM address in trace");
        diag::Bytes bytes;
        while (fields >> token) {
            unsigned value = 0;
            auto parsed = std::from_chars(token.data(),token.data()+token.size(),value,16);
            if (token.size()!=2 || parsed.ec!=std::errc{} || parsed.ptr!=token.data()+2 || bytes.size()>=4124)
                throw std::runtime_error("Invalid payload in trace");
            bytes.push_back(static_cast<std::uint8_t>(value));
        }
        if (tag == "REQUEST") {
            if (pending) throw std::runtime_error("Missing response before next request");
            pending = &diag::operation(bytes);
        } else {
            if (!pending) throw std::runtime_error("Response without a request");
            std::string result;
            if (bytes.size()==3 && bytes[0]==0x7f && bytes[1]==pending->request[0]) {
                try { diag::decode(*pending,bytes); }
                catch (const std::runtime_error &e) { result = e.what(); }
            } else result = diag::decode(*pending,bytes);
            std::cout << pending->name << ": " << result << '\n'; pending = nullptr; ++count;
        }
    }
    if (in.bad() || pending || !count) throw std::runtime_error("Incomplete or empty diagnostic capture");
}
