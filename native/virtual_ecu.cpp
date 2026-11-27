#include "virtual_ecu.h"
#include "files.h"
#include "image.h"
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QUuid>
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#ifdef Q_OS_WIN
#include <io.h>
#else
#include <unistd.h>
#endif

namespace {
constexpr int maxBank = 64 * 1024 * 1024;
[[noreturn]] void fail(const QString &s) { throw std::runtime_error(s.toStdString()); }
QString digest(const QByteArray &data) {
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
}
QJsonObject object(const QJsonValue &v, const QString &name) {
    if (!v.isObject())
        fail("Expected object: " + name);
    return v.toObject();
}
QJsonObject loadJson(const QString &path) {
    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(fileio::readFile(path, 256 * 1024), &err);
    if (err.error || !doc.isObject())
        fail("Invalid JSON object: " + path);
    return doc.object();
}
void checkKeys(const QJsonObject &o, const QStringList &allowed) {
    for (auto i = o.begin(); i != o.end(); ++i)
        if (!allowed.contains(i.key()))
            fail("Unknown field: " + i.key());
}
QString text(const QJsonObject &o, const QString &key) {
    auto s = o.value(key).toString();
    if (s.isEmpty() || s.size() > 255 || !std::all_of(s.begin(), s.end(), [](QChar c) { return c.isPrint(); }))
        fail("Invalid or missing " + key);
    return s;
}
int number(const QJsonObject &o, const QString &key, int min, int max) {
    auto n = o.value(key).toInt(-1);
    if (n < min || n > max)
        fail("Out of range or missing: " + key);
    return n;
}
quint32 address(const QJsonObject &o, const QString &key) {
    auto s = text(o, key);
    static const QRegularExpression pattern("^(?:0[xX])?[0-9a-fA-F]{1,8}$");
    if (!pattern.match(s).hasMatch())
        fail("Expected hexadecimal address: " + key);
    return s.toUInt(nullptr, 16);
}
QString hashField(const QJsonObject &o, const QString &key) {
    const auto s = text(o, key).toLower();
    static const QRegularExpression pattern("^[0-9a-f]{64}$");
    if (!pattern.match(s).hasMatch())
        fail("Expected SHA-256: " + key);
    return s;
}
QString local(const QDir &dir, const QString &name) {
    if (name.isEmpty() || name == "." || name == ".." || name.contains('/') || name.contains('\\') ||
        name.contains(':'))
        fail("Expected a filename in " + dir.absolutePath());
    auto path = dir.absoluteFilePath(name);
    if (QFileInfo(path).isSymLink())
        fail("Symbolic link not allowed: " + path);
    return path;
}
Image artifact(const QDir &dir, const QJsonValue &v, quint32 base, bool allowBin, bool block = false) {
    auto a = object(v, "image");
    auto keys = QStringList{"file", "sha256"};
    if (block)
        keys << "address";
    checkKeys(a, keys);
    auto path = local(dir, text(a, "file"));
    auto expected = hashField(a, "sha256");
    bool bin = path.endsWith(".bin", Qt::CaseInsensitive);
    if (block && bin)
        base = address(a, "address");
    if (!bin && a.contains("address"))
        fail("Do not override embedded image addresses");
    if (!allowBin && bin && !block)
        fail("Candidate and loader must carry their addresses");
    QByteArray actual;
    auto img = readImage(path, base, &actual);
    if (QString::fromLatin1(actual) != expected)
        fail("Source SHA-256 mismatch: " + path);
    return img;
}
void validateLookup(const QDir &dir, const QJsonObject &r, const QString &installedId) {
    if (!r.contains("lookup"))
        return;
    auto table = object(r.value("lookup"), "lookup");
    checkKeys(table, {"file", "sha256"});
    QByteArray actual;
    auto match =
        lookupImage(local(dir, text(table, "file")), installedId, r.value("mode") == "generic", &actual);
    if (QString::fromLatin1(actual) != hashField(table, "sha256"))
        fail("Lookup table SHA-256 mismatch");
    auto name = [](QString path) { return path.replace('\\', '/').section('/', -1); };
    if (match.primaryId.compare(text(r, "to"), Qt::CaseInsensitive) ||
        name(match.primaryPath)
            .compare(text(object(r.value("primary"), "primary"), "file"), Qt::CaseInsensitive) ||
        name(match.sblPath).compare(r.value("loader").toObject().value("file").toString(), Qt::CaseInsensitive))
        fail("Lookup table match does not select the job's software and files");
}
struct Range {
    QString name;
    quint32 address;
    int length;
};
QJsonArray kamRanges(const QByteArray &data) {
    QJsonArray result;
    int n = 0;
    static const QRegularExpression row(R"(^:\s*([0-9a-fA-F]{1,8})\s+([0-9]{1,5})\s*$)");
    for (const auto &line : data.split('\n')) {
        ++n;
        if (!line.startsWith(':'))
            continue;
        auto m = row.match(QString::fromLatin1(line).trimmed());
        if (!m.hasMatch() || result.size() >= 20)
            fail("Invalid KAM configuration at line " + QString::number(n));
        int len = m.captured(2).toInt();
        if (len < 1 || len > 65535)
            fail("Invalid KAM region length");
        result.append(QJsonObject{{"name", QString("KAM-%1").arg(result.size() + 1, 2, 10, QChar('0'))},
                                  {"address", m.captured(1)},
                                  {"bytes", len}});
    }
    if (result.isEmpty())
        fail("No KAM region records in configuration");
    return result;
}
struct Layout {
    quint32 base;
    int bytes, sector;
    QVector<Range> retain;
};
QString bankIdentity(const QByteArray &bank, const QJsonObject &profile) {
    auto field = object(profile.value("identityField"), "identityField");
    auto raw = bank.mid(address(field, "address") - address(profile, "base"), number(field, "bytes", 1, 128));
    while (raw.endsWith('\0') || raw.endsWith(' '))
        raw.chop(1);
    if (raw.isEmpty() || !std::all_of(raw.cbegin(), raw.cend(), [](char c) { return c >= 0x20 && c <= 0x7e; }))
        fail("Invalid software ID in virtual flash");
    return QString::fromLatin1(raw);
}
Layout layout(const QJsonObject &p) {
    checkKeys(p, {"format", "part", "software", "base", "bytes", "sectorBytes", "supplyMv", "baseline",
                  "retain", "ram", "identityField"});
    if (p.value("format") != "opendiag-virtual-ecu/1")
        fail("Not an OpenDiag virtual ECU profile");
    text(p, "part");
    text(p, "software");
    number(p, "supplyMv", 0, 60000);
    Layout l{address(p, "base"), number(p, "bytes", 1, maxBank), number(p, "sectorBytes", 16, 65536), {}};
    if ((l.sector & (l.sector - 1)) || l.base % l.sector || l.bytes % l.sector || l.bytes / l.sector > 65536 ||
        quint64(l.base) + l.bytes > (quint64(1) << 32))
        fail("Invalid flash geometry");
    auto id = object(p.value("identityField"), "identityField");
    checkKeys(id, {"address", "bytes"});
    quint64 idAt = address(id, "address"), idEnd = idAt + number(id, "bytes", 1, 128);
    if (idAt < l.base || idEnd > quint64(l.base) + l.bytes)
        fail("Software ID field outside flash");
    if (!p.value("retain").isArray() || p.value("retain").toArray().size() > 256)
        fail("Expected a retain array");
    for (const auto &v : p.value("retain").toArray()) {
        auto r = object(v, "retain range");
        checkKeys(r, {"name", "address", "bytes"});
        Range item{text(r, "name"), address(r, "address"), number(r, "bytes", 1, l.bytes)};
        if (item.address < l.base || quint64(item.address) + item.length > quint64(l.base) + l.bytes)
            fail("Retained range outside flash");
        if (item.address < idEnd && idAt < quint64(item.address) + item.length)
            fail("Retained range overlaps software ID field");
        for (const auto &old : l.retain)
            if (old.name == item.name || (quint64(old.address) < quint64(item.address) + item.length &&
                                          quint64(item.address) < quint64(old.address) + old.length))
                fail("Duplicate or overlapping retained range");
        l.retain.append(item);
    }
    if (p.contains("ram")) {
        auto r = object(p.value("ram"), "ram");
        checkKeys(r, {"base", "bytes"});
        quint64 start = address(r, "base"), end = start + number(r, "bytes", 1, 16 * 1024 * 1024);
        if (end > (quint64(1) << 32) || (start < quint64(l.base) + l.bytes && l.base < end))
            fail("Invalid RAM geometry");
    }
    return l;
}
void overlay(QByteArray &bank, quint32 base, const Image &img) {
    for (const auto &[start, bytes] : img.segments) {
        if (start < base || quint64(start) + bytes.size() > quint64(base) + bank.size())
            fail("Image segment outside its mapped bank");
        std::copy(bytes.cbegin(), bytes.cend(), bank.begin() + (start - base));
    }
}
QVector<int> changedSectors(const QByteArray &old, const QByteArray &goal, int width) {
    QVector<int> result;
    for (int at = 0; at < old.size(); at += width)
        if (QByteArrayView(old).sliced(at, width) != QByteArrayView(goal).sliced(at, width))
            result.append(at);
    return result;
}
QByteArray checked(const QString &path, int bytes, const QString &sha) {
    auto data = fileio::readFile(path, bytes);
    if (data.size() != bytes || digest(data) != sha)
        fail("Size or SHA-256 mismatch: " + path);
    return data;
}
void sectorWrite(const QString &path, int bankBytes, int at, const QByteArray &data, bool erase) {
    std::error_code ec;
    if (std::filesystem::hard_link_count(QFileInfo(path).filesystemFilePath(), ec) != 1 || ec)
        fail("Expected an exclusively owned virtual flash file");
    QFile f(path);
    if (!f.open(QIODevice::ReadWrite | QIODevice::ExistingOnly) || f.size() != bankBytes || !f.seek(at))
        fail("Cannot open virtual flash sector");
    auto old = f.read(data.size());
    if (old.size() != data.size())
        fail("Short read from virtual flash");
    if (!erase)
        for (int i = 0; i < data.size(); ++i)
            if ((quint8(old[i]) & quint8(data[i])) != quint8(data[i]))
                fail("Flash bit requires erasure; restore the verified backup");
    if (!f.seek(at) || f.write(data) != data.size() || !f.flush())
        fail("Failed to write virtual flash");
#ifdef Q_OS_WIN
    if (_commit(f.handle()) != 0)
        fail("Failed to sync virtual flash");
#else
    if (fsync(f.handle()) != 0)
        fail("Failed to sync virtual flash");
#endif
    if (!f.seek(at) || f.read(data.size()) != data)
        fail("Sector read-back mismatch");
}
} // namespace

void VirtualEcu::create(const QString &directory, const QString &profile) {
    auto p = loadJson(profile);
    if (p.value("retain").isObject()) {
        auto a = p.value("retain").toObject();
        checkKeys(a, {"file", "sha256"});
        auto bytes = fileio::readFile(local(QFileInfo(profile).dir(), text(a, "file")), 256 * 1024);
        if (digest(bytes) != hashField(a, "sha256"))
            fail("KAM configuration SHA-256 mismatch");
        p["retain"] = kamRanges(bytes);
    }
    auto l = layout(p);
    auto image = artifact(QFileInfo(profile).dir(), p.value("baseline"), l.base, true);
    if (image.segments.size() != 1 || image.segments.begin()->first != l.base ||
        image.segments.begin()->second.size() != l.bytes)
        fail("Baseline must cover the entire flash bank exactly");
    const auto &bank = image.segments.begin()->second;
    if (bankIdentity(bank, p) != text(p, "software"))
        fail("Baseline software ID does not match the device profile");
    QFileInfo target(directory);
    QDir parent = target.absoluteDir();
    auto name = target.fileName();
    if (name.isEmpty() || !parent.mkdir(name))
        fail("Choose a new workspace directory (existing paths are never reused)");
    QDir dir(target.absoluteFilePath());
    try {
        fileio::writeFile(local(dir, "flash.bin"), bank);
        QJsonObject s{{"format", "opendiag-virtual-state/1"},
                      {"profile", p},
                      {"phase", "idle"},
                      {"software", p.value("software")},
                      {"installedSha256", digest(bank)},
                      {"supplyMv", p.value("supplyMv")},
                      {"cursor", 0}};
        fileio::writeFile(local(dir, "state.json"), QJsonDocument(s).toJson());
    } catch (...) {
        QFile::remove(dir.filePath("flash.bin"));
        parent.rmdir(name);
        throw;
    }
}
VirtualEcu::VirtualEcu(const QString &directory)
    : root(QFileInfo(directory).absoluteFilePath()), lock(root.filePath("session.lock")) {
    if (!root.exists() || QFileInfo(root.path()).isSymLink())
        fail("Expected a virtual ECU workspace");
    lock.setStaleLockTime(0);
    if (!lock.tryLock())
        fail("Virtual ECU is busy or cannot be locked");
    state = loadJson(local(root, "state.json"));
    if (state.value("format") != "opendiag-virtual-state/1")
        fail("Not an OpenDiag virtual ECU workspace");
    auto l = layout(object(state.value("profile"), "profile"));
    number(state, "supplyMv", 0, 60000);
    text(state, "software");
    hashField(state, "installedSha256");
    const QStringList phases = {"idle",      "prepared", "erasing",   "programming", "verifying",
                                "resetting", "complete", "restoring", "restored"};
    if (!phases.contains(phase()))
        fail("Unknown virtual ECU phase");
    if (phase() != "idle") {
        auto j = object(state.value("job"), "job");
        asset("backup.bin");
        int total = number(j, "sectors", 1, l.bytes / l.sector);
        number(state, "cursor", 0, total);
        number(j, "minMv", 1, 60000);
        number(j, "maxMv", j.value("minMv").toInt(), 60000);
        hashField(j, "beforeSha256");
        hashField(j, "afterSha256");
        text(j, "from");
        text(j, "to");
    }
}
QString VirtualEcu::phase() const { return state.value("phase").toString(); }
bool VirtualEcu::finished() const {
    return phase() == "idle" || phase() == "complete" || phase() == "restored";
}
QString VirtualEcu::asset(const QString &name) const {
    auto id = text(state.value("job").toObject(), "id");
    if (QUuid(id).isNull() || QUuid(id).toString(QUuid::WithoutBraces) != id)
        fail("Invalid job ID");
    return local(QDir(local(root, "job-" + id)), name);
}
void VirtualEcu::commit(QJsonObject next) {
    fileio::writeFile(local(root, "state.json"), QJsonDocument(next).toJson());
    state = std::move(next);
}
void VirtualEcu::checkVoltage(const QJsonObject &j) const {
    int mv = state.value("supplyMv").toInt();
    if (mv < j.value("minMv").toInt() || mv > j.value("maxMv").toInt())
        fail("Virtual supply voltage outside the job range; checkpoint retained");
}
void VirtualEcu::setVoltage(int mv) {
    if (mv < 0 || mv > 60000)
        fail("Supply must be 0–60000 mV");
    auto s = state;
    s["supplyMv"] = mv;
    commit(s);
}
QJsonObject VirtualEcu::status() const {
    auto s = state;
    s["target"] = "file-backed virtual ECU";
    s["workspace"] = root.absolutePath();
    bool bootable = finished() || phase() == "prepared";
    if (bootable) {
        try {
            auto p = state["profile"].toObject();
            auto data =
                checked(local(root, "flash.bin"), p["bytes"].toInt(), hashField(state, "installedSha256"));
            auto id = bankIdentity(data, p);
            if (id != text(state, "software"))
                fail("Virtual boot software ID mismatch");
            s["reportedSoftware"] = id;
        } catch (const std::exception &e) {
            s["integrityError"] = QString::fromUtf8(e.what());
            bootable = false;
        }
    }
    s["bootable"] = bootable;
    return s;
}
void VirtualEcu::prepare(const QString &recipe) {
    if (!finished())
        fail("Finish or restore the current job first");
    auto p = state.value("profile").toObject();
    auto l = layout(p);
    auto r = loadJson(recipe);
    auto dir = QFileInfo(recipe).dir();
    checkKeys(r, {"format", "mode", "part", "from", "to", "minMv", "maxMv", "primary", "loader", "lookup"});
    if (r.value("format") != "opendiag-program-job/1")
        fail("Unsupported job format");
    auto mode = text(r, "mode");
    if (mode != "pcm" && mode != "generic" && mode != "block" && mode != "configuration")
        fail("Unsupported job mode");
    if ((mode == "block" || mode == "configuration") &&
        (r.value("to") != r.value("from") || r.contains("lookup")))
        fail("Block/configuration jobs must retain software identity and select their image directly");
    if (text(r, "part") != text(p, "part") || text(r, "from") != text(state, "software"))
        fail("Part or installed software ID does not match job");
    auto to = text(r, "to");
    int min = number(r, "minMv", 1, 60000), max = number(r, "maxMv", min, 60000);
    checkVoltage(r);
    auto backup = checked(local(root, "flash.bin"), l.bytes, hashField(state, "installedSha256"));
    auto goal = backup;
    if (bankIdentity(backup, p) != text(r, "from"))
        fail("Installed flash does not report the expected software ID");
    validateLookup(dir, r, bankIdentity(backup, p));
    auto img = artifact(dir, r.value("primary"), l.base, false, mode == "block");
    overlay(goal, l.base, img);
    int retainedDifferences = 0;
    QJsonArray restoredRanges;
    if (mode == "configuration") {
        qsizetype payload = 0, covered = 0;
        for (const auto &s : img.segments)
            payload += s.second.size();
        for (const auto &range : l.retain) {
            auto it = img.segments.upper_bound(range.address);
            if (it == img.segments.begin())
                continue;
            --it;
            auto at = quint64(range.address) - it->first;
            if (at + range.length <= quint64(it->second.size())) {
                covered += range.length;
                restoredRanges.append(range.name);
            }
        }
        if (!covered || covered != payload)
            fail("Configuration image must contain only complete named retained ranges");
    } else
        for (const auto &range : l.retain) {
            int at = int(range.address - l.base);
            for (int i = at; i < at + range.length; ++i) {
                if (goal[i] != backup[i])
                    ++retainedDifferences;
                goal[i] = backup[i];
            }
        }
    auto changed = changedSectors(backup, goal, l.sector);
    if (bankIdentity(goal, p) != to)
        fail("Candidate flash does not contain the requested software ID");
    if (changed.isEmpty())
        fail("No changed flash sectors after applying retention rules");
    auto id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QJsonObject job{{"id", id},
                    {"from", r.value("from")},
                    {"to", to},
                    {"mode", mode},
                    {"minMv", min},
                    {"maxMv", max},
                    {"sectors", int(changed.size())},
                    {"retainedDifferences", retainedDifferences},
                    {"primary", r.value("primary")},
                    {"beforeSha256", digest(backup)},
                    {"afterSha256", digest(goal)}};
    if (r.contains("lookup"))
        job["lookup"] = r.value("lookup");
    if (!restoredRanges.isEmpty())
        job["restoredRanges"] = restoredRanges;
    QByteArray loader;
    if (r.contains("loader")) {
        if (mode != "generic" || !p.contains("ram"))
            fail("A secondary loader needs a generic job and a mapped RAM bank");
        auto mem = p.value("ram").toObject();
        loader = QByteArray(mem.value("bytes").toInt(), 0);
        auto base = address(mem, "base");
        overlay(loader, base, artifact(dir, r.value("loader"), base, false));
        job["loader"] = r.value("loader");
        job["ramSha256"] = digest(loader);
    }
    auto folder = "job-" + id;
    if (!root.mkdir(folder))
        fail("Cannot create job backup directory");
    QDir assets(root.filePath(folder));
    try {
        fileio::writeFile(local(assets, "backup.bin"), backup);
        fileio::writeFile(local(assets, "desired.bin"), goal);
        if (!loader.isEmpty())
            fileio::writeFile(local(assets, "loader.bin"), loader);
        auto s = state;
        s["job"] = job;
        s["phase"] = "prepared";
        s["cursor"] = 0;
        commit(s);
    } catch (...) {
        for (const auto &f : {"backup.bin", "desired.bin", "loader.bin"})
            QFile::remove(assets.filePath(f));
        root.rmdir(folder);
        throw;
    }
    desired.clear();
    sectors.clear();
    ram.clear();
}
void VirtualEcu::loadJob() {
    if (!desired.isEmpty())
        return;
    auto j = state.value("job").toObject();
    auto p = state.value("profile").toObject();
    auto l = layout(p);
    auto old = checked(asset("backup.bin"), l.bytes, hashField(j, "beforeSha256"));
    auto goal = checked(asset("desired.bin"), l.bytes, hashField(j, "afterSha256"));
    auto changed = changedSectors(old, goal, l.sector);
    if (changed.size() != j.value("sectors").toInt())
        fail("Job sector map mismatch");
    if (j.contains("loader"))
        ram = checked(asset("loader.bin"), object(p.value("ram"), "ram").value("bytes").toInt(),
                      hashField(j, "ramSha256"));
    desired = std::move(goal);
    sectors = std::move(changed);
}
void VirtualEcu::step() {
    if (phase() == "idle")
        fail("Prepare a job first");
    if (finished())
        return;
    if (phase() == "restoring") {
        restoreBank();
        return;
    }
    auto s = state;
    auto j = s.value("job").toObject();
    checkVoltage(j);
    loadJob();
    auto p = s.value("profile").toObject();
    int width = p.value("sectorBytes").toInt(), cursor = s.value("cursor").toInt();
    auto bank = local(root, "flash.bin");
    if (phase() == "prepared") {
        checked(bank, desired.size(), hashField(j, "beforeSha256"));
        s["phase"] = "erasing";
    } else if (phase() == "erasing" || phase() == "programming") {
        if (cursor < 0 || cursor >= sectors.size())
            fail("Invalid sector checkpoint");
        bool erase = phase() == "erasing";
        int at = sectors[cursor];
        sectorWrite(bank, desired.size(), at, erase ? QByteArray(width, char(0xff)) : desired.mid(at, width),
                    erase);
        if (++cursor == sectors.size()) {
            s["phase"] = erase ? "programming" : "verifying";
            cursor = 0;
        }
        s["cursor"] = cursor;
    } else if (phase() == "verifying" || phase() == "resetting") {
        auto actual = checked(bank, desired.size(), hashField(j, "afterSha256"));
        if (bankIdentity(actual, p) != text(j, "to"))
            fail("Programmed bank failed software identification");
        if (phase() == "verifying")
            s["phase"] = "resetting";
        else {
            s["phase"] = "complete";
            s["software"] = j.value("to");
            s["installedSha256"] = j.value("afterSha256");
            ram.clear();
        }
    } else
        fail("Unexpected checkpoint");
    commit(s);
}
void VirtualEcu::restoreBank() {
    auto j = state.value("job").toObject();
    checkVoltage(j);
    auto bank = local(root, "flash.bin");
    auto sha = hashField(j, "beforeSha256");
    int bytes = state.value("profile").toObject().value("bytes").toInt();
    auto backup = checked(asset("backup.bin"), bytes, sha);
    auto s = state;
    if (bankIdentity(backup, state.value("profile").toObject()) != text(j, "from"))
        fail("Backup identity mismatch");
    if (phase() != "restoring") {
        s["phase"] = "restoring";
        s["cursor"] = 0;
        commit(s);
    }
    fileio::writeFile(bank, backup);
    checked(bank, bytes, sha);
    s["phase"] = "restored";
    s["software"] = j.value("from");
    s["installedSha256"] = sha;
    commit(s);
    ram.clear();
}
void VirtualEcu::restore() {
    if (phase() == "idle")
        fail("No job backup");
    restoreBank();
}
QByteArray VirtualEcu::installedBank() const {
    if (!finished() && phase() != "prepared")
        fail("Virtual bank is not bootable; resume or restore first");
    auto p = state.value("profile").toObject();
    auto data =
        checked(local(root, "flash.bin"), p.value("bytes").toInt(), hashField(state, "installedSha256"));
    if (bankIdentity(data, p) != text(state, "software"))
        fail("Installed software ID mismatch");
    return data;
}
void VirtualEcu::exportBank(const QString &newPath, bool backup) const {
    auto j = state.value("job").toObject();
    auto size = state.value("profile").toObject().value("bytes").toInt();
    if (backup && j.isEmpty())
        fail("No job backup");
    auto data = backup ? checked(asset("backup.bin"), size, hashField(j, "beforeSha256")) : installedBank();
    fileio::writeNew(newPath, data);
}
void VirtualEcu::exportRange(const QString &path, quint32 start, quint32 n) const {
    auto l = layout(state.value("profile").toObject());
    if (!n || start < l.base || quint64(start) + n > quint64(l.base) + l.bytes)
        fail("Requested block outside virtual flash");
    fileio::writeNew(path, installedBank().mid(start - l.base, n));
}
void VirtualEcu::exportConfiguration(const QString &path, const QString &name) const {
    auto l = layout(state.value("profile").toObject());
    auto bank = installedBank();
    Image img;
    for (const auto &r : l.retain)
        if (name.isEmpty() || name == r.name)
            img.segments.emplace(r.address, bank.mid(r.address - l.base, r.length));
    if (img.segments.empty())
        fail("No matching named retention range");
    img.saveHex(path);
}
void VirtualEcu::exportRecoveryKit(const QString &destination) const {
    auto profile = state.value("profile").toObject();
    auto job = state.value("job").toObject();
    auto bank = job.isEmpty() ? installedBank()
                              : checked(asset("backup.bin"), profile.value("bytes").toInt(),
                                        hashField(job, "beforeSha256"));
    auto sw = bankIdentity(bank, profile);
    if (sw != (job.isEmpty() ? text(state, "software") : text(job, "from")))
        fail("Recovery kit identity mismatch");
    profile["software"] = sw;
    profile["baseline"] = QJsonObject{{"file", "recovery.bin"}, {"sha256", digest(bank)}};
    QFileInfo target(destination);
    auto parent = target.absoluteDir();
    if (target.fileName().isEmpty() || !parent.mkdir(target.fileName()))
        fail("Choose a new recovery kit directory");
    QDir kit(target.absoluteFilePath());
    try {
        fileio::writeFile(local(kit, "recovery.bin"), bank);
        fileio::writeFile(local(kit, "device.json"), QJsonDocument(profile).toJson());
    } catch (...) {
        QFile::remove(kit.filePath("recovery.bin"));
        parent.rmdir(target.fileName());
        throw;
    }
}
void VirtualEcu::exportKamLayout(const QString &newPath) const {
    auto ranges = layout(state.value("profile").toObject()).retain;
    if (ranges.isEmpty() || ranges.size() > 20)
        fail("The PTDIAG KAM configuration needs 1–20 regions");
    QByteArray output = "file data  > KAM Configuration Data\r\n\r\n";
    for (const auto &r : ranges) {
        if (r.length > 65535)
            fail("Region exceeds PTDIAG KAM configuration length");
        output +=
            ": " + QByteArray::number(r.address, 16).toUpper() + "  " + QByteArray::number(r.length) + "\r\n";
    }
    fileio::writeNew(newPath, output);
}
void VirtualEcu::exportKamData(const QString &config, const QString &newPath) const {
    auto p = state.value("profile").toObject();
    p["retain"] = kamRanges(fileio::readFile(config, 256 * 1024));
    // Validate region geometry and overlap, but allow a read-only backup to include the ID field.
    auto bank = installedBank();
    auto base = address(p, "base");
    Image img;
    QByteArray header = "file data  > Specified KAM Regions\r\nfile type  > Intel Hexadecimal-32\r\n";
    int i = 0;
    for (const auto &v : p["retain"].toArray()) {
        auto r = v.toObject();
        auto start = address(r, "address");
        int len = number(r, "bytes", 1, 65535);
        if (start < base || quint64(start) + len > quint64(base) + bank.size())
            fail("KAM range outside virtual flash");
        auto next = img.segments.lower_bound(start);
        if ((next != img.segments.end() && quint64(start) + len > next->first) ||
            (next != img.segments.begin() &&
             quint64(std::prev(next)->first) + std::prev(next)->second.size() > start))
            fail("Overlapping KAM ranges");
        img.segments.emplace(start, bank.mid(start - base, len));
        header += QString("region %1  > address: x%2  length: %3 (decimal)\r\n")
                      .arg(++i, 2, 10, QChar('0'))
                      .arg(start, 8, 16, QChar('0'))
                      .arg(len)
                      .toLatin1();
    }
    img.saveHex(newPath, header + "\r\n");
}
