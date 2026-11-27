#pragma once
#include "virtual_ecu.h"
#include <QDialog>
#include <memory>

class QLabel;
class QPushButton;
class QProgressBar;
class QTimer;

class ProgramDialog : public QDialog {
  public:
    explicit ProgramDialog(QWidget *parent = nullptr);
    void openTarget(const QString &path);
    void prepareJob(const QString &path);
    QString diagnosticTarget() const { return result() == QDialog::Accepted ? workspace : QString(); }

  private:
    std::unique_ptr<VirtualEcu> ecu;
    QString workspace, exampleDir, recipeDir;
    QLabel *target, *detail, *message;
    QProgressBar *progress;
    QPushButton *create, *open, *demo, *prepare, *manual, *automatic, *restore, *voltage, *exportBank,
        *exportBackup, *exportConfig, *exportBlock, *recoveryKit, *identify;
    QTimer *timer;
    void refresh();
    void advance();
    void createTarget(bool example);
};
