#include "files.h"
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <stdexcept>

namespace fileio {
QByteArray readFile(const QString &path, qsizetype limit) {
    if (!QFileInfo(path).isFile())
        throw std::runtime_error(("Expected a regular file: " + path).toStdString());
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        throw std::runtime_error(f.errorString().toStdString());
    auto bytes = f.read(limit + 1);
    if (f.error() || bytes.size() > limit)
        throw std::runtime_error(("Unreadable or oversized file: " + path).toStdString());
    return bytes;
}
void writeFile(const QString &path, const QByteArray &bytes) {
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size() || !f.commit())
        throw std::runtime_error(f.errorString().toStdString());
}
void writeNew(const QString &path, const QByteArray &bytes) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::NewOnly))
        throw std::runtime_error(f.errorString().toStdString());
    if (f.write(bytes) != bytes.size() || !f.flush()) {
        auto error = f.errorString();
        f.remove();
        throw std::runtime_error(error.toStdString());
    }
}
} // namespace fileio
