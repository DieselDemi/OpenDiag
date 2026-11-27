#include "image.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <iostream>
#include <stdexcept>

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
                "INPUT START_HEX ROWS COLS CELL ORDER csv|fixed [BIN_BASE_HEX]\n";
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
