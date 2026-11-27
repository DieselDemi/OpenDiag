#pragma once
#include <cstdint>
#include <cstddef>

// SAE J2534-1 04.04 uses 32-bit scalars even on LP64 hosts.
namespace j2534 {
using U32 = std::uint32_t;
using Status = std::int32_t;
struct Message {
    U32 protocol, rxStatus, txFlags, timestamp, size, extra;
    std::uint8_t data[4128];
};
static_assert(sizeof(Message) == 4152 && offsetof(Message, data) == 24);
constexpr U32 iso15765 = 6, flowControl = 3, pad = 0x40, clearRx = 8;
constexpr U32 can = 5, passFilter = 1, extendedId = 0x100;
constexpr Status timeout = 9, empty = 0x10;
#ifdef _WIN32
#define PT_CALL __stdcall
#else
#define PT_CALL
#endif
using Open = Status(PT_CALL *)(void *, U32 *);
using Close = Status(PT_CALL *)(U32);
using Connect = Status(PT_CALL *)(U32, U32, U32, U32, U32 *);
using Filter = Status(PT_CALL *)(U32, U32, Message *, Message *, Message *, U32 *);
using StopFilter = Status(PT_CALL *)(U32, U32);
using Messages = Status(PT_CALL *)(U32, Message *, U32 *, U32);
using Ioctl = Status(PT_CALL *)(U32, U32, void *, void *);
using LastError = Status(PT_CALL *)(char *);
using Version = Status(PT_CALL *)(U32, char *, char *, char *);
} // namespace j2534
