#pragma once
class QString;
class QWidget;
void monitorCan(QWidget *parent, const QString &worker, const QString &provider, bool extended,
                const QString &baud);
