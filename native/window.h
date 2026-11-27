#pragma once
#include <QWidget>
#include "diagnostics.h"
#include "definitions.h"

class QComboBox;
class QPushButton;
class QCheckBox;
class QTabWidget;
class QPlainTextEdit;
class QProcess;
class QTimer;
class Monitor;
class QLabel;

class Window : public QWidget {
  public:
    Window();
    ~Window() override;

  protected:
    void closeEvent(QCloseEvent *event) override;

  private:
    enum State { Offline, Opening, Ready, QuickRead, Acquisition, Scanning, Closing } state = Offline;
    QProcess *worker;
    QTimer *watchdog, *clock;
    QComboBox *providers, *profiles, *op;
    QPushButton *link, *browse, *run;
    QCheckBox *sample;
    QPlainTextEdit *log;
    QTabWidget *tabs;
    Monitor *monitor;
    QLabel *connectionStatus;
    DtcDefinitions definitions;
    void setState(State next);
    void send(const QString &command, State owner);
    void note(const QString &s);
    void readLines();
    void shutdownSession();
};
