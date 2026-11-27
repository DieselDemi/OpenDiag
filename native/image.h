#pragma once
#include <QByteArray>
#include <QString>
#include <map>

struct Image {
    QString format;
    std::map<quint32, QByteArray> segments;
    QByteArray entryRecord;
    QString header;
    QString summary() const;
    QString page(quint32 segment, qsizetype number) const;
    QByteArray range(quint32 start, qsizetype length) const;
    void saveHex(const QString &path) const;
};
Image readImage(const QString &path, quint32 binaryBase = 0);
struct LookupMatch {
    QString primaryId, primaryPath, sblPath;
};
LookupMatch lookupImage(const QString &table, const QString &moduleId, bool generic);
QString compareCalibration(const QString &backup, quint32 base, const QString &candidate);
