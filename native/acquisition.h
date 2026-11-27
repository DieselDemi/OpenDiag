#pragma once
#include <QByteArray>
#include <QList>
#include <QStringList>
#include <deque>
#include <optional>

namespace acq {
QByteArray readFile(const QString &path, qsizetype limit);
void writeFile(const QString &path, const QByteArray &bytes);
struct Channel {
    QString name, command, record, formula, units;
    int size, point, bit, index;
    char encoding;
    bool packet = false;
    bool dmr = false;
    quint32 address = 0;
    double scalar, offset;
    double translate(QByteArrayView bytes) const;
    static Channel parse(const QString &fields);
};
struct Config {
    QList<Channel> channels;
    bool intel = false, rapid = false;
    QByteArray source;
    static Config parse(const QByteArray &data);
    QByteArray serialize() const;
    QByteArray fingerprint() const;
};
struct Init {
    std::optional<int> interval;
    QString configPath, pidPath;
    bool autoConfig = false, autoPid = false;
    QStringList unhandled;
    static Init load(const QString &path);
};
struct Sample {
    qint64 ms;
    QList<double> values;
};
struct Capture {
    QStringList names;
    std::deque<Sample> rows;
    void append(Sample sample, size_t capacity);
    void save(const QString &path) const;
    static Capture load(const QString &path);
    static Capture replay(const Config &config, const QString &path);
    static Capture snapshot(const Config &config, const QString &path, quint32 binaryBase);
};
} // namespace acq
