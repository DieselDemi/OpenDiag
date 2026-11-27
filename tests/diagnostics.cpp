#include "../native/diagnostics.h"
#include <array>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
template<class F> bool rejects(F f){try{f();return false;}catch(const std::exception &){return true;}}
}
int main(){
    try{
        auto pcm=diag::Link::parse("7E0","7E8","500000");
        check(!pcm.extended()&&pcm.addresses()=="7E0/7E8","11-bit physical addressing");
        auto can29=diag::Link::parse("18DA10F1","18DAF110","500000");
        check(can29.extended()&&can29.addresses()=="18DA10F1/18DAF110","29-bit physical addressing");
        const std::array<std::array<const char*,3>,4> invalid{{
            {{"7DF","7E8","500000"}},{{"7E0","18DAF110","500000"}},
            {{"18DA10F1","18DAF110","10000"}},{{"7E0","7E0","500000"}}}};
        for(const auto &bad:invalid)
            check(rejects([&]{diag::Link::parse(bad[0],bad[1],bad[2]);}),"Invalid physical link accepted");
        auto ping=diag::operation("ping");check(diag::decode(ping,{0x62,0xd1,0,1}).find("GGDS")!=std::string::npos,"Ford ping");
        check(diag::decode(diag::operation("dtc"),{0x43,1,0x01,0x33})=="P0133","Mode 03 decode");
        check(diag::decode(diag::operation("ford-dtc"),{0x59,2,0xff,1,0x33,0,8}).find("013300 status 0x08")!=std::string::npos,"Ford DTC decode");
        diag::Bytes vin{0x49,0x02,0x01};const std::string vinText="TEST0000000000001";
        vin.insert(vin.end(),vinText.begin(),vinText.end());
        check(diag::decode(diag::operation("obd-vin"),vin)==vinText,"Mode 09 VIN");
        check(rejects([&]{diag::decode(diag::operation("vin"),{0x62,0xf1,0x90,'I','N','V','A','L','I','D','0','0','0','0','0','0','0','0'});}),"Malformed VIN accepted");
        check(rejects([&]{diag::decode(ping,{0x7f,0x22,0x31});}),"Wrong ECU negative response accepted");
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
    return 0;
}
