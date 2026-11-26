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
std::vector<std::uint8_t> pending;
bool addr(const Message *m, unsigned id) {
    return m && m->protocol == iso15765 && m->size >= 4 && m->data[0] == (id >> 24) &&
        m->data[1] == ((id >> 16) & 255) && m->data[2] == ((id >> 8) & 255) && m->data[3] == (id & 255);
}
}
API PassThruOpen(void *, U32 *id) {
    if (!id || opened) return 7;
    opened = true; *id = 0; return 0;
}
API PassThruClose(U32 id) {
    if (!opened || id != 0 || connected) return 7;
    opened = false; return 0;
}
API PassThruConnect(U32 dev, U32 proto, U32 flags, U32 baud, U32 *id) {
    if (!opened || connected || dev || !id || proto != iso15765 || flags || baud != 500000) return 3;
    connected = true; *id = 1; return 0;
}
API PassThruDisconnect(U32 id) {
    if (!connected || id != 1 || filtered) return 2;
    pending.clear(); connected = false; return 0;
}
API PassThruStartMsgFilter(U32 ch, U32 type, Message *mask, Message *pattern, Message *flow, U32 *id) {
    if (!connected || ch != 1 || type != flowControl || !id || !addr(mask, 0xffffffff) ||
        !addr(pattern, 0x7e8) || !addr(flow, 0x7e0)) return 7;
    filtered = true; *id = 0; return 0;
}
API PassThruStopMsgFilter(U32 ch, U32 id) {
    if (ch != 1 || id || !filtered) return 7;
    filtered = false; return 0;
}
API PassThruIoctl(U32 ch, U32 code, void *, void *) {
    if (!connected || ch != 1 || code != clearRx) return 5;
    pending.clear(); return 0;
}
API PassThruGetLastError(char *dst) {
    if (!dst) return 4;
    std::strcpy(dst, "Invalid simulator operation"); return 0;
}
API PassThruReadVersion(U32 id, char *fw, char *dll, char *api) {
    if (!opened || id || !fw || !dll || !api) return 4;
    std::strcpy(fw,"simulated"); std::strcpy(dll,"OpenDiag simulator"); std::strcpy(api,"04.04"); return 0;
}
API PassThruWriteMsgs(U32 ch, Message *m, U32 *count, U32) {
    if (ch != 1 || !filtered || !count || *count != 1 || !addr(m, 0x7e0) ||
        m->size <= 4 || m->size > sizeof(m->data)) return 10;
    std::vector<std::uint8_t> q(m->data+4, m->data+m->size);
    if (q == decltype(q){0x22,0xd1,0x00}) pending = {0x62,0xd1,0x00,0x01};
    else if (q == decltype(q){0x22,0xf1,0x88} || q == decltype(q){0x22,0xf1,0x90}) {
        pending = {0x62,q[1],q[2]};
        const char *text = q[2] == 0x88 ? "SIM-PCM-001" : "TEST0000000000001";
        pending.insert(pending.end(), text, text+std::strlen(text));
    } else if (q == decltype(q){0x03}) pending = {0x43,1,0x01,0x33};
    else if (q == decltype(q){0x01,0x0c}) pending = {0x41,0x0c,0x0c,0x80};
    else pending = {0x7f,q[0],0x31};
    return 0;
}
API PassThruReadMsgs(U32 ch, Message *m, U32 *count, U32) {
    if (!filtered || ch != 1 || !m || !count || *count != 1) return 2;
    if (pending.empty()) { *count = 0; return empty; }
    *m = {}; m->protocol = iso15765; m->data[2] = 7; m->data[3] = 0xe8;
    std::copy(pending.begin(), pending.end(), m->data+4);
    m->extra = m->size = static_cast<U32>(pending.size())+4;
    pending.clear(); return 0;
}
