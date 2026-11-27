#pragma once
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <map>

struct Image {
    QString format;
    std::map<quint32, QByteArray> segments;
    QByteArray entryRecord;
    QString header;
    QString summary() const;
    QString page(quint32 segment, qsizetype number) const;
    QByteArray range(quint32 start, qsizetype length) const;
    QString table(quint32 start, int rows, int columns, const QString &cell, const QString &order,
                  const QString &delimiter) const;
    void saveHex(const QString &path, const QByteArray &preamble = {}) const;
};
Image readImage(const QString &path, quint32 binaryBase = 0, QByteArray *sourceSha256 = nullptr);
struct LookupMatch {
    QString primaryId, primaryPath, sblPath;
};
LookupMatch lookupImage(const QString &table, const QString &moduleId, bool generic,
                        QByteArray *sourceSha256 = nullptr);
QString compareCalibration(const QString &backup, quint32 base, const QString &candidate);
QStringList mxConfigProfiles();
void convertMxConfig(const QString &profile, const QString &source, const QString &newVbf,
                     const QString &part = "NA");
