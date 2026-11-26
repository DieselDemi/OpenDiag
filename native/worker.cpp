#include "diagnostics.h"
#include "j2534.h"
#include "replay.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
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
        if (!path.is_absolute()) throw std::runtime_error("Select an absolute provider path");
#ifdef _WIN32
        handle = LoadLibraryExW(path.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!handle) throw std::runtime_error("Cannot load provider (Windows error " + std::to_string(GetLastError()) + ")");
#else
        handle = dlopen(path.c_str(), RTLD_LOCAL | RTLD_NOW);
        if (!handle) throw std::runtime_error(dlerror());
#endif
    }
    template<class T> T get(const char *name, int stackBytes) {
#ifdef _WIN32
        auto p = GetProcAddress(handle, name);
        if (!p && sizeof(void *) == 4) {
            auto decorated = std::string(name) + "@" + std::to_string(stackBytes);
            p = GetProcAddress(handle, decorated.c_str());
            if (!p) p = GetProcAddress(handle, ("_" + decorated).c_str());
        }
#else
        (void)stackBytes;
        auto p = dlsym(handle, name);
#endif
        if (!p) throw std::runtime_error(std::string("Missing J2534 04.04 function: ") + name);
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
    for (int i = 3; i >= 0; --i, id >>= 8) m.data[i] = static_cast<std::uint8_t>(id);
    return m;
}

struct NoResponse : std::runtime_error { using std::runtime_error::runtime_error; };
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
    explicit Session(const std::filesystem::path &path) : lib(path) {}

    void check(Status status, const char *step) {
        if (status == 0) return;
        char detail[256]{};
        lastError(detail);
        detail[sizeof(detail)-1] = 0;
        throw std::runtime_error(std::string(step) + " failed (" + std::to_string(status) + "): " + detail);
    }
    std::string start() {
        U32 id{};
        check(open(nullptr, &id), "Open");
        device = id;
        char fw[256]{}, dll[256]{}, api[256]{};
        check(version(*device, fw, dll, api), "Read driver version");
        fw[255] = dll[255] = api[255] = 0;
        if (std::string(api) != "04.04") throw std::runtime_error(std::string("Expected J2534 API 04.04, provider reports ") + api);
        check(connect(*device, iso15765, 0, 500000, &id), "ISO 15765 connect");
        channel = id;
        auto mask = addressed(0xffffffff), pattern = addressed(0x7e8), flow = addressed(0x7e0);
        check(filter(*channel, flowControl, &mask, &pattern, &flow, &id), "Flow-control filter");
        filterId = id;
        return std::string("API ") + api + ", driver " + dll + ", firmware " + fw;
    }
    ~Session() {
        if (filterId) stop(*channel, *filterId);
        if (channel) disconnect(*channel);
        if (device) close(*device);
    }
    diag::Bytes request(const diag::Operation &op) {
        auto tx = addressed(0x7e0);
        std::copy(op.request.begin(), op.request.end(), tx.data + 4);
        tx.size += static_cast<U32>(op.request.size());
        check(ioctl(*channel, clearRx, nullptr, nullptr), "Clear receive buffer");
        U32 count = 1;
        check(write(*channel, &tx, &count, 500), "Transmit");
        if (count != 1) throw std::runtime_error("Provider did not transmit the request");
        using namespace std::chrono;
        auto start = steady_clock::now(), deadline = start + seconds(1);
        auto address = addressed(0x7e8);
        while (steady_clock::now() < deadline) {
            Message rx{};
            count = 1;
            auto result = read(*channel, &rx, &count, 100);
            if (result == timeout || result == empty) {
                if (count == 0) { std::this_thread::sleep_for(milliseconds(1)); continue; }
            } else check(result, "Receive");
            if (count != 1 || rx.protocol != iso15765 || rx.rxStatus != 0 || rx.size <= 4 ||
                rx.size > sizeof(rx.data) || !std::equal(rx.data, rx.data+4, address.data)) continue;
            auto end = rx.extra ? rx.extra : rx.size;
            if (end <= 4 || end > rx.size) continue;
            diag::Bytes pdu(rx.data+4, rx.data+end);
            if (pdu.size() == 3 && pdu[0] == 0x7f && pdu[1] == op.request[0]) {
                if (pdu[2] != 0x78) return pdu;
                deadline = start + seconds(5);
            } else if (pdu.size() >= op.request.size() && pdu[0] == op.request[0] + 0x40 &&
                       std::equal(op.request.begin()+1, op.request.end(), pdu.begin()+1)) return pdu;
        }
        throw NoResponse("No matching PCM response before timeout");
    }
};

void report(const char *kind, std::string text) {
    for (auto &c : text) if (static_cast<unsigned char>(c) < 32) c = ' ';
    std::cout << kind << ' ' << text << std::endl;
}
std::string hexBytes(const diag::Bytes &bytes) {
    std::ostringstream s;
    s << std::hex << std::uppercase << std::setfill('0');
    for (auto b : bytes) s << ' ' << std::setw(2) << unsigned(b);
    return s.str();
}
}

#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        if (argc == 3 && std::filesystem::path(argv[1]) == "--replay") { replay(argv[2]); return 0; }
        if (argc != 2) throw std::runtime_error("Usage: opendiag-driver ABSOLUTE_PROVIDER_PATH | --replay CAPTURE.log");
        Session session{std::filesystem::path(argv[1])};
        report("PROVIDER", session.start());
        report("OK", "Connected: PCM 7E0/7E8, ISO 15765, 500 kbit/s");
        std::string cmd;
        while (std::getline(std::cin, cmd) && cmd != "quit") {
            const auto &op = diag::operation(cmd);
            diag::Bytes reply;
            report("REQUEST", "7E0" + hexBytes(op.request));
            try { reply = session.request(op); }
            catch (const NoResponse &e) { report("ERR", e.what()); continue; }
            report("RESPONSE", "7E8" + hexBytes(reply));
            try { report("OK", diag::decode(op, reply)); }
            catch (const std::runtime_error &e) { report("ERR", e.what()); }
        }
    } catch (const std::exception &e) { report("ERR", e.what()); return 1; }
    return 0;
}
