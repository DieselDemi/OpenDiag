#pragma once
#include "acquisition.h"
#include <QElapsedTimer>
#include <QWidget>
#include <functional>

class QCheckBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTableWidget;
class QTimer;
class Monitor : public QWidget {
  public:
    Monitor();
    std::function<void(const QString &)> request;
    std::function<void(const QString &)> event;
    std::function<void()> stateChanged;
    bool running() const { return active; }
    bool confirmDiscard();
    void setAvailable(bool ready);
    void result(const QString &line);
    void stop(const QString &message);
    void loadConfig(const QString &path);
    void loadIni(const QString &path);
    void loadPidList(const QString &path);
    void saveConfig(const QString &path) const;
    void saveCapture(const QString &path);
    void openCapture(const QString &path);
    const acq::Capture &captured() const { return capture; }
    void chooseIni();
    void chooseConfig();
    void chooseCapture();
    void replaySession(const QString &path);
    void chooseReplay();
    void chooseSnapshot();

  private:
    void showCapture();
    void setConfig(acq::Config c);
    void chooseChannels();
    void editChannel(bool add);
    void start();
    void next();
    void finishScan();
    void refresh();
    void report(const std::function<void()> &action);
    bool active = false, available = false, dirty = false;
    int cursor = 0, remaining = -1;
    struct Trigger {
        std::optional<double> high;
        int post;
    };
    QList<Trigger> triggers;
    QList<double> minimum, maximum;
    QStringList commands;
    QHash<QString, QByteArray> replies;
    QString configPath, pidPath;
    acq::Config cfg, catalog;
    acq::Capture capture;
    QElapsedTimer elapsed, scan;
    QTimer *tick;
    QTableWidget *table;
    QLabel *status;
    QWidget *settings;
    QSpinBox *interval, *capacity;
    QCheckBox *stopFull;
    QPushButton *monitor, *view, *save;
};
