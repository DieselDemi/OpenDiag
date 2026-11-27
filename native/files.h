#pragma once
#include <QByteArray>
#include <QString>

namespace fileio {
QByteArray readFile(const QString &path, qsizetype limit);
void writeFile(const QString &path, const QByteArray &bytes);
void writeNew(const QString &path, const QByteArray &bytes);
} // namespace fileio
