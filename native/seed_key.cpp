#include "seed_key.h"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace seedkey {
namespace {
const std::vector<Profile> knownProfiles = {
    {"ptdiag-legacy-2byte", 2, 4, 2, true},
    {"ptdiag-legacy-83k", 2, 4, 2, true},
    {"ptdiag-legacy-ezref", 2, 4, 2, true},
    {"ptdiag-legacy-ezref-83k", 2, 4, 2, true},
    {"ptdiag-iso-2byte", 2, 4, 2, true},
    {"ptdiag-osc", 2, 0, 2, true},
    {"ptdiag-mazda-dmr", 0, 0, 0, false},
    {"ptdiag-ggds004", 0, 0, 0, false},
    {"ptdiag-generic-security-bytes", 3, 0, 3, true},
    {"ptdiag-diesel-swdl6", 0, 0, 0, false},
    {"ptdiag-pats-b299n", 3, 0, 3, true},
    {"ptdiag-pats-v408", 3, 0, 3, true},
    {"ptdiag-pats-v363-diesel", 3, 0, 3, true},
    {"ptdiag-pats-c3x-c5x", 3, 0, 3, true},
    {"ptdiag-pats-b2e", 3, 0, 3, true},
    {"ptdiag-pats-v363n", 3, 0, 3, true},
    {"ptdiag-dll-keydown", 0, 0, 0, false},
};

const Profile &findProfile(const std::string &name) {
    auto it = std::find_if(knownProfiles.begin(), knownProfiles.end(),
                           [&](const Profile &profile) { return profile.name == name; });
    if (it == knownProfiles.end())
        throw std::runtime_error("Unknown seed-key profile: " + name);
    return *it;
}

std::uint16_t word(const Bytes &bytes, std::size_t offset) {
    return std::uint16_t((std::uint16_t(bytes[offset]) << 8) | bytes[offset + 1]);
}

std::uint16_t low16(unsigned value) { return std::uint16_t(value & 0xffff); }

struct LegacyConstants {
    std::uint16_t poly0Offset;
    std::uint16_t poly2Offset;
    std::uint16_t poly3Offset;
};

std::uint16_t calculateLegacyPolynomial(std::uint16_t seed, std::uint16_t seedFactor,
                                        std::uint16_t polyIndicator, LegacyConstants c) {
    const std::uint16_t nib = seedFactor & 1 ? (polyIndicator >> 4) & 0xf : polyIndicator & 0xf;
    const std::uint16_t poly[2] = {std::uint16_t(nib & 3), std::uint16_t((nib & 0xc) >> 2)};
    const std::uint16_t seedByte[2] = {std::uint16_t(seed & 0xff), std::uint16_t(seed >> 8)};
    std::uint16_t found[2]{};
    for (int i = 0; i < 2; ++i) {
        const std::uint16_t s = seedByte[i];
        std::uint16_t total = 0;
        switch (poly[i]) {
        case 0: {
            std::uint16_t temp = s;
            for (std::uint16_t n = 1; n < seedFactor; ++n)
                temp = std::uint16_t(temp * s);
            total = low16(((s + c.poly0Offset) * (s + c.poly0Offset)) + ((temp - 1) * seedFactor));
            break;
        }
        case 1:
            total = low16((seedFactor * 5 * (s + 5) * (s + 5)) + ((0xfffb - s) * (s + 5) * (s + 5)));
            break;
        case 2:
            total = low16(((s * s * s) - (seedFactor * seedFactor) + c.poly2Offset) *
                          ((s * s * s) - (seedFactor * seedFactor) + c.poly2Offset));
            break;
        case 3:
            total = low16((s * s) + seedFactor +
                          ((s + c.poly3Offset) * (s + c.poly3Offset) * (s + c.poly3Offset)) - 0x28);
            break;
        }
        found[i] = std::uint16_t(total & 0xff);
    }
    return std::uint16_t(found[0] | (found[1] << 8));
}

std::uint16_t calculateEzref(std::uint16_t seed, std::uint16_t seedFactor,
                             std::uint16_t polyIndicator) {
    const std::uint16_t nib = seedFactor & 1 ? (polyIndicator >> 4) & 0xf : polyIndicator & 0xf;
    const std::uint16_t poly[2] = {std::uint16_t(nib & 3), std::uint16_t((nib & 0xc) >> 2)};
    const std::uint16_t seedByte[2] = {std::uint16_t(seed & 0xff), std::uint16_t(seed >> 8)};
    std::uint16_t found[2]{};
    for (int i = 0; i < 2; ++i) {
        const std::uint16_t s = seedByte[i];
        std::uint16_t total = 0;
        switch (poly[i]) {
        case 0:
            total = low16(((s + 0x31) * (s + 0x31) * 4) - seedFactor);
            break;
        case 1:
            total = low16((s * seedFactor) * (s * seedFactor) * (s * seedFactor) - 1);
            break;
        case 2:
            total = low16(s + seedFactor + (seedFactor * seedFactor));
            break;
        case 3:
            total = low16((s - seedFactor + 3) * (s - seedFactor + 3));
            break;
        }
        found[i] = std::uint16_t(total & 0xff);
    }
    return std::uint16_t(found[0] | (found[1] << 8));
}

std::uint16_t calculateEzref83K(std::uint16_t seed, std::uint16_t seedFactor,
                                std::uint16_t polyIndicator) {
    const std::uint16_t nib = seedFactor & 1 ? (polyIndicator >> 4) & 0xf : polyIndicator & 0xf;
    const std::uint16_t poly[2] = {std::uint16_t(nib & 3), std::uint16_t((nib & 0xc) >> 2)};
    const std::uint16_t seedByte[2] = {std::uint16_t(seed & 0xff), std::uint16_t(seed >> 8)};
    std::uint16_t found[2]{};
    for (int i = 0; i < 2; ++i) {
        const std::uint16_t x = seedByte[i];
        const std::uint16_t y = seedFactor;
        std::uint16_t total = 0;
        switch (poly[i]) {
        case 0:
            total = low16((2 * x + y + 0x62) * (2 * x + y + 0x62) - y);
            break;
        case 1:
            total = low16(((x - 0x14) * y) * ((x - 0x14) * y) * ((x - 0x14) * y) - 1);
            break;
        case 2:
            total = low16(15 * (x + y) + (y * y));
            break;
        case 3:
            total = low16((2 * x - y + 3) * (2 * x - y + 3));
            break;
        }
        found[i] = std::uint16_t(total & 0xff);
    }
    return std::uint16_t(found[0] | (found[1] << 8));
}

std::uint16_t calculateProfile(const std::string &name, std::uint16_t seed,
                               std::uint16_t seedFactor, std::uint16_t polyIndicator) {
    if (name == "ptdiag-legacy-2byte")
        // Lifted from PTDIAG CalculateSecurityKey(usSeed, usSeedFactor, usPolyIndicator).
        return calculateLegacyPolynomial(seed, seedFactor, polyIndicator, {100, 10, 0x22});
    if (name == "ptdiag-legacy-83k")
        return calculateLegacyPolynomial(seed, seedFactor, polyIndicator, {99, 11, 0x21});
    if (name == "ptdiag-legacy-ezref" || name == "ptdiag-iso-2byte")
        return calculateEzref(seed, seedFactor, polyIndicator);
    if (name == "ptdiag-legacy-ezref-83k")
        return calculateEzref83K(seed, seedFactor, polyIndicator);
    throw std::runtime_error("Seed-key profile has no dispatcher: " + name);
}

std::array<std::uint8_t, 5> genericFixedBytes(std::uint16_t type, const Bytes &securityBytes) {
    static const std::array<std::array<std::uint8_t, 5>, 49> table = {{
        {{0x08, 0x30, 0x61, 0xa4, 0xc5}}, {{0x44, 0x49, 0x4f, 0x44, 0x45}},
        {{0x4d, 0x61, 0x7a, 0x64, 0x41}}, {{0x41, 0x52, 0x49, 0x41, 0x4e}},
        {{0x4d, 0x61, 0x7a, 0x64, 0x41}}, {{0x7b, 0x03, 0xc9, 0x22, 0xf1}},
        {{0x4d, 0x61, 0x7a, 0x64, 0x41}}, {{0x4a, 0x65, 0x73, 0x75, 0x73}},
        {{0x41, 0x57, 0x54, 0x43, 0x55}}, {{0x3a, 0x62, 0x93, 0xd6, 0xf7}},
        {{0x83, 0x55, 0x07, 0x51, 0x9a}}, {{0x83, 0x55, 0x07, 0x51, 0x9a}},
        {{0x3f, 0x43, 0xef, 0x74, 0xbe}}, {{0x41, 0x49, 0x53, 0x49, 0x4e}},
        {{0x08, 0x30, 0x61, 0x55, 0xaa}}, {{0xac, 0xf7, 0x29, 0x92, 0xe1}},
        {{0xac, 0x03, 0x14, 0x89, 0xe3}}, {{0x49, 0x76, 0x66, 0x65, 0x52}},
        {{0xa1, 0x14, 0x39, 0xbc, 0x57}}, {{0x28, 0x05, 0xca, 0x87, 0x9d}},
        {{0x73, 0x49, 0x53, 0x63, 0x13}}, {{0x48, 0x15, 0x16, 0x23, 0x42}},
        {{0x5f, 0x9c, 0x99, 0xa3, 0x50}}, {{0x41, 0xaa, 0x42, 0xbb, 0x43}},
        {{0xa7, 0xc2, 0xe9, 0x19, 0x92}}, {{0xa3, 0xc1, 0xe4, 0x19, 0x89}},
        {{0xa3, 0xb2, 0xc0, 0x14, 0x92}}, {{0x24, 0x31, 0xde, 0xf9, 0x46}},
        {{0x9a, 0x78, 0x56, 0x34, 0x12}}, {{0x7e, 0x50, 0x43, 0x4d, 0x3f}},
        {{0x24, 0x65, 0x78, 0x29, 0x0d}}, {{0x05, 0x1a, 0xd1, 0xc6, 0x4b}},
        {{0xa7, 0xc2, 0xe9, 0x2c, 0x7a}}, {{0x37, 0xe1, 0xa2, 0x15, 0xc8}},
        {{0x33, 0x37, 0x31, 0x39, 0x41}}, {{0x23, 0x82, 0x54, 0x10, 0x01}},
        {{0x09, 0x13, 0x26, 0x28, 0x44}}, {{0x44, 0x35, 0x72, 0x65, 0x6b}},
        {{0x4a, 0x61, 0x4d, 0x65, 0x73}}, {{0x42, 0x72, 0x6f, 0x57, 0x6e}},
        {{0x17, 0x45, 0x12, 0xa8, 0xaa}}, {{0xaa, 0xbb, 0xcc, 0xdd, 0xee}},
        {{0xa7, 0xc2, 0xe9, 0x19, 0x92}}, {{0xa3, 0xc1, 0xe4, 0x19, 0x89}},
        {{0x35, 0xb4, 0x7a, 0x6d, 0x4f}}, {{0xff, 0xff, 0xff, 0xff, 0xff}},
        {{0x00, 0x00, 0x00, 0x00, 0x00}}, {{0x61, 0xb6, 0x0c, 0x6a, 0xd3}},
        {{0x55, 0x6f, 0x4d, 0x32, 0x37}},
    }};
    if (type < table.size())
        return table[type];
    if (type == 0xfff0 && securityBytes.size() == 7)
        return {securityBytes[2], securityBytes[3], securityBytes[4], securityBytes[5], securityBytes[6]};
    throw std::runtime_error("Unsupported PTDIAG generic security type");
}

std::uint8_t readBit(std::uint32_t value, unsigned bit) { return bit < 32 ? std::uint8_t((value >> bit) & 1) : 0; }
std::uint32_t adjustBit(std::uint32_t value, unsigned bit, std::uint8_t set) {
    if (bit >= 32)
        return 0;
    const std::uint32_t mask = 1u << bit;
    return set ? (value | mask) : (value & ~mask);
}

Bytes calculateGenericSecurityBytes(const Input &input) {
    if (input.securityBytes.size() != 2 && input.securityBytes.size() != 7)
        throw std::runtime_error("Profile ptdiag-generic-security-bytes expects 2 security type bytes, plus 5 fixed bytes for type FFF0");
    const std::uint16_t type = word(input.securityBytes, 0);
    const auto fixed = genericFixedBytes(type, input.securityBytes);
    const std::uint32_t challengeHigh = (std::uint32_t(fixed[0]) << 24) |
                                        (std::uint32_t(input.seed[2]) << 16) |
                                        (std::uint32_t(input.seed[1]) << 8) | input.seed[0];
    const std::uint32_t challengeLow = (std::uint32_t(fixed[4]) << 24) |
                                       (std::uint32_t(fixed[3]) << 16) |
                                       (std::uint32_t(fixed[2]) << 8) | fixed[1];
    std::uint32_t shift = 0x00c541a9;
    for (unsigned counter = 0; counter <= 0x3f; ++counter) {
        const auto source = counter <= 0x1f ? challengeHigh : challengeLow;
        const auto position = counter <= 0x1f ? counter : counter - 0x20;
        const std::uint8_t challengeBit = readBit(source, position);
        const std::uint8_t xorBit = readBit(shift, 0);
        shift >>= 1;
        const std::uint8_t newBit = xorBit ^ challengeBit;
        shift = adjustBit(shift, 0x17, newBit);
        for (unsigned bit : {0x14u, 0x0fu, 0x0cu, 0x05u, 0x03u})
            shift = adjustBit(shift, bit, readBit(shift, bit) ^ newBit);
    }
    return {std::uint8_t(shift >> 4),
            std::uint8_t(((shift >> 20) & 0x0f) | ((shift >> 8) & 0xf0)),
            std::uint8_t(((shift >> 16) & 0x0f) | ((shift << 4) & 0xf0))};
}

Bytes calculatePats(const Input &input) {
    Bytes securityBytes;
    if (input.profile == "ptdiag-pats-v408")
        securityBytes = {0x00, 0x2c};
    else if (input.profile == "ptdiag-pats-v363-diesel")
        securityBytes = {0x00, 0x27};
    else if (input.profile == "ptdiag-pats-v363n")
        securityBytes = {0x00, 0x28};
    else if (input.profile == "ptdiag-pats-c3x-c5x" || input.profile == "ptdiag-pats-b2e") {
        if (input.securityBytes.size() != 5)
            throw std::runtime_error("Profile " + input.profile + " expects five PTDIAG FFF0 fixed bytes");
        securityBytes = {0xff, 0xf0, input.securityBytes[0], input.securityBytes[1], input.securityBytes[2],
                         input.securityBytes[3], input.securityBytes[4]};
    } else if (input.profile == "ptdiag-pats-b299n") {
        if (input.level != 0x1d && input.level != 0x1e && input.level != 0x1f && input.level != 0x24)
            throw std::runtime_error("Profile ptdiag-pats-b299n expects level 1D, 1E, 1F, or 24");
        securityBytes = {0x00, input.level};
    } else
        throw std::runtime_error("Seed-key profile has no PATS dispatcher: " + input.profile);
    return calculateGenericSecurityBytes({input.profile, input.level, input.seed, securityBytes});
}

Bytes calculateOsc(const Input &input) {
    if (input.level && input.level != 1)
        throw std::runtime_error("Profile ptdiag-osc only supports security type 1");
    const auto seed = word(input.seed, 0);
    const auto value = 2 * int(seed) * int(seed) - 253 * int(seed);
    const auto key = std::uint16_t(value < 0 ? -value : value);
    return {std::uint8_t(key >> 8), std::uint8_t(key)};
}
} // namespace

const std::vector<Profile> &profiles() { return knownProfiles; }

Bytes key(const Input &input) {
    const auto &profile = findProfile(input.profile);
    if (profile.seedLength && input.seed.size() != profile.seedLength)
        throw std::runtime_error("Profile " + profile.name + " expects a " +
                                 std::to_string(profile.seedLength) + "-byte seed");
    if (profile.securityBytesLength && input.securityBytes.size() != profile.securityBytesLength)
        throw std::runtime_error("Profile " + profile.name + " expects " +
                                 std::to_string(profile.securityBytesLength) + " security bytes");
    if (!profile.builtIn)
        throw std::runtime_error("Seed-key profile has no verified built-in implementation: " + profile.name);
    if (profile.name.rfind("ptdiag-pats-", 0) == 0)
        return calculatePats(input);
    if (profile.name == "ptdiag-osc")
        return calculateOsc(input);
    if (profile.name == "ptdiag-generic-security-bytes")
        return calculateGenericSecurityBytes(input);
    if (profile.securityBytesLength == 4 && profile.seedLength == 2) {
        const auto k = calculateProfile(profile.name, word(input.seed, 0), word(input.securityBytes, 0),
                                        word(input.securityBytes, 2));
        return {std::uint8_t(k >> 8), std::uint8_t(k)};
    }
    throw std::runtime_error("Seed-key profile has no dispatcher: " + profile.name);
}
} // namespace seedkey
