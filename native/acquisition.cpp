#include "acquisition.h"
#include "diagnostics.h"
#include "image.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QTextStream>
#include <cmath>
#include <cstring>
#include <stdexcept>

namespace acq {
[[noreturn]] static void fail(const QString &s) { throw std::runtime_error(s.toStdString()); }
QByteArray readFile(const QString &path, qsizetype limit) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        fail(f.errorString());
    auto bytes = f.read(limit + 1);
    if (f.error() || bytes.size() > limit)
        fail("Unreadable or oversized file: " + path);
    return bytes;
}
void writeFile(const QString &path, const QByteArray &bytes) {
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size() || !f.commit())
        fail(f.errorString());
}
static int integer(const QString &s, int low, int high, int base = 10) {
    bool ok;
    int x = s.toInt(&ok, base);
    if (!ok || x < low || x > high)
        fail("Invalid integer: " + s);
    return x;
}
static double real(const QString &s) {
    bool ok;
    double x = s.toDouble(&ok);
    if (!ok || !std::isfinite(x))
        fail("Invalid number: " + s);
    return x;
}
// Arithmetic expressions in data files are data, not scripts. Whitelist the supported grammar.
class Expression {
    const QString &s;
    double x;
    int at = 0;
    bool take(QChar c) {
        if (at < s.size() && s[at] == c) {
            ++at;
            return true;
        }
        return false;
    }
    double atom() {
        if (take('+'))
            return atom();
        if (take('-'))
            return -atom();
        if (take('x') || take('X'))
            return x;
        if (take('(')) {
            double n = sum();
            if (!take(')'))
                fail("Unmatched formula parenthesis");
            return n;
        }
        int start = at;
        while (at < s.size() && (s[at].isDigit() || s[at] == '.'))
            ++at;
        if (at == start)
            fail("Unsupported formula: " + s);
        if (take('e') || take('E')) {
            if (!take('+'))
                take('-');
            while (at < s.size() && s[at].isDigit())
                ++at;
        }
        return real(s.mid(start, at - start));
    }
    double product() {
        double n = atom();
        while (true) {
            if (take('*'))
                n *= atom();
            else if (take('/'))
                n /= atom();
            else
                return n;
        }
    }
    double sum() {
        double n = product();
        while (true) {
            if (take('+'))
                n += product();
            else if (take('-'))
                n -= product();
            else
                return n;
        }
    }

  public:
    Expression(const QString &formula, double value) : s(formula), x(value) {}
    double value() {
        if (s.size() > 128)
            fail("Formula exceeds 128 characters");
        double n = sum();
        if (at != s.size())
            fail("Unsupported formula: " + s);
        return n;
    }
};
Channel Channel::parse(const QString &fields) {
    auto f = fields.simplified().split(' ', Qt::SkipEmptyParts);
    if (f.size() < 6 || f.size() > 11)
        fail("Expected name, hex ID, type, size, binary point, translation, [scalar offset index formula "
             "units]");
    Channel c{};
    c.record = fields.simplified();
    c.name = f[0];
    static const QRegularExpression name(R"(^[A-Za-z0-9_.%/()\-]{1,80}$)");
    if (!name.match(c.name).hasMatch())
        fail("Unsupported channel name: " + c.name);
    const auto type = f[2].toLower();
    if (type != "p" && type != "k" && type != "1" && type != "2" && type != "d")
        fail("Unsupported acquisition type: " + type);
    bool obd = type == "1" || type == "2";
    c.dmr = type == "d";
    if (c.dmr) {
        bool ok;
        c.address = f[1].toUInt(&ok, 16);
        if (!ok)
            fail("Invalid DMR address");
    } else {
        auto id = integer(f[1], 0, obd ? 255 : 65535, 16);
        c.command = (type == "1"   ? "obd:"
                     : type == "2" ? "freeze:"
                                   : "did:") +
                    QString("%1").arg(id, obd ? 2 : 4, 16, QChar('0')).toUpper();
    }
    c.size = integer(f[3], 1, 4);
    c.point = integer(f[4], -31, 31);
    c.bit = -1;
    auto code = f[5].toLower();
    if (code == "s" || code == "u" || code == "f")
        c.encoding = code[0].toLatin1();
    else {
        c.encoding = 'b';
        c.bit = integer(code, 0, c.size * 8 - 1);
    }
    if ((c.encoding == 'b' && c.point) || (c.encoding == 'f' && (c.size != 4 || c.point)))
        fail("Invalid binary point or width for this translation");
    c.scalar = f.size() > 6 ? real(f[6]) : 1;
    c.offset = f.size() > 7 ? real(f[7]) : 0;
    c.packet = type == "k";
    c.index = f.size() > 8 ? integer(f[8], c.packet ? 1 : 0, 4095) : 1;
    if (type != "k" && c.index > 1)
        fail("Packet index requires type k");
    c.index = std::max(1, c.index);
    if (f.size() > 9 && f[9].compare("x", Qt::CaseInsensitive)) {
        c.formula = f[9];
        Expression(c.formula, 1).value();
    }
    if (f.size() > 10 && f[10].compare("na", Qt::CaseInsensitive))
        c.units = f[10];
    return c;
}
double Channel::translate(QByteArrayView bytes) const {
    if (index - 1 + size > bytes.size() || (!packet && bytes.size() != size))
        fail(name + ": reply width does not match the configured field");
    quint32 raw = 0;
    for (int i = 0; i < size; ++i)
        raw = (raw << 8) | quint8(bytes[index - 1 + i]);
    double n = raw;
    if (encoding == 'f') {
        float v;
        static_assert(sizeof(v) == 4);
        std::memcpy(&v, &raw, 4);
        n = v;
    } else if (encoding == 'b')
        n = (raw >> bit) & 1;
    else if (encoding == 's' && (raw & (quint32(1) << (8 * size - 1))))
        n = qint64(raw) - (qint64(1) << (8 * size));
    auto v = std::ldexp(n, -point) * scalar + offset;
    if (!formula.isEmpty())
        v = Expression(formula, v).value();
    if (!std::isfinite(v))
        fail(name + ": non-finite result");
    return v;
}
Config Config::parse(const QByteArray &data) {
    Config cfg;
    QSet<QString> names;
    int line = 0;
    for (auto &v : data.split('\n')) {
        ++line;
        auto s = QString::fromLatin1(v.data(), v.size());
        if (s.startsWith(':')) {
            try {
                auto c = Channel::parse(s.mid(1));
                if (names.contains(c.name.toCaseFolded()))
                    fail("Duplicate channel " + c.name);
                if (cfg.channels.size() >= 4096)
                    fail("List exceeds 4096 channels");
                names.insert(c.name.toCaseFolded());
                cfg.channels.append(c);
            } catch (const std::exception &e) {
                fail(QString("Line %1: %2").arg(line).arg(e.what()));
            }
        } else {
            auto h = s.trimmed().toLower();
            if (h == "<data_rate=single>" || h == "<data_rate=rapid>")
                cfg.rapid = h.endsWith("rapid>");
            else if (h == "<byte_order=motorola>" || h == "<byte_order=intel>")
                cfg.intel = h.endsWith("intel>");
            else if (h.startsWith("<data_rate=") || h.startsWith("<byte_order="))
                fail("Unknown acquisition setting: " + h);
        }
    }
    if (cfg.channels.isEmpty())
        fail("No PTDIAG channel records (':' in column one)");
    cfg.source = data;
    return cfg;
}
QByteArray Config::serialize() const {
    if (!source.isEmpty()) {
        QByteArray out;
        qsizetype idx = 0, pos = 0;
        auto eol = source.contains("\r\n") ? QByteArray("\r\n") : QByteArray("\n");
        while (pos < source.size()) {
            auto end = source.indexOf('\n', pos);
            auto row = source.mid(pos, (end < 0 ? source.size() : end + 1) - pos);
            pos += row.size();
            if (!row.startsWith(':')) {
                out += row;
                continue;
            }
            if (idx >= channels.size())
                continue;
            auto record = channels[idx++].record.toLatin1();
            if (row.mid(1).simplified() == record)
                out += row;
            else
                out += ": " + record + eol;
        }
        while (idx < channels.size()) {
            if (!out.endsWith('\n'))
                out += eol;
            out += ": " + channels[idx++].record.toLatin1() + eol;
        }
        return out;
    }
    QByteArray out = "<PID/DMR Configuration Data>\r\n";
    out += rapid ? "<data_rate=rapid>\r\n" : "<data_rate=single>\r\n";
    out += intel ? "<byte_order=intel>\r\n" : "<byte_order=motorola>\r\n";
    for (auto &c : channels)
        out += ": " + c.record.toLatin1() + "\r\n";
    return out;
}
QByteArray Config::fingerprint() const {
    QByteArray key = rapid ? "rapid\n" : "single\n";
    key += intel ? "intel\n" : "motorola\n";
    for (const auto &channel : channels)
        key += channel.record.toUtf8() + '\n';
    return QCryptographicHash::hash(key, QCryptographicHash::Sha256).toHex();
}
Init Init::load(const QString &path) {
    Init ini;
    auto data = readFile(path, 1024 * 1024);
    int n = 0;
    for (const auto &line : data.split('\n')) {
        ++n;
        auto s = QString::fromLatin1(line).trimmed();
        auto eq = s.indexOf('=');
        if (eq < 1)
            continue;
        auto key = s.left(eq).trimmed().toUpper(), val = s.mid(eq + 1).trimmed();
        if (val.size() >= 2 && val.startsWith('"') && val.endsWith('"'))
            val = val.mid(1, val.size() - 2);
        auto flag = [&] {
            auto x = val.toUpper();
            if (x != "ENABLED" && x != "DISABLED")
                fail(QString("INI line %1: expected ENABLED or DISABLED").arg(n));
            return x == "ENABLED";
        };
        if (key == "SAMPLE_RATE")
            ini.interval = integer(val, 0, 60000);
        else if (key == "AUTO_CONFIG_LOAD")
            ini.autoConfig = flag();
        else if (key == "AUTO_PID_LOAD")
            ini.autoPid = flag();
        else if (key == "CONFIG_PATH" || key == "PID_LIST_PATH") {
            if (val.isEmpty() || val.contains('"'))
                fail("Invalid INI path at line " + QString::number(n));
            val.replace('\\', '/');
            if (!QDir::isAbsolutePath(val) && !(val.size() > 2 && val[1] == ':'))
                val = QFileInfo(path).dir().filePath(val);
            (key == "CONFIG_PATH" ? ini.configPath : ini.pidPath) = val;
        } else if (!key.startsWith(';') && !key.startsWith('#'))
            ini.unhandled << key;
    }
    return ini;
}
void Capture::append(Sample sample, size_t capacity) {
    if (!capacity || sample.values.size() != names.size())
        fail("Invalid capture dimensions");
    if (rows.size() == capacity)
        rows.pop_front();
    rows.push_back(std::move(sample));
}
void Capture::save(const QString &path) const {
    if (rows.empty())
        fail("No captured samples");
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        fail(file.errorString());
    QTextStream out(&file);
    out.setLocale(QLocale::c());
    out.setRealNumberPrecision(17);
    out << "created by > OpenDiag\r\ntime units = milliseconds\r\nmilliseconds," << names.join(',') << "\r\n";
    for (auto &row : rows) {
        out << row.ms;
        for (double v : row.values)
            out << ',' << v;
        out << "\r\n";
    }
    out << " -- end of file --\r\n";
    out.flush();
    if (out.status() != QTextStream::Ok || !file.commit())
        fail(file.errorString());
}
Capture Capture::load(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        fail(file.errorString());
    Capture cap;
    bool seconds = false;
    int n = 0;
    while (!file.atEnd()) {
        auto b = file.readLine(32768);
        ++n;
        if (file.error() || (!b.endsWith('\n') && !file.atEnd()) || n > 201000)
            fail("Invalid or oversized capture");
        auto s = QString::fromUtf8(b).trimmed();
        if (s.isEmpty())
            continue;
        if (cap.names.isEmpty()) {
            if (s.startsWith("milliseconds,") || s.startsWith("seconds,")) {
                auto fields = s.split(',');
                seconds = fields.takeFirst() == "seconds";
                cap.names = fields;
                if (fields.size() > 64 || fields.contains(QString()))
                    fail("Invalid capture columns");
            }
            continue;
        }
        if (s == "-- end of file --" || s == "-- end of data --")
            break;
        auto fields = s.split(',');
        if (fields.size() != cap.names.size() + 1)
            fail("Capture column count at line " + QString::number(n));
        double ms = real(fields.takeFirst()) * (seconds ? 1000 : 1);
        if (ms < 0 || ms > 9007199254740991.0 || (!cap.rows.empty() && std::llround(ms) < cap.rows.back().ms))
            fail("Invalid capture timestamp");
        Sample row{std::llround(ms), {}};
        for (auto &f : fields)
            row.values.append(real(f));
        if (cap.rows.size() >= 200000)
            fail("Capture exceeds 200000 samples");
        cap.rows.push_back(std::move(row));
    }
    if (cap.rows.empty())
        fail("No numeric PTDIAG capture rows found");
    return cap;
}
Capture Capture::replay(const Config &cfg, const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        fail(file.errorString());
    if (file.size() > 64 * 1024 * 1024 || cfg.channels.isEmpty() || cfg.channels.size() > 64)
        fail("Invalid replay dimensions");
    QHash<QByteArray, QString> reads;
    QStringList names;
    for (auto &c : cfg.channels) {
        auto op = diag::operation(c.command.toStdString());
        reads.insert(QByteArray(reinterpret_cast<const char *>(op.request.data()), op.request.size()),
                     c.command);
        names << c.name;
    }
    Capture cap;
    bool active = false, opened = false;
    int capacity = 0;
    QByteArray tx, rx, pending;
    QHash<QString, QByteArray> results;
    auto digest = cfg.fingerprint();
    qint64 previous = 0;
    auto bytes = [](QList<QByteArray> pieces) {
        QByteArray payload;
        for (auto &piece : pieces) {
            auto b = QByteArray::fromHex(piece);
            if (piece.size() != 2 || b.size() != 1 || b.toHex().toUpper() != piece.toUpper())
                fail("Invalid trace payload");
            payload += b;
        }
        return payload;
    };
    while (!file.atEnd()) {
        auto line = file.readLine(32768);
        if (file.error() || (!line.endsWith('\n') && !file.atEnd()))
            fail("Invalid trace line");
        if (line.size() > 10 && line[4] == '-' && line[7] == '-') {
            auto pos = line.indexOf("  ");
            if (pos >= 0)
                line.remove(0, pos + 2);
        }
        auto f = line.simplified().split(' ');
        if (f.value(0) == "ACQ" && f.value(1) == "BEGIN") {
            if (active)
                fail("Unterminated acquisition in trace");
            if (f.value(2) != digest)
                continue;
            auto pair = f.value(4).split('/');
            if (pair.size() != 2 || f.size() != 5)
                fail("Invalid acquisition trace header");
            diag::Link::parse(pair[0].toStdString(), pair[1].toStdString(), "500000");
            tx = pair[0];
            rx = pair[1];
            capacity = integer(QString::fromLatin1(f[3]), 1, 200000);
            cap = {names, {}};
            pending.clear();
            results.clear();
            previous = 0;
            active = opened = true;
        } else if (active) {
            if (f.value(0) == "REQUEST") {
                if (!pending.isEmpty() || f.value(1) != tx)
                    fail("Unexpected request in acquisition trace");
                pending = bytes(f.mid(2));
                if (!reads.contains(pending) || results.contains(reads.value(pending)))
                    fail("Trace does not match channel configuration");
            } else if (f.value(0) == "RESPONSE") {
                if (pending.isEmpty() || f.value(1) != rx)
                    fail("Unexpected response in acquisition trace");
                auto reply = bytes(f.mid(2));
                auto op = diag::operation(reads.value(pending).toStdString());
                diag::Bytes value(reply.begin(), reply.end());
                if (diag::matches(op, value))
                    results[reads.value(pending)] = reply.mid(pending.size());
                else if (!(reply.size() == 3 && quint8(reply[0]) == 0x7f && reply[1] == pending[0]))
                    fail("Wrong response in trace");
                pending.clear();
            } else if (f.value(0) == "ACQ" && f.value(1) == "SAMPLE") {
                bool ok;
                auto ms = f.value(2).toLongLong(&ok);
                if (!ok || ms < previous || !pending.isEmpty() || results.size() != reads.size())
                    fail("Incomplete or unordered trace sample");
                Sample row{ms, {}};
                for (auto &c : cfg.channels)
                    row.values << c.translate(results.value(c.command));
                cap.append(std::move(row), size_t(capacity));
                previous = ms;
                results.clear();
            } else if (f.value(0) == "ACQ" && f.value(1) == "END")
                active = false;
        }
    }
    if (active || !opened || cap.rows.empty())
        fail("No complete acquisition matching this configuration in the session log");
    return cap;
}
Capture Capture::snapshot(const Config &cfg, const QString &path, quint32 binaryBase) {
    if (cfg.channels.isEmpty() || cfg.channels.size() > 64)
        fail("Expected 1–64 DMR records");
    auto image = readImage(path, binaryBase);
    Capture cap;
    Sample sample{0, {}};
    for (auto &c : cfg.channels) {
        if (!c.dmr)
            fail("A memory snapshot requires a DMR-only configuration");
        auto bytes = image.range(c.address, c.size);
        if (cfg.intel)
            std::reverse(bytes.begin(), bytes.end());
        cap.names << c.name;
        sample.values << c.translate(bytes);
    }
    cap.append(std::move(sample), 1);
    return cap;
}
} // namespace acq
