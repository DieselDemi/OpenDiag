#include "diagnostics.h"
#include "j2534.h"
#include "replay.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <optional>
#include <stdexcept>
#include <thread>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

using namespace j2534;

namespace {
class Library {
#ifdef _WIN32
    HMODULE handle{};
#else
    void *handle{};
#endif
  public:
    explicit Library(const std::filesystem::path &path) {
        if (!path.is_absolute())
            throw std::runtime_error("Select an absolute provider path");
#ifdef _WIN32
        handle = LoadLibraryExW(path.c_str(), nullptr,
                                LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!handle)
            throw std::runtime_error("Cannot load provider (Windows error " + std::to_string(GetLastError()) +
                                     ")");
#else
        handle = dlopen(path.c_str(), RTLD_LOCAL | RTLD_NOW);
        if (!handle)
            throw std::runtime_error(dlerror());
#endif
    }
    template <class T> T get(const char *name, int stackBytes) {
#ifdef _WIN32
        auto p = GetProcAddress(handle, name);
        if (!p && sizeof(void *) == 4) {
            auto decorated = std::string(name) + "@" + std::to_string(stackBytes);
            p = GetProcAddress(handle, decorated.c_str());
            if (!p)
                p = GetProcAddress(handle, ("_" + decorated).c_str());
        }
#else
        (void)stackBytes;
        auto p = dlsym(handle, name);
#endif
        if (!p)
            throw std::runtime_error(std::string("Missing J2534 04.04 function: ") + name);
        return reinterpret_cast<T>(p);
    }
    ~Library() {
#ifdef _WIN32
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
    }
};

Message addressed(U32 id) {
    Message m{};
    m.protocol = iso15765;
    m.txFlags = pad;
    m.size = 4;
    for (int i = 3; i >= 0; --i, id >>= 8)
        m.data[i] = static_cast<std::uint8_t>(id);
    return m;
}

struct NoResponse : std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct Session {
    Library lib;
    Open open = lib.get<Open>("PassThruOpen", 8);
    Close close = lib.get<Close>("PassThruClose", 4);
    Connect connect = lib.get<Connect>("PassThruConnect", 20);
    Close disconnect = lib.get<Close>("PassThruDisconnect", 4);
    Filter filter = lib.get<Filter>("PassThruStartMsgFilter", 24);
    StopFilter stop = lib.get<StopFilter>("PassThruStopMsgFilter", 8);
    Messages read = lib.get<Messages>("PassThruReadMsgs", 16);
    Messages write = lib.get<Messages>("PassThruWriteMsgs", 16);
    Ioctl ioctl = lib.get<Ioctl>("PassThruIoctl", 16);
    LastError lastError = lib.get<LastError>("PassThruGetLastError", 4);
    Version version = lib.get<Version>("PassThruReadVersion", 16);
    std::optional<U32> device, channel, filterId;
    diag::Link link;
    Session(const std::filesystem::path &path, diag::Link settings) : lib(path), link(settings) {}

    void check(Status status, const char *step) {
        if (status == 0)
            return;
        char detail[256]{};
        lastError(detail);
        detail[sizeof(detail) - 1] = 0;
        throw std::runtime_error(std::string(step) + " failed (" + std::to_string(status) + "): " + detail);
    }
    std::string start(bool raw = false) {
        U32 id{};
        check(open(nullptr, &id), "Open J2534 device");
        device = id;
        char fw[256]{}, dll[256]{}, api[256]{};
        check(version(*device, fw, dll, api), "Read driver version");
        fw[255] = dll[255] = api[255] = 0;
        if (std::string(api) != "04.04")
            throw std::runtime_error(std::string("Expected J2534 API 04.04, provider reports ") + api);
        U32 flags = link.extended() ? extendedId : 0;
        check(connect(*device, raw ? can : iso15765, flags, link.baud, &id),
              raw ? "Raw CAN connect" : "ISO 15765 connect");
        channel = id;
        auto mask = addressed(0xffffffff), pattern = addressed(link.rx), flow = addressed(link.tx);
        mask.txFlags |= flags;
        pattern.txFlags |= flags;
        flow.txFlags |= flags;
        if (raw) {
            mask = pattern = addressed(0);
            mask.protocol = pattern.protocol = can;
            mask.txFlags = pattern.txFlags = flags;
        }
        check(filter(*channel, raw ? passFilter : flowControl, &mask, &pattern, raw ? nullptr : &flow, &id),
              raw ? "CAN pass filter" : "Flow-control filter");
        filterId = id;
        return std::string("API ") + api + ", driver " + dll + ", firmware " + fw;
    }
    ~Session() {
        if (filterId)
            stop(*channel, *filterId);
        if (channel)
            disconnect(*channel);
        if (device)
            close(*device);
    }
    std::string battery() {
        U32 mv{};
        auto result = ioctl(*channel, 3, nullptr, &mv);
        if (result == 1 || result == 5)
            return "Adapter voltage measurement unavailable";
        check(result, "Read battery voltage");
        return "Adapter supply: " + std::to_string(mv) + " mV";
    }
    void pollCan() {
        std::array<Message, 32> batch{};
        U32 count = U32(batch.size());
        auto result = read(*channel, batch.data(), &count, 50);
        if (result != timeout && result != empty)
            check(result, "Read CAN");
        if (count > batch.size())
            throw std::runtime_error("Invalid CAN batch size");
        for (U32 i = 0; i < count; ++i) {
            auto &m = batch[i];
            if (m.protocol != can || m.rxStatus != (link.extended() ? extendedId : 0) || m.size < 4 ||
                m.size > 12)
                continue;
            U32 id = 0;
            for (int j = 0; j < 4; ++j)
                id = (id << 8) | m.data[j];
            if (id > (link.extended() ? 0x1fffffffu : 0x7ffu))
                continue;
            std::cout << "FRAME " << m.timestamp << ' ' << diag::hex(id, link.extended() ? 8 : 3) << ' ';
            for (U32 j = 4; j < m.size; ++j)
                std::cout << diag::hex(m.data[j], 2);
            std::cout << '\n';
        }
    }
    diag::Bytes request(const diag::Operation &op) {
        auto tx = addressed(link.tx);
        U32 flags = link.extended() ? 0x100 : 0;
        tx.txFlags |= flags;
        std::copy(op.request.begin(), op.request.end(), tx.data + 4);
        tx.size += static_cast<U32>(op.request.size());
        check(ioctl(*channel, clearRx, nullptr, nullptr), "Clear receive buffer");
        U32 count = 1;
        check(write(*channel, &tx, &count, 500), "Transmit");
        if (count != 1)
            throw std::runtime_error("Provider did not transmit the request");
        using namespace std::chrono;
        auto start = steady_clock::now(), deadline = start + seconds(1);
        auto address = addressed(link.rx);
        while (steady_clock::now() < deadline) {
            Message rx{};
            count = 1;
            auto result = read(*channel, &rx, &count, 100);
            if (result == timeout || result == empty) {
                if (count == 0) {
                    std::this_thread::sleep_for(milliseconds(1));
                    continue;
                }
            } else
                check(result, "Receive");
            if (count != 1 || rx.protocol != iso15765 || rx.rxStatus != flags || rx.size <= 4 ||
                rx.size > sizeof(rx.data) || !std::equal(rx.data, rx.data + 4, address.data))
                continue;
            auto end = rx.extra ? rx.extra : rx.size;
            if (end <= 4 || end > rx.size)
                continue;
            diag::Bytes pdu(rx.data + 4, rx.data + end);
            if (pdu.size() == 3 && pdu[0] == 0x7f && pdu[1] == op.request[0]) {
                if (pdu[2] != 0x78)
                    return pdu;
                deadline = start + seconds(5);
            } else if (diag::matches(op, pdu))
                return pdu;
        }
        throw NoResponse("No matching PCM response before timeout");
    }
};

void report(const char *kind, std::string text) {
    for (auto &c : text)
        if (static_cast<unsigned char>(c) < 32)
            c = ' ';
    std::cout << kind << ' ' << text << std::endl;
}
std::string hexBytes(const diag::Bytes &bytes) {
    std::ostringstream s;
    s << std::hex << std::uppercase << std::setfill('0');
    for (auto b : bytes)
        s << ' ' << std::setw(2) << unsigned(b);
    return s.str();
}

void exchange(Session &session, const diag::Operation &op) {
    auto pair = session.link.addresses();
    report("REQUEST", pair.substr(0, pair.find('/')) + hexBytes(op.request));
    diag::Bytes reply;
    try {
        reply = session.request(op);
    } catch (const NoResponse &e) {
        report("ERR", e.what());
        return;
    }
    report("RESPONSE", pair.substr(pair.find('/') + 1) + hexBytes(reply));
    try {
        report("OK", diag::decode(op, reply));
    } catch (const std::runtime_error &e) {
        report("ERR", e.what());
    }
}
void obdScan(Session &session, const std::string &cmd) {
    struct ItemError : std::runtime_error {
        using std::runtime_error::runtime_error;
    };
    const std::string prefix = cmd == "scan:01"   ? "obd:"
                               : cmd == "scan:02" ? "freeze:"
                               : cmd == "scan:06" ? "mode6:"
                                                  : "mode9:";
    const auto pair = session.link.addresses();
    unsigned reads = 0, errors = 0;
    auto read = [&](unsigned id) {
        auto op = diag::operation(prefix + diag::hex(id, 2));
        report("REQUEST", pair.substr(0, pair.find('/')) + hexBytes(op.request));
        auto reply = session.request(op);
        report("RESPONSE", pair.substr(pair.find('/') + 1) + hexBytes(reply));
        try {
            diag::decode(op, reply);
        } catch (const std::runtime_error &e) {
            throw ItemError(e.what());
        }
        return diag::Bytes(reply.begin() + op.request.size(), reply.end());
    };
    for (unsigned base = 0; base <= 0xe0; base += 32) {
        diag::Bytes map;
        try {
            map = read(base);
        } catch (const NoResponse &e) {
            report("ERR", e.what());
            return;
        } catch (const ItemError &e) {
            report("ERR", e.what());
            return;
        }
        if (map.size() != 4) {
            report("ERR", "Invalid supported-ID bitmap");
            return;
        }
        for (unsigned bit = 0; bit < 31; ++bit)
            if (map[bit / 8] & (0x80 >> (bit % 8))) {
                unsigned id = base + bit + 1;
                const auto label = prefix + diag::hex(id, 2);
                try {
                    auto data = read(id);
                    std::string text = "raw" + hexBytes(data);
                    if ((prefix == "obd:" || prefix == "freeze:")) {
                        if (id == 0x0c && data.size() == 2)
                            text += "; " + std::to_string((data[0] * 256 + data[1]) / 4.0) + " rpm";
                        if ((id == 5 || id == 0x0f) && data.size() == 1)
                            text += "; " + std::to_string(int(data[0]) - 40) + " C";
                        if (id == 0x0d && data.size() == 1)
                            text += "; " + std::to_string(data[0]) + " km/h";
                    } else if (prefix == "mode9:" && (id == 2 || id == 4 || id == 6)) {
                        diag::Bytes record{0x49, std::uint8_t(id)};
                        record.insert(record.end(), data.begin(), data.end());
                        try {
                            text += "; " + diag::decode(diag::operation(id == 2   ? "obd-vin"
                                                                        : id == 4 ? "calid"
                                                                                  : "cvn"),
                                                        record);
                        } catch (const std::runtime_error &e) {
                            throw ItemError(e.what());
                        }
                    }
                    report("ITEM", label + " " + text);
                    ++reads;
                } catch (const NoResponse &e) {
                    report("ITEM_ERROR", label + " " + e.what());
                    ++errors;
                } catch (const ItemError &e) {
                    report("ITEM_ERROR", label + " " + e.what());
                    ++errors;
                }
            }
        if (!(map[3] & 1))
            break;
    }
    report("OK", "OBD scan: " + std::to_string(reads) + " items, " + std::to_string(errors) + " item errors");
}
void multiDtc(const std::filesystem::path &provider, const std::filesystem::path &path,
              const std::string &mask) {
    const auto op = diag::operation("ford-dtc:" + mask);
    if (std::filesystem::file_size(path) > 1024 * 1024)
        throw std::runtime_error("Module list exceeds 1 MiB");
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("Cannot open module list");
    struct Target {
        std::string name, protocol;
        unsigned id;
    };
    std::vector<Target> targets;
    std::string line;
    int n = 0;
    while (std::getline(file, line)) {
        ++n;
        if (line.empty() || line[0] != ':')
            continue;
        std::istringstream in(line.substr(1));
        std::string name, protocol, id, extra;
        unsigned value = 0;
        if (!(in >> name >> protocol >> id) || (in >> extra) || name.size() > 30 || id.size() != 4 ||
            id.find_first_not_of("0123456789ABCDEFabcdef") != id.npos || targets.size() >= 64)
            throw std::runtime_error("Invalid module list at line " + std::to_string(n));
        value = unsigned(std::stoul(id, nullptr, 16));
        for (char &c : protocol)
            if (c >= 'a' && c <= 'z')
                c -= 'a' - 'A';
        targets.push_back({name, protocol, value});
    }
    if (file.bad() || targets.empty())
        throw std::runtime_error("Unreadable or empty module list");
    for (auto &t : targets) {
        report("MODULE", t.name + " " + t.protocol + " " + diag::hex(t.id, 4));
        // Only the standard physical ISO 15765 range has an implicit response address here.
        if (t.protocol != "CAN" || t.id < 0x7e0 || t.id > 0x7e7) {
            report("SKIP", "No qualified ISO 14229 routing profile for this entry");
            continue;
        }
        diag::Link link{t.id, t.id + 8, 500000};
        Session session{provider, link};
        report("PROVIDER", session.start());
        report("OK", "Connected: module " + link.addresses() + ", ISO 15765, 500 kbit/s");
        exchange(session, op);
    }
}
} // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        if (argc == 5 && std::filesystem::path(argv[1]) == "--dtc-scan") {
            multiDtc(argv[2], argv[3], std::filesystem::path(argv[4]).string());
            return 0;
        }
        if (argc == 3 && std::filesystem::path(argv[1]) == "--replay") {
            replay(argv[2]);
            return 0;
        }
        if (argc != 2 && argc != 5)
            throw std::runtime_error("Usage: opendiag-driver PROVIDER [TX_HEX RX_HEX BAUD | --can 11|29 BAUD] "
                                     "| --replay CAPTURE.log");
        diag::Link link;
        bool raw = argc == 5 && std::filesystem::path(argv[2]) == "--can";
        if (raw) {
            auto width = std::filesystem::path(argv[3]).string();
            if (width != "11" && width != "29")
                throw std::runtime_error("Expected CAN ID width 11 or 29");
            link = diag::Link::parse(width == "11" ? "7E0" : "18DA10F1", width == "11" ? "7E8" : "18DAF110",
                                     std::filesystem::path(argv[4]).string());
        } else if (argc == 5)
            link = diag::Link::parse(std::filesystem::path(argv[2]).string(),
                                     std::filesystem::path(argv[3]).string(),
                                     std::filesystem::path(argv[4]).string());
        Session session{std::filesystem::path(argv[1]), link};
        report("PROVIDER", session.start(raw));
        if (raw) {
            report("OK", "Raw CAN monitor connected");
            std::string cmd;
            while (std::getline(std::cin, cmd) && cmd != "quit") {
                if (cmd != "poll")
                    throw std::runtime_error("Expected CAN monitor poll");
                session.pollCan();
                report("OK", "Poll complete");
            }
            return 0;
        }
        auto pair = link.addresses();
        report("OK", "Connected: " + std::string(link.tx == 0x7e0 && link.rx == 0x7e8 ? "PCM " : "module ") +
                         pair + ", ISO 15765, " + std::to_string(link.baud / 1000) + " kbit/s");
        std::string cmd;
        while (std::getline(std::cin, cmd) && cmd != "quit") {
            if (cmd == "voltage")
                report("OK", session.battery());
            else if (cmd == "scan:01" || cmd == "scan:02" || cmd == "scan:06" || cmd == "scan:09")
                obdScan(session, cmd);
            else
                exchange(session, diag::operation(cmd));
        }
    } catch (const std::exception &e) {
        report("ERR", e.what());
        return 1;
    }
    return 0;
}
