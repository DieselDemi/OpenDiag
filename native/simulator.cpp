#include "j2534.h"
#include <algorithm>
#include <cstring>
#include <vector>

using namespace j2534;
#ifdef _WIN32
#define API extern "C" __declspec(dllexport) Status PT_CALL
#else
#define API extern "C" __attribute__((visibility("default"))) Status
#endif
namespace {
bool opened{}, connected{}, filtered{};
U32 busFlags{}, txId{}, rxId{}, rpmReads{}, protocol{}, frameCounter{};
std::vector<std::uint8_t> pending;
bool addr(const Message *m, unsigned id) {
    return m && m->protocol == iso15765 && m->size >= 4 && m->data[0] == (id >> 24) &&
           m->data[1] == ((id >> 16) & 255) && m->data[2] == ((id >> 8) & 255) && m->data[3] == (id & 255);
}
} // namespace
API PassThruOpen(void *, U32 *id) {
    if (!id || opened)
        return 7;
    opened = true;
    *id = 0;
    return 0;
}
API PassThruClose(U32 id) {
    if (!opened || id != 0 || connected)
        return 7;
    opened = false;
    return 0;
}
API PassThruConnect(U32 dev, U32 proto, U32 flags, U32 baud, U32 *id) {
    if (!opened || connected || dev || !id || (proto != iso15765 && proto != can) || (flags & ~0x100u) ||
        (baud != 125000 && baud != 250000 && baud != 500000 && baud != 1000000))
        return 3;
    connected = true;
    protocol = proto;
    busFlags = flags;
    rpmReads = frameCounter = 0;
    *id = 1;
    return 0;
}
API PassThruDisconnect(U32 id) {
    if (!connected || id != 1 || filtered)
        return 2;
    pending.clear();
    connected = false;
    return 0;
}
API PassThruStartMsgFilter(U32 ch, U32 type, Message *mask, Message *pattern, Message *flow, U32 *id) {
    if (protocol == can && connected && ch == 1 && type == passFilter && id && !flow && mask && pattern &&
        mask->protocol == can && pattern->protocol == can && mask->size == 4 && pattern->size == 4 &&
        mask->txFlags == busFlags && pattern->txFlags == busFlags &&
        std::all_of(mask->data, mask->data + 4, [](int c) { return c == 0; })) {
        filtered = true;
        *id = 0;
        return 0;
    }
    if (!connected || ch != 1 || type != flowControl || !id || !addr(mask, 0xffffffff) || !pattern || !flow ||
        (pattern->txFlags & 0x100) != busFlags || (flow->txFlags & 0x100) != busFlags)
        return 7;
    if (busFlags) {
        txId = 0x18da10f1;
        rxId = 0x18daf110;
    } else if (addr(flow, 0x7e1)) {
        txId = 0x7e1;
        rxId = 0x7e9;
    } else {
        txId = 0x7e0;
        rxId = 0x7e8;
    }
    if (!addr(pattern, rxId) || !addr(flow, txId))
        return 7;
    filtered = true;
    *id = 0;
    return 0;
}
API PassThruStopMsgFilter(U32 ch, U32 id) {
    if (ch != 1 || id || !filtered)
        return 7;
    filtered = false;
    return 0;
}
API PassThruIoctl(U32 ch, U32 code, void *, void *out) {
    if (connected && ch == 1 && code == 3 && out) {
        *static_cast<U32 *>(out) = 13600;
        return 0;
    }
    if (!connected || ch != 1 || code != clearRx)
        return 5;
    pending.clear();
    return 0;
}
API PassThruGetLastError(char *dst) {
    if (!dst)
        return 4;
    std::strcpy(dst, "Invalid simulator operation");
    return 0;
}
API PassThruReadVersion(U32 id, char *fw, char *dll, char *api) {
    if (!opened || id || !fw || !dll || !api)
        return 4;
    std::strcpy(fw, "simulated");
    std::strcpy(dll, "OpenDiag simulator");
    std::strcpy(api, "04.04");
    return 0;
}
API PassThruWriteMsgs(U32 ch, Message *m, U32 *count, U32) {
    if (ch != 1 || !filtered || !count || *count != 1 || !addr(m, txId) || (m->txFlags & 0x100) != busFlags ||
        m->size <= 4 || m->size > sizeof(m->data))
        return 10;
    std::vector<std::uint8_t> q(m->data + 4, m->data + m->size);
    if (q == decltype(q){0x22, 0xd1, 0x00})
        pending = {0x62, 0xd1, 0x00, 0x01};
    else if (q == decltype(q){0x22, 0xf1, 0x88} || q == decltype(q){0x22, 0xf1, 0x90}) {
        pending = {0x62, q[1], q[2]};
        const char *text = q[2] == 0x88 ? (txId == 0x7e1 ? "SIM-TCM-001" : "SIM-PCM-001") : "TEST0000000000001";
        pending.insert(pending.end(), text, text + std::strlen(text));
    } else if (q == decltype(q){0x03})
        pending = {0x43, 1, 0x01, 0x33};
    else if (q == decltype(q){0x01, 0x0c})
        pending = {0x41, 0x0c, 0x0c, 0x80};
    else if (q == decltype(q){0x01, 0})
        pending = {0x41, 0, 0x08, 0x10, 0, 0};
    else if (q == decltype(q){0x02, 0, 0})
        pending = {0x42, 0, 0, 0, 0x10, 0, 0};
    else if (q == decltype(q){0x06, 0})
        pending = {0x46, 0, 0x80, 0, 0, 0};
    else if (q == decltype(q){0x06, 1})
        pending = {0x46, 1, 1, 1, 0, 40, 0, 20, 0, 80};
    else if (q == decltype(q){0x09, 0})
        pending = {0x49, 0, 0x54, 0, 0, 0};
    else if (q == decltype(q){0x22, 0x11, 0x65}) {
        auto value = 3200 + (rpmReads++ % 20) * 80;
        pending = {0x62, 0x11, 0x65, std::uint8_t(value >> 8), std::uint8_t(value)};
    } else if (q == decltype(q){0x22, 0x11, 0x72})
        pending = {0x62, 0x11, 0x72, 0xe0};
    else if (q == decltype(q){0x22, 0xe6, 0x11})
        pending = {0x62, 0xe6, 0x11, 'S', 'I', 'M', '-', 'I', 'D'};
    else if (q == decltype(q){0x01, 0x05})
        pending = {0x41, 0x05, 0x78};
    else if (q == decltype(q){0x02, 0x0c, 0})
        pending = {0x42, 0x0c, 0, 0x0e, 0x10};
    else if (q == decltype(q){0x07})
        pending = {0x47, 1, 0x04, 0x20};
    else if (q == decltype(q){0x0a})
        pending = {0x4a, 0};
    else if (q.size() == 3 && q[0] == 0x19 && q[1] == 2) {
        pending = {0x59, 0x02, 0xff};
        if (q[2] & 8)
            pending.insert(pending.end(), {0x01, 0x33, 0x00, 0x08});
    } else if (q == decltype(q){0x09, 0x06})
        pending = {0x49, 0x06, 1, 0x12, 0x34, 0x56, 0x78};
    else if (q == decltype(q){0x09, 0x02} || q == decltype(q){0x09, 0x04}) {
        pending = {0x49, q[1], 1};
        auto text = q[1] == 2 ? "TEST0000000000001" : "SIM-CAL-001";
        pending.insert(pending.end(), text, text + std::strlen(text));
        pending.resize(q[1] == 2 ? 20 : 19);
    } else
        pending = {0x7f, q[0], 0x31};
    return 0;
}
API PassThruReadMsgs(U32 ch, Message *m, U32 *count, U32) {
    if (protocol == can && filtered && ch == 1 && m && count && *count >= 2) {
        for (U32 i = 0; i < 2; ++i) {
            auto id = busFlags ? (0x18fe0000u + i) : (i ? 0x710u : 0x320u);
            auto &msg = m[i];
            msg = {};
            msg.protocol = can;
            msg.rxStatus = busFlags;
            for (int j = 0; j < 4; ++j)
                msg.data[j] = std::uint8_t(id >> (24 - j * 8));
            msg.data[4] = std::uint8_t(frameCounter);
            msg.data[5] = 0xe0;
            msg.timestamp = frameCounter++ * 10000;
            msg.size = 6;
        }
        *count = 2;
        return 0;
    }
    if (!filtered || ch != 1 || !m || !count || *count != 1)
        return 2;
    if (pending.empty()) {
        *count = 0;
        return empty;
    }
    *m = {};
    m->protocol = iso15765;
    m->rxStatus = busFlags;
    for (int i = 0; i < 4; ++i)
        m->data[i] = std::uint8_t(rxId >> (24 - i * 8));
    std::copy(pending.begin(), pending.end(), m->data + 4);
    m->extra = m->size = static_cast<U32>(pending.size()) + 4;
    pending.clear();
    return 0;
}
