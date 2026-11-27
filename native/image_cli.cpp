#include "image.h"
#include "virtual_ecu.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonDocument>
#include <iostream>
#include <stdexcept>

static void ecuCommand(const QStringList &args) {
    constexpr const char *usage =
        "opendiag-image ecu ACTION WORKSPACE [arguments]\nActions: create, prepare, status, step, run, "
        "restore, voltage, export, backup, read-block, save-config, kam-layout, save-kam, recovery-kit";
    if (args == QStringList{"--help"}) {
        std::cout << usage << '\n';
        return;
    }
    if (args.size() < 2)
        throw std::runtime_error(usage);
    const auto action = args[0], directory = args[1];
    if (action == "create" && args.size() == 3)
        VirtualEcu::create(directory, args[2]);
    VirtualEcu ecu(directory);
    if (action == "prepare" && args.size() == 3)
        ecu.prepare(args[2]);
    else if (action == "step" && args.size() == 2)
        ecu.step();
    else if (action == "run" && args.size() == 2) {
        if (ecu.phase() == "idle")
            throw std::runtime_error("Prepare a job first");
        while (!ecu.finished()) {
            auto previous = ecu.phase();
            ecu.step();
            if (ecu.phase() != previous)
                std::cerr << ecu.phase().toStdString() << std::endl;
        }
    } else if (action == "restore" && args.size() == 2)
        ecu.restore();
    else if (action == "voltage" && args.size() == 3) {
        bool ok;
        auto mv = args[2].toInt(&ok);
        if (!ok)
            throw std::runtime_error("Expected virtual supply in mV");
        ecu.setVoltage(mv);
    } else if ((action == "export" || action == "backup") && args.size() == 3)
        ecu.exportBank(args[2], action == "backup");
    else if (action == "read-block" && args.size() == 5) {
        bool a, b;
        auto addr = args[2].toUInt(&a, 16), length = args[3].toUInt(&b, 16);
        if (!a || !b)
            throw std::runtime_error("Expected hexadecimal address and length");
        ecu.exportRange(args[4], addr, length);
    } else if (action == "save-config" && (args.size() == 3 || args.size() == 4))
        ecu.exportConfiguration(args[2], args.value(3));
    else if (action == "recovery-kit" && args.size() == 3)
        ecu.exportRecoveryKit(args[2]);
    else if (action == "kam-layout" && args.size() == 3)
        ecu.exportKamLayout(args[2]);
    else if (action == "save-kam" && args.size() == 4)
        ecu.exportKamData(args[2], args[3]);
    else if (!(action == "create" && args.size() == 3) && !(action == "status" && args.size() == 2))
        throw std::runtime_error(usage);
    std::cout << QJsonDocument(ecu.status()).toJson(QJsonDocument::Compact).constData() << '\n';
}

static QString vidFields(const QString &path, int layout) {
    if (QFileInfo(path).suffix().compare("bin", Qt::CaseInsensitive) != 0 || layout < 0 || layout > 2)
        throw std::runtime_error("Expected a raw VID block .bin and layout 0, 1 or 2");
    const auto image = readImage(path);
    const auto &data = image.segments.begin()->second;
    static const struct {
        const char *field;
        unsigned offset[3];
    } fields[] = {
#define VID(field, a, b, c) {#field, {a, b, c}},
#include "vid_fields.inc"
#undef VID
    };
    QString result;
    for (auto &f : fields) {
        auto offset = f.offset[layout];
        if (offset >= data.size())
            throw std::runtime_error("VID block is too short for this layout");
        result += QString("%1 @0x%2: 0x%3\n")
                      .arg(f.field)
                      .arg(offset, 2, 16, QChar('0'))
                      .arg(quint8(data[offset]), 2, 16, QChar('0'));
    }
    return result;
}

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    auto args = app.arguments();
    if (args.value(1) == "mx32-config") {
        if (args.size() == 3 && args[2] == "--list") {
            std::cout << mxConfigProfiles().join('\n').toStdString() << '\n';
            return 0;
        }
        if (args.size() != 5 && args.size() != 6) {
            std::cerr << "Usage: opendiag-image mx32-config PROFILE INPUT.h32 NEW.vbf [PART]\n";
            return 2;
        }
        try {
            convertMxConfig(args[2], args[3], args[4], args.value(5, "NA"));
            return 0;
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
            return 1;
        }
    }
    if (args.value(1) == "ecu") {
        try {
            ecuCommand(args.mid(2));
            return 0;
        } catch (const std::exception &e) {
            std::cerr << e.what() << '\n';
            return 1;
        }
    }
    bool convert = args.size() >= 2 && args[1] == "to-hex";
    bool vid = args.size() == 4 && args[1] == "vid";
    bool lookup = args.size() == 5 && args[1] == "lookup" && (args[2] == "pcm" || args[2] == "generic");
    bool compare = args.size() == 5 && args[1] == "compare";
    bool table = (args.size() == 9 || args.size() == 10) && args[1] == "table";
    if (!vid && !lookup && !compare && !table && !(args.size() == 3 && args[1] == "inspect") &&
        !(convert && (args.size() == 4 || args.size() == 5))) {
        std::cerr
            << "Usage: opendiag-image inspect INPUT | to-hex INPUT NEW.hex [BINARY_BASE_HEX] | vid "
               "BLOCK.bin LAYOUT | lookup pcm|generic TABLE ID | compare BACKUP.bin BASE_HEX CANDIDATE | table "
               "INPUT START_HEX ROWS COLS CELL ORDER csv|fixed [BIN_BASE_HEX] | ecu ACTION WORKSPACE [ARG]\n";
        return 2;
    }
    try {
        if (table) {
            bool a, b = true;
            auto start = args[3].toUInt(&a, 16), base = args.size() == 10 ? args[9].toUInt(&b, 16) : 0;
            if (!a || !b || (args.size() == 10 && !args[2].endsWith(".bin", Qt::CaseInsensitive)))
                throw std::runtime_error("Invalid table mapping");
            std::cout << readImage(args[2], base)
                             .table(start, args[4].toInt(), args[5].toInt(), args[6], args[7], args[8])
                             .toStdString();
            return 0;
        }
        if (compare) {
            bool ok;
            auto base = args[3].toUInt(&ok, 16);
            if (!ok)
                throw std::runtime_error("Invalid backup base");
            std::cout << compareCalibration(args[2], base, args[4]).toStdString() << '\n';
            return 0;
        }
        if (lookup) {
            auto m = lookupImage(args[3], args[4], args[2] == "generic");
            std::cout << m.primaryId.toStdString() << '\n' << m.primaryPath.toStdString() << '\n';
            if (!m.sblPath.isEmpty())
                std::cout << m.sblPath.toStdString() << '\n';
            return 0;
        }
        bool bin = QFileInfo(args[2]).suffix().compare("bin", Qt::CaseInsensitive) == 0;
        bool ok = true;
        if (vid) {
            int layout = args[3].toInt(&ok);
            std::cout << vidFields(args[2], ok ? layout : -1).toStdString();
            return 0;
        }
        quint32 base = args.size() == 5 ? args[4].toUInt(&ok, 16) : 0;
        if (!ok || (convert && bin != (args.size() == 5)))
            throw std::runtime_error("Specify a hexadecimal base only for binary conversion");
        auto image = readImage(args[2], base);
        if (convert)
            image.saveHex(args[3]);
        std::cout << image.summary().toStdString() << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
