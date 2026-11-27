#include "program_dialog.h"
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QJsonArray>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

namespace {
QString examples() {
    const QDir app(QApplication::applicationDirPath());
    for (const auto &dir :
         {app.filePath("../Resources/examples/programming"),
          app.filePath("../share/opendiag/examples/programming"), app.filePath("../examples/programming"),
          app.filePath("examples/programming"), QDir::current().filePath("reference/examples/programming")})
        if (QFileInfo::exists(QDir(dir).filePath("device.json")))
            return QDir(dir).absolutePath();
    return {};
}
} // namespace
ProgramDialog::ProgramDialog(QWidget *parent) : QDialog(parent), exampleDir(examples()), recipeDir(exampleDir) {
    setWindowTitle("Programming and recovery — virtual ECU");
    resize(730, 400);
    auto *layout = new QVBoxLayout(this);
    auto label = [&](const QString &text) {
        auto *l = new QLabel(text);
        l->setTextFormat(Qt::PlainText);
        l->setWordWrap(true);
        l->setTextInteractionFlags(Qt::TextSelectableByMouse);
        layout->addWidget(l);
        return l;
    };
    label("This programs a local virtual flash bank. It does not use the vehicle adapter.");
    auto row = [&] {
        auto *bar = new QHBoxLayout;
        layout->addLayout(bar);
        return bar;
    };
    auto button = [this](QHBoxLayout *bar, const QString &title, auto action) {
        auto *b = new QPushButton(title);
        bar->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, action] {
            try {
                message->clear();
                action();
            } catch (const std::exception &e) {
                timer->stop();
                message->setText(QString::fromUtf8(e.what()));
            }
            refresh();
        });
        return b;
    };
    auto *select = row();
    create = button(select, "Create from profile…", [this] { createTarget(false); });
    open = button(select, "Open workspace…", [this] {
        auto p = QFileDialog::getExistingDirectory(this, "Open virtual ECU workspace");
        if (!p.isEmpty())
            openTarget(p);
    });
    demo = button(select, "Try PCM example…", [this] { createTarget(true); });
    target = label({});
    detail = label({});
    progress = new QProgressBar;
    layout->addWidget(progress);
    auto *controls = row();
    prepare = button(controls, "Prepare job…", [this] {
        auto p = QFileDialog::getOpenFileName(this, "Select PCM, generic, block or configuration job",
                                              recipeDir, "Job (*.json)");
        if (!p.isEmpty())
            prepareJob(p);
    });
    manual = button(controls, "Single step", [this] { advance(); });
    manual->setObjectName("programStep");
    automatic = button(controls, "Run / resume", [this] {
        if (timer->isActive()) {
            timer->stop();
            message->setText("Paused at the last completed checkpoint.");
        } else
            timer->start();
    });
    automatic->setObjectName("programRun");
    auto *recovery = row();
    restore = button(recovery, "Restore pre-job backup…", [this] {
        if (QMessageBox::question(
                this, "Restore virtual ECU",
                "Replace this virtual flash bank and software ID with the verified pre-job backup?",
                QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes)
            ecu->restore();
    });
    voltage = button(recovery, "Set virtual supply…", [this] {
        bool ok;
        int mv = QInputDialog::getInt(this, "Virtual supply", "Millivolts", ecu->status()["supplyMv"].toInt(),
                                      0, 60000, 100, &ok);
        if (ok)
            ecu->setVoltage(mv);
    });
    recoveryKit = button(recovery, "Save recovery kit…", [this] {
        auto p = QFileDialog::getSaveFileName(this, "Name a NEW directory for the pre-job image and profile",
                                              "RecoveryKit", {}, nullptr, QFileDialog::DontConfirmOverwrite);
        if (!p.isEmpty())
            ecu->exportRecoveryKit(p);
    });
    auto save = [this](bool backup) {
        auto p = QFileDialog::getSaveFileName(this, "Export to a new binary file",
                                              backup ? "backup.bin" : "installed.bin", "Binary (*.bin)");
        if (!p.isEmpty())
            ecu->exportBank(p, backup);
    };
    auto *outputs = row();
    exportBank = button(outputs, "Export installed bank…", [save] { save(false); });
    exportBackup = button(outputs, "Export backup…", [save] { save(true); });
    exportConfig = new QPushButton("Configuration / KAM");
    outputs->addWidget(exportConfig);
    auto *kam = new QMenu(exportConfig);
    exportConfig->setMenu(kam);
    auto fileAction = [this, kam](const QString &title, auto action) {
        kam->addAction(title, this, [this, action] {
            try {
                action();
            } catch (const std::exception &e) {
                message->setText(QString::fromUtf8(e.what()));
            }
        });
    };
    fileAction("Save retained fields as HEX…", [this] {
        auto p =
            QFileDialog::getSaveFileName(this, "Save retained ranges to new Intel HEX", "configuration.hex");
        if (!p.isEmpty())
            ecu->exportConfiguration(p);
    });
    fileAction("Save KAM region layout…", [this] {
        auto p = QFileDialog::getSaveFileName(this, "Save new PTDIAG region file", "KAM.cfg");
        if (!p.isEmpty())
            ecu->exportKamLayout(p);
    });
    fileAction("Back up regions from KAM file…", [this] {
        auto cfg = QFileDialog::getOpenFileName(this, "Select PTDIAG KAM region file", recipeDir);
        if (cfg.isEmpty())
            return;
        auto p = QFileDialog::getSaveFileName(this, "New KAM backup", "KAM.hex");
        if (!p.isEmpty())
            ecu->exportKamData(cfg, p);
    });
    exportBlock = button(outputs, "Read block…", [this] {
        bool ok;
        auto s = QInputDialog::getText(this, "Read virtual flash block", "Address and length, both hexadecimal",
                                       QLineEdit::Normal, {}, &ok);
        if (!ok)
            return;
        auto fields = s.simplified().split(' ');
        bool a, b;
        auto address = fields.value(0).toUInt(&a, 16), length = fields.value(1).toUInt(&b, 16);
        if (fields.size() != 2 || !a || !b)
            throw std::runtime_error("Expected hexadecimal address and length");
        auto p = QFileDialog::getSaveFileName(this, "Export block to new binary file", "block.bin");
        if (!p.isEmpty())
            ecu->exportRange(p, address, length);
    });
    identify = button(row(), "Select this virtual ECU in Diagnostics", [this] { accept(); });
    message = label({});
    message->setObjectName("programMessage");
    layout->addStretch();
    timer = new QTimer(this);
    timer->setInterval(0);
    connect(timer, &QTimer::timeout, this, [this] {
        try {
            advance();
        } catch (const std::exception &e) {
            timer->stop();
            message->setText(QString::fromUtf8(e.what()));
        }
        refresh();
    });
    connect(this, &QDialog::finished, timer, &QTimer::stop);
    refresh();
}
void ProgramDialog::createTarget(bool example) {
    QString profile = example ? QDir(exampleDir).filePath("device.json")
                              : QFileDialog::getOpenFileName(this, "Select virtual ECU profile", recipeDir,
                                                             "Profile (*.json)");
    if (profile.isEmpty())
        return;
    auto p = QFileDialog::getSaveFileName(this, "Name a NEW workspace directory", "Bench.odecu", {}, nullptr,
                                          QFileDialog::DontConfirmOverwrite);
    if (p.isEmpty())
        return;
    VirtualEcu::create(p, profile);
    openTarget(p);
    recipeDir = QFileInfo(profile).absolutePath();
    if (example)
        prepareJob(QDir(recipeDir).filePath("pcm.json"));
}
void ProgramDialog::openTarget(const QString &path) {
    auto absolute = QFileInfo(path).absoluteFilePath();
    if (absolute != workspace) {
        auto next = std::make_unique<VirtualEcu>(absolute);
        ecu = std::move(next);
        workspace = absolute;
    }
    refresh();
}
void ProgramDialog::prepareJob(const QString &path) {
    if (!ecu)
        throw std::runtime_error("Open a virtual ECU first");
    ecu->prepare(path);
    recipeDir = QFileInfo(path).absolutePath();
    refresh();
}
void ProgramDialog::advance() {
    ecu->step();
    if (ecu->finished())
        timer->stop();
}
void ProgramDialog::refresh() {
    auto s = ecu ? ecu->status() : QJsonObject{};
    auto phase = s.value("phase").toString();
    auto p = s.value("profile").toObject(), j = s.value("job").toObject();
    bool running = timer->isActive(), pending = ecu && !ecu->finished();
    create->setEnabled(!running);
    open->setEnabled(!running);
    demo->setEnabled(!running && !exampleDir.isEmpty());
    prepare->setEnabled(ecu && !pending && s["bootable"].toBool());
    manual->setEnabled(pending && !running);
    automatic->setEnabled(pending);
    automatic->setText(running ? "Pause" : "Run / resume");
    voltage->setEnabled(ecu && !running);
    restore->setEnabled(!j.isEmpty() && !running);
    exportBackup->setEnabled(restore->isEnabled());
    exportBank->setEnabled(!running && s["bootable"].toBool());
    recoveryKit->setEnabled(ecu && !running);
    identify->setEnabled(exportBank->isEnabled());
    exportConfig->setEnabled(exportBank->isEnabled());
    exportBlock->setEnabled(exportBank->isEnabled());
    target->setText(ecu ? workspace + "\nPart: " + p["part"].toString() +
                              "    Boot ID: " + s["reportedSoftware"].toString("unavailable") +
                              QString("    Supply: %1 mV").arg(s["supplyMv"].toInt())
                        : "Open or create a virtual ECU to begin.");
    int n = j["sectors"].toInt(), cursor = s["cursor"].toInt();
    progress->setRange(0, 3 + 2 * n);
    int value = phase == "erasing"       ? 1 + cursor
                : phase == "programming" ? 1 + n + cursor
                : phase == "verifying"   ? 1 + 2 * n
                : phase == "resetting"   ? 2 + 2 * n
                : phase == "complete"    ? 3 + 2 * n
                                         : 0;
    progress->setValue(value);
    progress->setFormat(phase + (phase == "erasing" || phase == "programming"
                                     ? QString(" · %1 / %2 sectors").arg(cursor).arg(n)
                                     : QString()));
    QStringList lines;
    if (!j.isEmpty()) {
        lines << j["mode"].toString().toUpper() + ": " + j["from"].toString() + " → " + j["to"].toString() +
                     "    Image: " + j["primary"].toObject()["file"].toString();
        if (j.contains("loader"))
            lines << "RAM loader image: " + j["loader"].toObject()["file"].toString();
        QStringList names;
        for (auto v : p["retain"].toArray())
            names << v.toObject()["name"].toString();
        lines << QString("Changed sectors: %1    Retain: %2    Incoming bytes overridden by retention: %3")
                     .arg(n)
                     .arg(names.isEmpty() ? "none" : names.join(", "))
                     .arg(j["retainedDifferences"].toInt());
        if (j.contains("restoredRanges")) {
            QStringList restored;
            for (auto v : j["restoredRanges"].toArray())
                restored << v.toString();
            lines << "Configuration fields being replaced: " + restored.join(", ");
        }
        lines << "Expected installed SHA-256: " + j["afterSha256"].toString();
    }
    if (s.contains("integrityError"))
        lines << s["integrityError"].toString();
    detail->setText(lines.join('\n'));
}
