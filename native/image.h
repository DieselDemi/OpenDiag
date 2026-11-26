#pragma once
#include <QByteArray>
#include <QString>
#include <map>

struct Image {
    QString format;
    std::map<quint32, QByteArray> segments;
    QByteArray entryRecord;
    QString summary() const;
    void saveHex(const QString &path) const;
};
Image readImage(const QString &path, quint32 binaryBase = 0);
