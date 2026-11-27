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

class Window : public QWidget {
  public:
    Window();

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
    DtcDefinitions definitions;
    void setState(State next);
    void send(const QString &command, State owner);
    void note(const QString &s);
    void readLines();
};
