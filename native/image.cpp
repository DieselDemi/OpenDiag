#include "image.h"
#include "files.h"
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QtEndian>
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <array>

namespace {
[[noreturn]] void fail(const QString &message) { throw std::runtime_error(message.toStdString()); }

void insert(Image &img, quint32 start, QByteArray bytes) {
    if (bytes.isEmpty())
        return;
    quint64 end = quint64(start) + bytes.size();
    auto next = img.segments.lower_bound(start);
    if (end > (quint64(1) << 32) || (next != img.segments.end() && end > next->first))
        fail("Overlapping or out-of-range segment");
    if (next != img.segments.begin()) {
        auto prev = std::prev(next);
        auto prevEnd = quint64(prev->first) + prev->second.size();
        if (start < prevEnd)
            fail("Overlapping segment");
        if (start == prevEnd) {
            prev->second += bytes;
            return;
        }
    }
    if (img.segments.size() >= 65536)
        fail("Too many disjoint segments");
    img.segments.emplace_hint(next, start, std::move(bytes));
}

Image intelHex(const QByteArray &file) {
    Image img{"Intel HEX", {}, {}};
    bool eof = false;
    bool recordsStarted = false;
    QByteArray preamble;
    quint32 base = 0;
    qsizetype pos = 0;
    for (int n = 1; pos < file.size(); ++n) {
        auto end = file.indexOf('\n', pos);
        if (end < 0)
            end = file.size();
        auto line = QByteArrayView(file).sliced(pos, end - pos).trimmed();
        pos = end + 1;
        if (line.empty())
            continue;
        if (!recordsStarted && line.front() != ':') {
            if (line.size() > 4096 || preamble.size() + line.size() > 1024 * 1024 ||
                !std::all_of(line.begin(), line.end(),
                             [](unsigned char c) { return c == '\t' || (c >= 32 && c < 127); }))
                fail("Invalid Intel HEX text header");
            preamble += line.toByteArray();
            preamble += '\n';
            continue;
        }
        if (!recordsStarted && !preamble.isEmpty()) {
            auto h = QString::fromLatin1(preamble);
            auto field = [&](const QString &pattern) {
                return QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption |
                                                       QRegularExpression::MultilineOption)
                    .match(h)
                    .hasMatch();
            };
            bool kam = field(R"(^file data\s*>\s*Specified KAM Regions?\s*$)");
            bool mx =
                field(R"(^file name\s*>[^\n]+$)") && field(R"(^last data address\s*>\s*[0-9a-f]{1,8}\s*$)");
            if (!kam && !mx)
                fail("Unrecognized Intel HEX text header");
            img.header = h;
            img.format = kam ? "PTDIAG KAM HEX" : "Ford annotated Intel HEX";
        }
        recordsStarted = true;
        auto invalid = [n] { fail(QString("Invalid Intel HEX record at line %1").arg(n)); };
        if (eof || line.size() < 11 || line.size() > 521 || line[0] != ':' || line.size() % 2 != 1)
            invalid();
        for (char c : line.sliced(1))
            if (!(c >= '0' && c <= '9') && !(c >= 'A' && c <= 'F') && !(c >= 'a' && c <= 'f'))
                invalid();
        auto rec = QByteArray::fromHex(line.sliced(1).toByteArray());
        quint8 checksum = 0;
        for (auto c : rec)
            checksum += quint8(c);
        int len = quint8(rec[0]), type = quint8(rec[3]);
        auto addr = qFromBigEndian<quint16>(rec.constData() + 1);
        if (checksum || rec.size() != len + 5 || (type != 0 && addr))
            invalid();
        switch (type) {
        case 0:
            if (quint64(base) + addr + len > (quint64(1) << 32) || unsigned(addr) + len > 65536u)
                invalid();
            insert(img, base + addr, rec.mid(4, len));
            break;
        case 1:
            if (len)
                invalid();
            eof = true;
            break;
        case 2:
        case 4:
            if (len != 2)
                invalid();
            base = quint32(qFromBigEndian<quint16>(rec.constData() + 4)) << (type == 2 ? 4 : 16);
            break;
        case 3:
        case 5:
            if (len != 4 || !img.entryRecord.isEmpty())
                invalid();
            img.entryRecord = line.toByteArray();
            break;
        default:
            invalid();
        }
    }
    if (!eof || img.segments.empty())
        fail("Intel HEX is missing data or EOF");
    return img;
}

quint16 crc16(QByteArrayView data) {
    quint16 crc = 0xffff;
    for (auto c : data) {
        crc ^= quint16(quint8(c)) << 8;
        for (int i = 0; i < 8; ++i)
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}

quint32 crc32(QByteArrayView data) {
    static const auto table = [] {
        std::array<quint32, 256> t{};
        for (unsigned i = 0; i < t.size(); ++i) {
            quint32 c = i;
            for (int n = 0; n < 8; ++n)
                c = (c >> 1) ^ ((c & 1) ? 0xedb88320u : 0u);
            t[i] = c;
        }
        return t;
    }();
    quint32 crc = 0xffffffffu;
    for (char c : data)
        crc = (crc >> 8) ^ table[(crc ^ quint8(c)) & 0xff];
    return ~crc;
}

Image vbf(const QByteArray &file) {
    // Mask quoted text and comments before interpreting structural braces or header fields.
    QByteArray header;
    int depth = 0;
    bool quoted = false, comment = false, blockComment = false;
    qsizetype pos = 0;
    for (; pos < file.size() && pos < 1024 * 1024; ++pos) {
        char c = file[pos], next = pos + 1 < file.size() ? file[pos + 1] : '\0';
        if (comment) {
            if (c == '\n')
                comment = false;
        } else if (blockComment) {
            if (c == '*' && next == '/') {
                blockComment = false;
                ++pos;
            }
        } else if (quoted) {
            if (c == '\\')
                ++pos;
            else if (c == '"')
                quoted = false;
        } else if (c == '/' && next == '/') {
            comment = true;
            ++pos;
        } else if (c == '/' && next == '*') {
            blockComment = true;
            ++pos;
        } else if (c == '"')
            quoted = true;
        else {
            header += c;
            if (c == '{')
                ++depth;
            if (c == '}' && --depth == 0) {
                ++pos;
                break;
            }
            continue;
        }
        header += ' ';
    }
    const auto text = QString::fromLatin1(header);
    static const QRegularExpression version(R"(^\s*vbf_version\s*=\s*(2\.\d+)\s*;\s*header\s*\{)",
                                            QRegularExpression::CaseInsensitiveOption);
    auto v = version.match(text);
    if (!v.hasMatch() || depth || !header.endsWith('}'))
        fail("Unsupported or incomplete VBF 2.x header");
    static const QRegularExpression field(R"(\bdata_format_identifier\b)",
                                          QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression plain(R"(\bdata_format_identifier\s*=\s*(?:0x0+|0+)\s*;)",
                                          QRegularExpression::CaseInsensitiveOption);
    if (text.count(field) != text.count(plain))
        fail("Only uncompressed VBF data is supported");
    static const QRegularExpression checksumName(R"(\bfile_checksum\b)",
                                                 QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression checksumValue(R"(\bfile_checksum\s*=\s*0x([0-9a-f]{8})\s*;)",
                                                  QRegularExpression::CaseInsensitiveOption);
    auto checksum = checksumValue.match(text);
    if (text.count(checksumName) != (checksum.hasMatch() ? 1 : 0))
        fail("Invalid or repeated VBF file checksum");
    auto parse = [&](qsizetype at) {
        Image img{"VBF " + v.captured(1) + " (segment CRC-16 verified)", {}, {}};
        img.header = QString::fromLatin1(file.constData(), pos);
        if (checksum.hasMatch()) {
            if (crc32(QByteArrayView(file).sliced(at)) != checksum.captured(1).toUInt(nullptr, 16))
                fail("VBF file checksum mismatch");
            img.format += " (body CRC-32 verified)";
        }
        while (at < file.size()) {
            if (file.size() - at < 10)
                fail("Truncated VBF segment");
            const char *p = file.constData() + at;
            auto addr = qFromBigEndian<quint32>(p), len = qFromBigEndian<quint32>(p + 4);
            if (!len || len > quint64(file.size() - at - 10))
                fail("Invalid VBF segment size");
            if (crc16(QByteArrayView(p + 8, len)) != qFromBigEndian<quint16>(p + 8 + len))
                fail("VBF segment CRC mismatch");
            insert(img, addr, QByteArray(p + 8, len));
            at += 10 + qsizetype(len);
        }
        if (img.segments.empty())
            fail("No VBF segments");
        return img;
    };
    try {
        return parse(pos);
    } catch (const std::runtime_error &) {
        if (file.mid(pos, 2) == "\r\n")
            return parse(pos + 2);
        if (file.mid(pos, 1) == "\n")
            return parse(pos + 1);
        throw;
    }
}
} // namespace

namespace {
struct MxShape {
    const char *key, *label;
    quint32 start, bytes;
};
constexpr MxShape mxShapes[] = {{"vid128", "128 Bytes - VID Block", 0x100c0, 128},
                                {"trid256", "256 Bytes - TRID Block", 0x10140, 256},
                                {"vidtrid384", "384 Bytes - VID and TRID Block", 0x100c0, 384},
                                {"trid128", "128 Bytes - TRID Block", 0x10140, 128}};
} // namespace
QStringList mxConfigProfiles() {
    QStringList keys;
    for (auto &p : mxShapes)
        keys << p.key;
    return keys;
}
void convertMxConfig(const QString &key, const QString &source, const QString &newVbf, const QString &part) {
    auto profile =
        std::find_if(std::begin(mxShapes), std::end(mxShapes), [&](const auto &p) { return key == p.key; });
    if (profile == std::end(mxShapes))
        fail("Unknown MX32 configuration-block profile");
    if (!(source.endsWith(".h32", Qt::CaseInsensitive) || source.endsWith(".hex", Qt::CaseInsensitive)) ||
        !newVbf.endsWith(".vbf", Qt::CaseInsensitive))
        fail("Expected addressed H32/HEX input and a new VBF output file");
    static const QRegularExpression partNumber("^[A-Za-z0-9_-]{1,7}$");
    if (!partNumber.match(part).hasMatch())
        fail("Expected a 1–7 character part number");
    auto img = readImage(source);
    if (img.segments.size() != 1 || img.segments.begin()->first != profile->start ||
        img.segments.begin()->second.size() != profile->bytes)
        fail("Input must cover the selected configuration block exactly (no gaps or extra bytes)");
    auto &data = img.segments.begin()->second;
    QByteArray body(8, 0);
    qToBigEndian(profile->start, body.data());
    qToBigEndian(profile->bytes, body.data() + 4);
    body += data;
    char crc[2];
    qToBigEndian(crc16(data), crc);
    body.append(crc, 2);
    auto hex = [](quint32 n) { return QByteArray::number(n, 16).rightJustified(8, '0').toUpper(); };
    QByteArray header = "vbf_version = 2.4;\nheader {\n    // OpenDiag: ";
    header += profile->label;
    header += "\n    sw_part_number = \"" + part.toLatin1() +
              "\";\n    sw_part_type = EXE;\n    network = CAN_HS;\n    ecu_address = 0x7E0;\n    "
              "data_format_identifier = 0x00;\n    frame_format = CAN_STANDARD;\n    erase = {{0x" +
              hex(profile->start) + ",0x" + hex(profile->bytes) + "}};\n    file_checksum = 0x" +
              hex(crc32(body)) + ";\n}";
    fileio::writeNew(newVbf, header + body);
}

Image readImage(const QString &path, quint32 binaryBase, QByteArray *sourceSha256) {
    if (!QFileInfo(path).isFile())
        fail("Expected an image file: " + path);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        fail(file.errorString());
    constexpr int limit = 64 * 1024 * 1024;
    if (file.size() > limit)
        fail("Input exceeds 64 MiB");
    auto bytes = file.read(limit + 1);
    if (file.error() || bytes.size() > limit || bytes.isEmpty())
        fail("Empty, too large or unreadable input");
    if (sourceSha256)
        *sourceSha256 = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
    auto ext = QFileInfo(file).suffix().toLower();
    Image img;
    if (ext == "hex" || ext == "h32")
        img = intelHex(bytes);
    else if (ext == "vbf")
        img = vbf(bytes);
    else if (ext == "bin") {
        img.format = "Binary";
        insert(img, binaryBase, std::move(bytes));
    } else
        fail("Expected .hex, Intel-record .h32, .vbf or .bin");
    // Out-of-order input can leave adjacent, non-overlapping ranges in the map.
    auto prev = img.segments.begin();
    for (auto next = std::next(prev); next != img.segments.end();) {
        if (quint64(prev->first) + prev->second.size() == next->first) {
            prev->second += next->second;
            next = img.segments.erase(next);
        } else
            prev = next++;
    }
    return img;
}

QByteArray Image::range(quint32 start, qsizetype length) const {
    auto next = segments.upper_bound(start);
    if (length <= 0 || next == segments.begin())
        fail("Unmapped image range");
    auto i = std::prev(next);
    auto pos = quint64(start) - i->first;
    if (pos + quint64(length) > quint64(i->second.size()))
        fail("Read crosses unmapped image data");
    return i->second.mid(qsizetype(pos), length);
}

QString Image::table(quint32 start, int rows, int columns, const QString &cell, const QString &order,
                     const QString &delimiter) const {
    const QStringList types = {"u8", "s8", "u16", "s16", "u32", "s32", "f32"};
    if (rows < 1 || rows > 65535 || columns < 1 || columns > 40 || qint64(rows) * columns > 250000)
        fail("Expected 1–40 columns and at most 250000 table cells");
    if (!types.contains(cell) || (order != "intel" && order != "motorola") ||
        (delimiter != "csv" && delimiter != "fixed"))
        fail("Unknown table cell type, byte order or delimiter");
    int width = cell.mid(1).toInt() / 8;
    auto data = range(start, qsizetype(rows) * columns * width);
    auto sep = delimiter == "csv" ? QString(",") : QString("  ");
    QString out = "(address)";
    for (int c = 0; c < columns; ++c)
        out += sep + (columns == 1 ? QString("data") : QString("col%1").arg(c + 1));
    out += '\n';
    int at = 0;
    for (int r = 0; r < rows; ++r) {
        out += '(' + QString("%1").arg(start + quint32(at), 8, 16, QChar('0')).toUpper() + ')';
        for (int c = 0; c < columns; ++c) {
            quint32 bits = 0;
            for (int i = 0; i < width; ++i)
                bits = (bits << 8) | quint8(data[at + (order == "intel" ? width - 1 - i : i)]);
            QString value;
            if (cell == "f32") {
                static_assert(sizeof(float) == 4);
                float n;
                std::memcpy(&n, &bits, 4);
                if (!std::isfinite(n))
                    fail("Non-finite float in translated table");
                value = QString::number(n, 'g', 9);
            } else {
                qint64 n = bits;
                if (cell[0] == 's' && (bits & (quint32(1) << (width * 8 - 1))))
                    n -= qint64(1) << (width * 8);
                value = QString::number(n);
            }
            out += sep + (delimiter == "fixed" ? value.rightJustified(16) : value);
            at += width;
        }
        out += '\n';
    }
    return out;
}

QString compareCalibration(const QString &backup, quint32 base, const QString &candidate) {
    if (!backup.endsWith(".bin", Qt::CaseInsensitive) || candidate.endsWith(".bin", Qt::CaseInsensitive))
        fail("Select a raw backup and an addressed HEX/H32/VBF candidate");
    auto original = readImage(backup, base), image = readImage(candidate);
    auto projected = original.segments.begin()->second;
    qsizetype touched = 0, changed = 0;
    QStringList lines{image.summary()};
    for (auto &[addr, bytes] : image.segments) {
        auto before = original.range(addr, bytes.size());
        qsizetype count = 0;
        for (qsizetype i = 0; i < bytes.size(); ++i)
            if (before[i] != bytes[i])
                ++count;
        changed += count;
        touched += bytes.size();
        projected.replace(qsizetype(addr - base), bytes.size(), bytes);
        lines << QString("0x%1: %2 bytes, %3 different")
                     .arg(addr, 8, 16, QChar('0'))
                     .arg(bytes.size())
                     .arg(count);
    }
    auto digest = [](const QByteArray &b) {
        return QString::fromLatin1(QCryptographicHash::hash(b, QCryptographicHash::Sha256).toHex());
    };
    lines << QString("%1 changed bytes; %2 addresses outside the candidate are retained in this comparison.")
                 .arg(changed)
                 .arg(projected.size() - touched)
          << "Backup SHA256: " + digest(original.segments.begin()->second)
          << "Projected SHA256: " + digest(projected);
    return lines.join('\n');
}

QString Image::summary() const {
    quint64 total = 0;
    for (auto &s : segments)
        total += s.second.size();
    return QString("%1: %2 bytes, %3 range(s), first address 0x%4, last 0x%5")
        .arg(format)
        .arg(total)
        .arg(segments.size())
        .arg(segments.begin()->first, 8, 16, QChar('0'))
        .arg(quint64(segments.rbegin()->first) + segments.rbegin()->second.size() - 1, 8, 16, QChar('0'));
}

QString Image::page(quint32 segment, qsizetype number) const {
    auto item = segments.find(segment);
    if (item == segments.end() || number < 0 || number > (item->second.size() - 1) / 256)
        fail("Page outside image range");
    QStringList lines;
    auto &bytes = item->second;
    for (auto i = number * 256; i < std::min(bytes.size(), (number + 1) * 256); i += 16) {
        auto row = bytes.mid(i, 16);
        QString ascii;
        for (auto b : row)
            ascii += b >= 32 && b <= 126 ? QChar::fromLatin1(b) : QChar('.');
        lines << QString("%1  %2  %3")
                     .arg(segment + quint32(i), 8, 16, QChar('0'))
                     .arg(QString::fromLatin1(row.toHex(' ').toUpper()), -47)
                     .arg(ascii);
    }
    return lines.join('\n');
}

LookupMatch lookupImage(const QString &path, const QString &moduleId, bool generic, QByteArray *sourceSha256) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        fail(file.errorString());
    if (file.size() > 4 * 1024 * 1024 || moduleId.isEmpty())
        fail("Expected lookup table and complete module ID");
    static const QRegularExpression record(
        R"re(^:[ ,\t]*0[ ,\t]+"([^"]+)"[ ,\t]+"([^"]+)"[ ,\t]+"([^"]+)"(?:[ ,\t]+"([^"]+)")?[ \t]*$)re");
    LookupMatch match;
    QCryptographicHash checksum(QCryptographicHash::Sha256);
    int n = 0;
    while (!file.atEnd()) {
        auto b = file.readLine(8192);
        if (sourceSha256)
            checksum.addData(b);
        ++n;
        if (file.error() || (!b.endsWith('\n') && !file.atEnd()))
            fail("Unreadable or oversized lookup record");
        if (!b.startsWith(':'))
            continue;
        auto m = record.match(QString::fromLatin1(b).trimmed());
        if (!m.hasMatch() || generic != !m.captured(4).isEmpty())
            fail("Invalid lookup record at line " + QString::number(n));
        if (m.captured(1).compare(moduleId, Qt::CaseInsensitive))
            continue;
        if (!match.primaryId.isEmpty())
            fail("Ambiguous match for " + moduleId);
        match = {m.captured(2), m.captured(3), m.captured(4)};
    }
    if (match.primaryId.isEmpty())
        fail("No full ID match for " + moduleId);
    if (sourceSha256)
        *sourceSha256 = checksum.result().toHex();
    return match;
}

void Image::saveHex(const QString &path, const QByteArray &preamble) const {
    // NewOnly prevents overwriting an input, existing conversion, or symlink destination.
    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::NewOnly))
        fail(out.errorString());
    auto write = [&](const QByteArray &line) {
        if (out.write(line) != line.size()) {
            out.remove();
            fail("Could not write Intel HEX output");
        }
    };
    auto record = [&](int type, quint16 addr, QByteArray data) {
        QByteArray r;
        r.append(char(data.size())).append(char(addr >> 8)).append(char(addr)).append(char(type)).append(data);
        quint8 sum = 0;
        for (char c : r)
            sum += quint8(c);
        r.append(char(-sum));
        write(":" + r.toHex().toUpper() + "\n");
    };
    if (!preamble.isEmpty())
        write(preamble);
    quint32 upper = 0;
    for (auto &[base, data] : segments)
        for (qsizetype i = 0; i < data.size();) {
            quint32 addr = base + quint32(i);
            if ((addr >> 16) != upper) {
                upper = addr >> 16;
                record(4, 0, QByteArray(1, char(upper >> 8)) + char(upper));
            }
            auto n = std::min<qsizetype>({16, 65536 - (addr & 0xffff), data.size() - i});
            record(0, quint16(addr), data.mid(i, n));
            i += n;
        }
    if (!entryRecord.isEmpty())
        write(entryRecord + "\n");
    record(1, 0, {});
    if (!out.flush()) {
        out.remove();
        fail("Could not finish Intel HEX output");
    }
}
