#include "diagnostics.h"
#include "j2534.h"
#include "replay.h"
#include "seed_key.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cctype>
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
        return requestPdu(op.request, op.request[0] + 0x40, op.request[0] == 0x19 ? 2u : op.request.size(), 60,
                          false);
    }
    diag::Bytes requestPdu(const diag::Bytes &request, std::uint8_t positiveSid, size_t echoBytes = 1,
                           int pendingSeconds = 60, bool throwNegative = true) {
        if (request.empty())
            throw std::runtime_error("Empty request");
        auto tx = addressed(link.tx);
        U32 flags = link.extended() ? 0x100 : 0;
        tx.txFlags |= flags;
        if (request.size() + 4 > sizeof(tx.data))
            throw std::runtime_error("Request exceeds J2534 message size");
        std::copy(request.begin(), request.end(), tx.data + 4);
        tx.size += static_cast<U32>(request.size());
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
            if (pdu.size() >= 3 && pdu[0] == 0x7f && pdu[1] == request[0]) {
                if (pdu[2] != 0x78) {
                    if (!throwNegative)
                        return pdu;
                    throw std::runtime_error("ECU negative response 0x" + diag::hex(pdu[2], 2));
                }
                deadline = start + seconds(pendingSeconds);
            } else if (pdu.size() >= echoBytes && pdu[0] == positiveSid &&
                       std::equal(request.begin() + 1, request.begin() + echoBytes, pdu.begin() + 1))
                return pdu;
        }
        throw NoResponse("No matching ECU response before timeout");
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
std::string compactHex(const seedkey::Bytes &bytes) {
    std::ostringstream s;
    s << std::hex << std::uppercase << std::setfill('0');
    for (auto b : bytes)
        s << std::setw(2) << unsigned(b);
    return s.str();
}
unsigned hexByte(char high, char low) {
    auto digit = [](char c) -> int {
        if (c >= '0' && c <= '9')
            return c - '0';
        c = char(std::toupper(static_cast<unsigned char>(c)));
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        return -1;
    };
    auto a = digit(high), b = digit(low);
    if (a < 0 || b < 0)
        throw std::runtime_error("Expected hex bytes");
    return unsigned(a * 16 + b);
}
seedkey::Bytes bytesFromHex(std::string text) {
    text.erase(std::remove_if(text.begin(), text.end(), [](unsigned char c) { return std::isspace(c); }),
               text.end());
    if (text.size() % 2)
        throw std::runtime_error("Expected an even number of hex digits");
    seedkey::Bytes bytes;
    for (size_t i = 0; i < text.size(); i += 2)
        bytes.push_back(std::uint8_t(hexByte(text[i], text[i + 1])));
    return bytes;
}
std::uint8_t levelFromHex(const std::string &text) {
    if (text.empty() || text.size() > 2)
        throw std::runtime_error("Expected one-byte security level");
    auto bytes = bytesFromHex(text.size() == 1 ? "0" + text : text);
    return bytes.front();
}
void seedKey(const std::string &profile, const std::string &level, const std::string &seed,
             const std::string &security = {}) {
    report("KEY", compactHex(seedkey::key({profile, levelFromHex(level), bytesFromHex(seed), bytesFromHex(security)})));
}
void seedKeyProfiles() {
    for (const auto &profile : seedkey::profiles())
        report("PROFILE", profile.name + " seed=" + std::to_string(profile.seedLength) +
                              " security=" + std::to_string(profile.securityBytesLength) +
                              " key=" + std::to_string(profile.keyLength) +
                              (profile.builtIn ? " built-in" : " external-or-unrecovered"));
}
void seedKeyCommand(const std::string &cmd) {
    std::istringstream in(cmd);
    std::string verb, profile, level, seed, security, extra;
    in >> verb >> profile >> level >> seed >> security;
    if (verb != "seed-key" || profile.empty() || level.empty() || seed.empty() || (in >> extra))
        throw std::runtime_error("Expected seed-key PROFILE LEVEL_HEX SEED_HEX [SECURITY_HEX]");
    seedKey(profile, level, seed, security);
}
unsigned long long hexNumber(const std::string &text, const char *what) {
    if (text.empty() || text.find_first_not_of("0123456789abcdefABCDEF") != text.npos)
        throw std::runtime_error(std::string("Expected hex ") + what);
    return std::stoull(text, nullptr, 16);
}
diag::Bytes integerBytes(unsigned long long value) {
    diag::Bytes bytes;
    bool seen = false;
    for (int shift = 56; shift >= 0; shift -= 8) {
        auto b = std::uint8_t(value >> shift);
        if (b || seen || shift == 0) {
            bytes.push_back(b);
            seen = true;
        }
    }
    return bytes;
}
diag::Bytes downloadRequest(const std::string &addressText, const std::string &sizeText,
                            const std::string &formatText) {
    auto address = integerBytes(hexNumber(addressText, "download address"));
    auto size = integerBytes(hexNumber(sizeText, "download size"));
    if (address.size() > 8 || size.size() > 8)
        throw std::runtime_error("Address and size must fit UDS length format");
    auto format = levelFromHex(formatText);
    diag::Bytes request{0x34, format, std::uint8_t((address.size() << 4) | size.size())};
    request.insert(request.end(), address.begin(), address.end());
    request.insert(request.end(), size.begin(), size.end());
    return request;
}
diag::Bytes readBinaryFile(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Cannot open programming file");
    return diag::Bytes(std::istreambuf_iterator<char>(file), {});
}
void requestAndReport(Session &session, const diag::Bytes &request, std::uint8_t positiveSid,
                      size_t echoBytes = 1, int pendingSeconds = 60) {
    auto pair = session.link.addresses();
    report("REQUEST", pair.substr(0, pair.find('/')) + hexBytes(request));
    auto reply = session.requestPdu(request, positiveSid, echoBytes, pendingSeconds);
    report("RESPONSE", pair.substr(pair.find('/') + 1) + hexBytes(reply));
    report("OK", "UDS service 0x" + diag::hex(request.front(), 2) + " complete");
}
void securityAccess(Session &session, const std::string &profile, const std::string &levelText,
                    const std::string &securityBytesText) {
    auto level = levelFromHex(levelText);
    diag::Bytes seedRequest{0x27, level};
    auto pair = session.link.addresses();
    report("REQUEST", pair.substr(0, pair.find('/')) + hexBytes(seedRequest));
    auto seedReply = session.requestPdu(seedRequest, 0x67, 2, 60);
    report("RESPONSE", pair.substr(pair.find('/') + 1) + hexBytes(seedReply));
    if (seedReply.size() < 3)
        throw std::runtime_error("Security seed response did not include a seed");
    seedkey::Bytes seed(seedReply.begin() + 2, seedReply.end());
    auto key = seedkey::key({profile, level, seed, bytesFromHex(securityBytesText)});
    diag::Bytes keyRequest{0x27, std::uint8_t(level + 1)};
    keyRequest.insert(keyRequest.end(), key.begin(), key.end());
    report("REQUEST", pair.substr(0, pair.find('/')) + hexBytes(keyRequest));
    auto keyReply = session.requestPdu(keyRequest, 0x67, 2, 60);
    report("RESPONSE", pair.substr(pair.find('/') + 1) + hexBytes(keyReply));
    report("OK", "Security access granted with " + profile);
}
void flashFile(Session &session, const std::string &addressText, const std::filesystem::path &path,
               const std::string &chunkText, const std::string &formatText) {
    auto data = readBinaryFile(path);
    if (data.empty())
        throw std::runtime_error("Programming file is empty");
    const auto request = downloadRequest(addressText, diag::hex(unsigned(data.size()), 1), formatText);
    auto pair = session.link.addresses();
    report("REQUEST", pair.substr(0, pair.find('/')) + hexBytes(request));
    auto reply = session.requestPdu(request, 0x74, 1, 60);
    report("RESPONSE", pair.substr(pair.find('/') + 1) + hexBytes(reply));
    size_t ecuMax = 0;
    if (reply.size() >= 3) {
        const auto length = reply[1] & 0xf;
        for (size_t i = 0; i < length && 2 + i < reply.size(); ++i)
            ecuMax = (ecuMax << 8) | reply[2 + i];
    }
    size_t chunk = chunkText.empty() ? 512 : size_t(hexNumber(chunkText, "transfer chunk size"));
    if (ecuMax > 2)
        chunk = std::min(chunk, ecuMax - 2);
    if (!chunk || chunk > 4093)
        throw std::runtime_error("Invalid transfer chunk size");
    size_t offset = 0;
    unsigned sequence = 1;
    while (offset < data.size()) {
        const auto count = std::min(chunk, data.size() - offset);
        diag::Bytes transfer{0x36, std::uint8_t(sequence)};
        transfer.insert(transfer.end(), data.begin() + offset, data.begin() + offset + count);
        report("REQUEST", pair.substr(0, pair.find('/')) + " 36 " + diag::hex(sequence & 0xff, 2) +
                              " [" + std::to_string(count) + " bytes]");
        auto transferReply = session.requestPdu(transfer, 0x76, 2, 60);
        report("RESPONSE", pair.substr(pair.find('/') + 1) + hexBytes(transferReply));
        offset += count;
        sequence = (sequence + 1) & 0xff;
        if (!sequence)
            sequence = 0;
        report("PROGRESS", std::to_string(offset) + "/" + std::to_string(data.size()));
    }
    requestAndReport(session, {0x37}, 0x77, 1, 60);
}
struct ProgramProfile {
    const char *name;
    const char *session;
    const char *eraseRoutine;
    const char *checkRoutine;
    const char *reset;
    const char *format;
    const char *chunk;
};
const ProgramProfile &programProfile(const std::string &name) {
    static const ProgramProfile profiles[] = {
        {"pcm", "02", "FF00", "", "01", "00", "0200"},
        {"generic", "02", "FF00", "", "01", "00", "0200"},
        {"block", "03", "FF00", "", "", "00", "0100"},
        {"kam", "03", "FF00", "", "", "00", "0100"},
        {"vid-trid", "03", "FF00", "", "", "00", "0100"},
    };
    for (auto &profile : profiles)
        if (name == profile.name)
            return profile;
    throw std::runtime_error("Unknown program profile; expected pcm, generic, block, kam, or vid-trid");
}
void automatedProgram(Session &session, const std::string &profileName, const std::string &address,
                      const std::filesystem::path &file, const std::string &securityProfile,
                      const std::string &securityLevel, const std::string &securityBytes,
                      const std::string &eraseData) {
    const auto &profile = programProfile(profileName);
    report("STAGE", "program " + profileName + " begin");
    if (*profile.session) {
        report("STAGE", std::string("session ") + profile.session);
        requestAndReport(session, {0x10, levelFromHex(profile.session)}, 0x50, 2, 60);
    }
    if (!securityProfile.empty() && securityProfile != "none") {
        report("STAGE", "security " + securityProfile);
        securityAccess(session, securityProfile, securityLevel.empty() ? "01" : securityLevel, securityBytes);
    }
    if (*profile.eraseRoutine) {
        report("STAGE", std::string("erase ") + profile.eraseRoutine);
        auto rid = bytesFromHex(profile.eraseRoutine);
        auto payload = bytesFromHex(eraseData);
        diag::Bytes request{0x31, 0x01, rid[0], rid[1]};
        request.insert(request.end(), payload.begin(), payload.end());
        requestAndReport(session, request, 0x71, 4, 300);
    }
    report("STAGE", "download");
    flashFile(session, address, file, profile.chunk, profile.format);
    if (*profile.checkRoutine) {
        report("STAGE", std::string("check ") + profile.checkRoutine);
        auto rid = bytesFromHex(profile.checkRoutine);
        requestAndReport(session, {0x31, 0x01, rid[0], rid[1]}, 0x71, 4, 300);
    }
    if (*profile.reset) {
        report("STAGE", std::string("reset ") + profile.reset);
        requestAndReport(session, {0x11, levelFromHex(profile.reset)}, 0x51, 2, 30);
    }
    report("OK", "Program profile " + profileName + " complete");
}
void programmingCommand(Session &session, const std::string &cmd) {
    std::istringstream in(cmd);
    std::string verb;
    in >> verb;
    if (verb == "uds") {
        std::string request, extra;
        in >> request;
        if (request.empty() || (in >> extra))
            throw std::runtime_error("Expected uds REQUEST_HEX");
        auto bytes = bytesFromHex(request);
        requestAndReport(session, bytes, bytes.front() + 0x40);
    } else if (verb == "session") {
        std::string sub, extra;
        in >> sub;
        if (sub.empty() || (in >> extra))
            throw std::runtime_error("Expected session SUBFUNCTION_HEX");
        requestAndReport(session, {0x10, levelFromHex(sub)}, 0x50, 2);
    } else if (verb == "reset") {
        std::string sub, extra;
        in >> sub;
        if (sub.empty() || (in >> extra))
            throw std::runtime_error("Expected reset TYPE_HEX");
        requestAndReport(session, {0x11, levelFromHex(sub)}, 0x51, 2);
    } else if (verb == "security") {
        std::string profile, level, security, extra;
        in >> profile >> level >> security;
        if (profile.empty() || level.empty() || (in >> extra))
            throw std::runtime_error("Expected security PROFILE LEVEL_HEX [SECURITY_HEX]");
        securityAccess(session, profile, level, security);
    } else if (verb == "routine" || verb == "erase") {
        std::string action, routine, data, extra;
        if (verb == "erase") {
            action = "01";
            in >> routine >> data;
        } else
            in >> action >> routine >> data;
        if (action.empty() || routine.empty() || (in >> extra))
            throw std::runtime_error("Expected routine ACTION_HEX ROUTINE_HEX [DATA_HEX]");
        auto rid = bytesFromHex(routine);
        if (rid.size() != 2)
            throw std::runtime_error("Routine identifier must be two bytes");
        auto payload = bytesFromHex(data);
        diag::Bytes request{0x31, levelFromHex(action), rid[0], rid[1]};
        request.insert(request.end(), payload.begin(), payload.end());
        requestAndReport(session, request, 0x71, 4, 120);
    } else if (verb == "request-download") {
        std::string address, size, format = "00", extra;
        in >> address >> size;
        if (!(in >> format))
            format = "00";
        if (address.empty() || size.empty() || (in >> extra))
            throw std::runtime_error("Expected request-download ADDRESS_HEX SIZE_HEX [FORMAT_HEX]");
        requestAndReport(session, downloadRequest(address, size, format), 0x74, 1, 60);
    } else if (verb == "transfer") {
        std::string sequence, data, extra;
        in >> sequence >> data;
        if (sequence.empty() || data.empty() || (in >> extra))
            throw std::runtime_error("Expected transfer SEQUENCE_HEX DATA_HEX");
        auto payload = bytesFromHex(data);
        diag::Bytes request{0x36, levelFromHex(sequence)};
        request.insert(request.end(), payload.begin(), payload.end());
        requestAndReport(session, request, 0x76, 2, 60);
    } else if (verb == "transfer-exit") {
        std::string data, extra;
        in >> data;
        if (in >> extra)
            throw std::runtime_error("Expected transfer-exit [DATA_HEX]");
        auto payload = bytesFromHex(data);
        diag::Bytes request{0x37};
        request.insert(request.end(), payload.begin(), payload.end());
        requestAndReport(session, request, 0x77, 1, 60);
    } else if (verb == "flash") {
        std::string address, file, chunk, format = "00", extra;
        in >> address >> file >> chunk;
        if (!(in >> format))
            format = "00";
        if (address.empty() || file.empty() || (in >> extra))
            throw std::runtime_error("Expected flash ADDRESS_HEX FILE [CHUNK_HEX] [FORMAT_HEX]");
        flashFile(session, address, file, chunk, format);
    } else if (verb == "program") {
        std::string profile, address, file, securityProfile = "none", securityLevel = "01", securityBytes, eraseData,
            extra;
        in >> profile >> address >> file;
        if (profile.empty() || address.empty() || file.empty())
            throw std::runtime_error("Expected program PROFILE ADDRESS_HEX FILE [SECURITY_PROFILE LEVEL_HEX SECURITY_HEX [ERASE_DATA_HEX]]");
        in >> securityProfile >> securityLevel >> securityBytes >> eraseData;
        if (in >> extra)
            throw std::runtime_error("Expected program PROFILE ADDRESS_HEX FILE [SECURITY_PROFILE LEVEL_HEX SECURITY_HEX [ERASE_DATA_HEX]]");
        automatedProgram(session, profile, address, file, securityProfile, securityLevel, securityBytes, eraseData);
    } else
        throw std::runtime_error("Unknown programming command");
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
        if (argc == 2 && std::filesystem::path(argv[1]) == "--seed-key-profiles") {
            seedKeyProfiles();
            return 0;
        }
        if ((argc == 5 || argc == 6) && std::filesystem::path(argv[1]) == "--seed-key") {
            seedKey(std::filesystem::path(argv[2]).string(), std::filesystem::path(argv[3]).string(),
                    std::filesystem::path(argv[4]).string(),
                    argc == 6 ? std::filesystem::path(argv[5]).string() : std::string{});
            return 0;
        }
        if (argc != 2 && argc != 5)
            throw std::runtime_error("Usage: opendiag-driver PROVIDER [TX_HEX RX_HEX BAUD | --can 11|29 BAUD] "
                                     "| --replay CAPTURE.log | --seed-key-profiles | --seed-key PROFILE LEVEL_HEX SEED_HEX [SECURITY_HEX]");
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
            else if (cmd.rfind("seed-key ", 0) == 0)
                seedKeyCommand(cmd);
            else if (cmd.rfind("uds ", 0) == 0 || cmd.rfind("session ", 0) == 0 ||
                     cmd.rfind("reset ", 0) == 0 || cmd.rfind("security ", 0) == 0 ||
                     cmd.rfind("routine ", 0) == 0 || cmd.rfind("erase ", 0) == 0 ||
                     cmd.rfind("request-download ", 0) == 0 || cmd.rfind("transfer ", 0) == 0 ||
                     cmd.rfind("transfer-exit", 0) == 0 || cmd.rfind("flash ", 0) == 0 ||
                     cmd.rfind("program ", 0) == 0)
                programmingCommand(session, cmd);
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
