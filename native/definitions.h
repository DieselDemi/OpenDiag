#pragma once
#include <QMap>
#include <QStringList>
class QWidget;
struct DtcDefinitions {
    QMultiMap<QString, QString> kwp, uds;
    static DtcDefinitions read(const QString &reference);
    static QString code(quint16 encoded);
    void show(QWidget *parent) const;
};
