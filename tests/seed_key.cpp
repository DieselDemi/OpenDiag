#include "../native/seed_key.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
template <class F> bool rejects(F f) {
    try {
        f();
        return false;
    } catch (const std::exception &) {
        return true;
    }
}
} // namespace

int main() {
    try {
        const auto &profiles = seedkey::profiles();
        check(!profiles.empty(), "No seed-key profiles registered");
        check(std::any_of(profiles.begin(), profiles.end(),
                          [](const seedkey::Profile &p) { return p.name == "ptdiag-osc"; }),
              "PTDIAG OSC profile missing");
        check(std::any_of(profiles.begin(), profiles.end(),
                          [](const seedkey::Profile &p) { return p.name == "ptdiag-dll-keydown"; }),
              "PTDIAG DLL-backed profile missing");
        check(rejects([] { seedkey::key({"missing", 0, {0x12, 0x34}, {}}); }),
              "Unknown profile accepted");
        check(rejects([] { seedkey::key({"ptdiag-osc", 0, {0x12}, {}}); }),
              "Bad seed length accepted");
        check(rejects([] { seedkey::key({"ptdiag-mazda-dmr", 0, {0x12, 0x34}, {}}); }),
              "Unverified algorithm returned a key");
        check(rejects([] { seedkey::key({"ptdiag-legacy-2byte", 0, {0x12, 0x34}, {0, 2, 0}}); }),
              "Bad security byte length accepted");
        check((seedkey::key({"ptdiag-legacy-2byte", 0, {0x12, 0x34}, {0, 2, 0, 0}}) ==
               seedkey::Bytes{0xea, 0x5e}),
              "PTDIAG legacy 2-byte key mismatch");
        check((seedkey::key({"ptdiag-legacy-2byte", 0, {0xab, 0xcd}, {0, 3, 0, 0x1b}}) ==
               seedkey::Bytes{0x17, 0x34}),
              "PTDIAG legacy odd-factor key mismatch");
        check((seedkey::key({"ptdiag-legacy-83k", 0, {0x12, 0x34}, {0, 2, 0, 0}}) ==
               seedkey::Bytes{0xff, 0x2f}),
              "PTDIAG legacy 83K key mismatch");
        check((seedkey::key({"ptdiag-legacy-ezref", 0, {0x12, 0x34}, {0, 2, 0, 0}}) ==
               seedkey::Bytes{0x22, 0x62}),
              "PTDIAG legacy EZREF key mismatch");
        check((seedkey::key({"ptdiag-iso-2byte", 0, {0x12, 0x34}, {0, 2, 0, 0}}) ==
               seedkey::Bytes{0x22, 0x62}),
              "PTDIAG ISO key mismatch");
        check((seedkey::key({"ptdiag-legacy-ezref-83k", 0, {0x12, 0x34}, {0, 2, 0, 0}}) ==
               seedkey::Bytes{0x3e, 0x8e}),
              "PTDIAG legacy EZREF 83K key mismatch");
        check((seedkey::key({"ptdiag-generic-security-bytes", 0, {0x12, 0x34, 0x56}, {0, 0}}) ==
               seedkey::Bytes{0x1d, 0xad, 0xbf}),
              "PTDIAG generic security type 0 key mismatch");
        check((seedkey::key({"ptdiag-generic-security-bytes", 0, {0x12, 0x34, 0x56}, {0, 2}}) ==
               seedkey::Bytes{0x86, 0xca, 0x06}),
              "PTDIAG generic security type 2 key mismatch");
        check((seedkey::key({"ptdiag-generic-security-bytes", 0, {0x12, 0x34, 0x56}, {0xff, 0xf0, 1, 2, 3, 4, 5}}) ==
               seedkey::Bytes{0x26, 0xaa, 0x63}),
              "PTDIAG generic security custom key mismatch");
        check((seedkey::key({"ptdiag-osc", 1, {0x12, 0x34}, {}}) == seedkey::Bytes{0xb7, 0xbc}),
              "PTDIAG OSC key mismatch");
        check(rejects([] { seedkey::key({"ptdiag-osc", 2, {0x12, 0x34}, {}}); }),
              "Unsupported OSC security type accepted");
        check((seedkey::key({"ptdiag-pats-v408", 0, {0x12, 0x34, 0x56}, {}}) ==
               seedkey::Bytes{0x31, 0x3d, 0xcf}),
              "PTDIAG PATS V408 key mismatch");
        check((seedkey::key({"ptdiag-pats-v363-diesel", 0, {0x12, 0x34, 0x56}, {}}) ==
               seedkey::Bytes{0x97, 0x3a, 0x02}),
              "PTDIAG PATS V363 diesel key mismatch");
        check((seedkey::key({"ptdiag-pats-v363n", 0, {0x12, 0x34, 0x56}, {}}) ==
               seedkey::Bytes{0x99, 0x32, 0xb1}),
              "PTDIAG PATS V363N key mismatch");
        check((seedkey::key({"ptdiag-pats-b299n", 0x1d, {0x12, 0x34, 0x56}, {}}) ==
               seedkey::Bytes{0x2a, 0x79, 0x17}),
              "PTDIAG PATS B299N primary key mismatch");
        check((seedkey::key({"ptdiag-pats-b299n", 0x1e, {0x12, 0x34, 0x56}, {}}) ==
               seedkey::Bytes{0xa1, 0x64, 0x69}),
              "PTDIAG PATS B299N secondary key mismatch");
        check(rejects([] { seedkey::key({"ptdiag-pats-b299n", 0, {0x12, 0x34, 0x56}, {}}); }),
              "PTDIAG PATS B299N accepted missing level");
        check((seedkey::key({"ptdiag-pats-c3x-c5x", 0, {0x12, 0x34, 0x56}, {1, 2, 3, 4, 5}}) ==
               seedkey::Bytes{0x26, 0xaa, 0x63}),
              "PTDIAG PATS C3X/C5X key mismatch");
        check((seedkey::key({"ptdiag-pats-b2e", 0, {0x12, 0x34, 0x56}, {1, 2, 3, 4, 5}}) ==
               seedkey::Bytes{0x26, 0xaa, 0x63}),
              "PTDIAG PATS B2E key mismatch");
        check(rejects([] { seedkey::key({"ptdiag-pats-b2e", 0, {0x12, 0x34, 0x56}, {1, 2, 3, 4}}); }),
              "PTDIAG PATS custom fixed bytes length accepted");
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
