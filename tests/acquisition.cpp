#include "../native/acquisition.h"
#include "../native/image.h"
#include "../native/definitions.h"
#include <QCoreApplication>
#include <QTemporaryDir>
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
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QTemporaryDir dir;
    try {
        auto ini = acq::Init::load(app.arguments().at(1) + "/PTDIAG.INI");
        check(ini.autoConfig && ini.autoPid && ini.interval == 100, "Legacy init options");
        auto cfg = acq::Config::parse(acq::readFile(ini.configPath, 4096));
        check(cfg.channels.size() == 2 && cfg.channels[0].command == "did:1165" &&
                  cfg.channels[0].translate(QByteArray::fromHex("0c80")) == 800 &&
                  cfg.channels[1].translate(QByteArray::fromHex("e0")) == 14,
              "2293 help example");
        auto list = acq::Config::parse(acq::readFile(ini.pidPath, 4096));
        check(list.channels.at(2).translate(QByteArray::fromHex("78")) == 80, "OBD-II offset");
        auto packet = acq::Channel::parse("temp 1100 k 2 -1 s 0.5 5 2");
        check(packet.translate(QByteArray::fromHex("33fffe")) == 3, "Signed packet scaling");
        check(rejects([&] { packet.translate(QByteArray::fromHex("33ff")); }), "Truncated packet accepted");
        check(acq::Channel::parse("bit 1122 p 1 0 7").translate(QByteArray::fromHex("80")) == 1, "Bit decode");
        check(acq::Channel::parse("float 1100 p 4 0 f").translate(QByteArray::fromHex("bf800000")) == -1,
              "Float decode");
        check(rejects([] { acq::Config::parse(": a 1100 p 2 0 u\n: A 1101 p 2 0 u\n"); }),
              "Duplicate accepted");
        check(rejects([] { acq::Config::parse(": a 1100 p 2 0 u bad\n"); }), "Invalid scalar accepted");
        check(acq::Config::parse(list.serialize()).serialize() == list.serialize(), "Config round trip");
        for (auto build : {"1980", "2200", "2201", "2262", "2293"}) {
            auto golden =
                acq::readFile(app.arguments().at(1) + "/../golden/PTDIAG-" + build + "-RPM-VBAT.cfg", 8192);
            auto actual = acq::Config::parse(golden);
            check(actual.serialize() == golden && actual.channels.size() == 2 &&
                      actual.channels[0].translate(QByteArray::fromHex("0c80")) == 800 &&
                      actual.channels[1].translate(QByteArray::fromHex("e0")) == 14,
                  "Original GUI output");
        }
        auto formula = acq::Channel::parse("t 05 1 1 0 u 1 -40 1 (x*9/5)+32 degF");
        check(formula.translate(QByteArray::fromHex("78")) == 176 && formula.units == "degF",
              "Formula after scalar and offset");
        check(rejects([] { acq::Channel::parse("t 05 1 1 0 u 1 0 1 f(x)"); }), "Unknown formula accepted");
        QByteArray original = "Operator comment\r\n<custom=preserve>\r\n: rpm     1165  p  2  2  u\r\n";
        auto retained = acq::Config::parse(original);
        check(retained.serialize() == original, "Unchanged configuration bytes");
        retained.channels[0] = acq::Channel::parse("vbat 1172 p 1 4 u");
        check(retained.serialize().contains("<custom=preserve>\r\n: vbat"), "Metadata lost on edit");
        acq::Capture cap{{"rpm"}, {}};
        for (int i = 0; i < 4; ++i)
            cap.append({i, {i + 0.25}}, 2);
        auto file = dir.filePath("data.csv");
        cap.save(file);
        auto read = acq::Capture::load(file);
        check(read.rows.size() == 2 && read.rows.front().ms == 2 && read.rows.back().values.front() == 3.25,
              "FIFO round trip");
        acq::writeFile(file, "seconds,rpm\n1.25,800\n1.2,810\n");
        check(rejects([&] { acq::Capture::load(file); }), "Bad capture timestamps");
        auto memory = dir.filePath("memory.bin");
        acq::writeFile(memory, QByteArray::fromHex("aa80fe"));
        auto dmrs = acq::Config::parse("<byte_order=intel>\n: sensor FFFF0001 d 2 0 s\n");
        check(acq::Capture::snapshot(dmrs, memory, 0xffff0000).rows.front().values.front() == -384,
              "Little-endian DMR snapshot");
        check(rejects([&] { acq::Capture::snapshot(dmrs, memory, 0xffff0002); }), "DMR hole was read");
        acq::writeFile(file, ": 0, \"OLD\", \"NEW\", \"c:\\cal files\\image.hex\"\n");
        check(lookupImage(file, "old", false).primaryPath == "c:\\cal files\\image.hex", "PCM lookup");
        check(rejects([&] { lookupImage(file, "ol", false); }) &&
                  rejects([&] { lookupImage(file, "OLD", true); }),
              "Partial/wrong family lookup");
        acq::writeFile(file, ": 0 \"OLD\" \"NEW\" \"a.vbf\" \"b.vbf\"\n");
        check(lookupImage(file, "old", true).sblPath == "b.vbf", "Generic lookup");
        Image image{"binary", {{0x2000, QByteArray(257, 'A')}}, {}, {}};
        check(image.page(0x2000, 1).startsWith("00002100  41"), "Sparse image last page");
        check(rejects([&] { DtcDefinitions::read(file); }), "Unrecognized DTC dictionary accepted");
        if (argc == 3) {
            auto db = DtcDefinitions::read(app.arguments()[2]);
            check(db.kwp.size() == 10170 && db.uds.size() == 13309 &&
                      db.uds.value("P0133") == "O2 Sensor Circuit Slow Response (Bank 1 Sensor 1)" &&
                      db.kwp.value("P0133") != db.uds.value("P0133"),
                  "Original DTC dictionaries");
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
