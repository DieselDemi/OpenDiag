#pragma once
#include <QDir>
#include <QJsonObject>
#include <QLockFile>
#include <QVector>

// A file-backed flash bank. This target never loads a provider or sends vehicle messages.
class VirtualEcu {
  public:
    static void create(const QString &directory, const QString &profile);
    explicit VirtualEcu(const QString &directory);
    QJsonObject status() const;
    void prepare(const QString &recipe);
    void step();
    void restore();
    void setVoltage(int mv);
    void exportBank(const QString &newPath, bool backup = false) const;
    void exportRange(const QString &newPath, quint32 address, quint32 bytes) const;
    void exportConfiguration(const QString &newPath, const QString &name = {}) const;
    void exportRecoveryKit(const QString &newDirectory) const;
    void exportKamLayout(const QString &newPath) const;
    void exportKamData(const QString &config, const QString &newPath) const;
    bool finished() const;
    QString phase() const;

  private:
    QDir root;
    QLockFile lock;
    QJsonObject state;
    QByteArray desired, ram;
    QVector<int> sectors;
    QString asset(const QString &name) const;
    void commit(QJsonObject next);
    void checkVoltage(const QJsonObject &job) const;
    void loadJob();
    void restoreBank();
    QByteArray installedBank() const;
};
