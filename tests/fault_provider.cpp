#include "../native/j2534.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
using namespace j2534;
#ifdef _WIN32
#define API extern "C" __declspec(dllexport) Status PT_CALL
#else
#define API extern "C" Status
#endif
static void trace(const char *s) { std::ofstream(std::getenv("OD_TRACE"), std::ios::app) << s << '\n'; }
static std::string mode() { return std::getenv("OD_FAULT"); }
static int step;
API PassThruOpen(void *, U32 *d) { trace("open"); *d=0; return 0; }
API PassThruClose(U32) { trace("close"); return 0; }
API PassThruConnect(U32,U32,U32,U32,U32 *c) { trace("connect"); *c=0; return 0; }
API PassThruDisconnect(U32) { trace("disconnect"); return 0; }
API PassThruStartMsgFilter(U32,U32,Message*,Message*,Message*,U32 *f) { trace("filter"); *f=0; return mode()=="filter" ? 1 : 0; }
API PassThruStopMsgFilter(U32,U32) { trace("stop"); return 0; }
API PassThruGetLastError(char *err) { std::strcpy(err,"fixture failure"); return 0; }
API PassThruReadVersion(U32,char *fw,char *dll,char *api) {
    *fw=*dll=0; std::strcpy(api, mode()=="version" ? "05.00" : "04.04"); return 0;
}
API PassThruIoctl(U32,U32,void*,void*) { return 0; }
API PassThruWriteMsgs(U32,Message*,U32*,U32) { step=0; trace("tx"); return 0; }
API PassThruReadMsgs(U32,Message *m,U32 *n,U32) {
    if (mode()=="unplug") return 8;
    if (mode()=="timeout") { *n=0; return 9; }
    const unsigned char frame[]={0,0,7,0xe8,0x62,0xd1,0,0x85};
    m->protocol=6; m->size=sizeof(frame); std::memcpy(m->data,frame,sizeof(frame));
    if (mode()=="negative" || step==3) {
        m->size=7; m->data[4]=0x7f; m->data[5]=0x22; m->data[6]=mode()=="negative" ? 0x31 : 0x78;
    } else if (step==0) { m->data[3]=0xe9; m->data[7]=0xff; }
    else if (step==1) { m->rxStatus=2; m->data[7]=0xff; }
    else if (step==2) { m->size=5000; }
    ++step; return 0;
}
