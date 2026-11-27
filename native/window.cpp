#include "window.h"
#include "acquisition.h"
#include "can_monitor.h"
#include "image.h"
#include "monitor.h"
#include "program_dialog.h"
#include "viewer.h"
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QtEndian>

namespace {
QString workerFor(const QString &provider) {
    auto dir = QApplication::applicationDirPath() + '/';
#ifdef Q_OS_WIN
    QFile dll(provider);
    if (dll.open(QIODevice::ReadOnly)) {
        auto h = dll.read(64);
        if (h.size() == 64 && h.startsWith("MZ") && dll.seek(qFromLittleEndian<quint32>(h.constData() + 60))) {
            auto pe = dll.read(6);
            if (pe.size() == 6 && pe.startsWith(QByteArray("PE\0\0", 4)) &&
                qFromLittleEndian<quint16>(pe.constData() + 4) == 0x14c)
                return dir + "opendiag-driver-x86.exe";
        }
    }
    return dir + "opendiag-driver.exe";
#else
    Q_UNUSED(provider);
    return dir + "opendiag-driver";
#endif
}
} // namespace
Window::Window() {
    setWindowTitle("OpenDiag — Pass Thru Diagnostics");
    resize(950, 680);
    auto *layout = new QVBoxLayout(this);
    auto *menus = new QMenuBar;
    layout->setMenuBar(menus);
    auto *file = menus->addMenu("&File"), *view = menus->addMenu("&View"), *setup = menus->addMenu("&Setup"),
         *diagMenu = menus->addMenu("&Diagnostic"), *program = menus->addMenu("&Programming"),
         *tools = menus->addMenu("&Tools"), *utility = menus->addMenu("&Utility"),
         *help = menus->addMenu("&Help");
    auto *bus = new QHBoxLayout;
    layout->addLayout(bus);
    providers = new QComboBox;
    auto *label = new QLabel("&Provider");
    label->setBuddy(providers);
    bus->addWidget(label);
    bus->addWidget(providers, 1);
    providers->setObjectName("providerList");
    providers->addItem("Simulator (no vehicle)", QApplication::applicationDirPath() + "/" OPENDIAG_SIM_FILE);
#ifdef Q_OS_WIN
    for (auto format : {QSettings::Registry64Format, QSettings::Registry32Format}) {
        QSettings reg("HKEY_LOCAL_MACHINE\\SOFTWARE\\PassThruSupport.04.04", format);
        for (const auto &key : reg.childGroups()) {
            auto p = reg.value(key + "/FunctionLibrary").toString();
            if (QFileInfo(p).isAbsolute() && providers->findData(p) < 0)
                providers->addItem(reg.value(key + "/Name", key).toString() +
                                       (format == QSettings::Registry32Format ? " (x86)" : " (x64)"),
                                   p);
        }
    }
#endif
    browse = new QPushButton("Browse…");
    bus->addWidget(browse);
    profiles = new QComboBox;
    profiles->setAccessibleName("Module and network");
    bus->addWidget(profiles);
    profiles->addItem("PCM · 7E0/7E8", QStringList{"7E0", "7E8", "500000"});
    profiles->addItem("TCM · 7E1/7E9", QStringList{"7E1", "7E9", "500000"});
    profiles->addItem("29-bit · 18DA10F1/18DAF110", QStringList{"18DA10F1", "18DAF110", "500000"});
    link = new QPushButton("&Connect");
    bus->addWidget(link);
    tabs = new QTabWidget;
    layout->addWidget(tabs, 3);
    auto *quick = new QWidget;
    auto *quickLayout = new QVBoxLayout(quick);
    tabs->addTab(quick, "Diagnostics");
    op = new QComboBox;
    auto add = [this](const QString &text, const QString &id) { op->addItem(text, id); };
    add("Ping module (D100)", "ping");
    add("Read module software ID (F188)", "module");
    add("Read VIN (F190)", "vin");
    add("Read stored OBD-II DTCs", "dtc");
    add("Read pending OBD-II DTCs", "pending-dtc");
    add("Read permanent OBD-II DTCs", "permanent-dtc");
    add("Read Ford/UDS DTC status records", "ford-dtc");
    add("Read engine RPM", "rpm");
    add("Mode 9 vehicle identification", "obd-vin");
    add("Mode 9 calibration IDs", "calid");
    add("Mode 9 calibration verification numbers", "cvn");
    add("Read generic programming lookup ID (E611)", "lookup-id");
    add("Read adapter supply voltage", "voltage");
    add("Scan supported Mode 1 live PIDs", "scan:01");
    add("Scan supported Mode 2 freeze frame PIDs", "scan:02");
    add("Scan supported Mode 6 monitors (raw)", "scan:06");
    add("Scan supported Mode 9 information", "scan:09");
    auto *opLabel = new QLabel("&Operation");
    opLabel->setBuddy(op);
    quickLayout->addWidget(opLabel);
    quickLayout->addWidget(op);
    run = new QPushButton("&Read");
    quickLayout->addWidget(run);
    sample = new QCheckBox("Sample engine RPM once per second");
    quickLayout->addWidget(sample);
    quickLayout->addStretch();
    monitor = new Monitor;
    monitor->setObjectName("acquisition");
    tabs->addTab(monitor, "Data acquisition");
    log = new QPlainTextEdit;
    log->setReadOnly(true);
    log->setAccessibleName("Session log");
    log->setMaximumBlockCount(10000);
    layout->addWidget(log, 2);
    auto *generalLog = view->addAction("General log");
    generalLog->setCheckable(true);
    generalLog->setChecked(true);
    connect(generalLog, &QAction::toggled, log, &QWidget::setVisible);
    file->addAction("View log file…", this, [this] {
        auto p = QFileDialog::getOpenFileName(this, "View log file");
        if (p.isEmpty())
            return;
        try {
            auto data = acq::readFile(p, 16 << 20);
            QDialog panel(this);
            panel.setWindowTitle(QFileInfo(p).fileName());
            panel.resize(850, 500);
            QVBoxLayout v(&panel);
            QPlainTextEdit contents;
            contents.setReadOnly(true);
            contents.setPlainText(QString::fromUtf8(data));
            v.addWidget(&contents);
            QLineEdit query;
            query.setPlaceholderText("Find text; press Enter for next match");
            v.addWidget(&query);
            connect(&query, &QLineEdit::returnPressed, &panel, [&] {
                if (!contents.find(query.text())) {
                    contents.moveCursor(QTextCursor::Start);
                    contents.find(query.text());
                }
            });
            panel.exec();
        } catch (const std::exception &e) {
            note(e.what());
        }
    });
    file->addAction("Open acquisition configuration…", this, [this] {
        tabs->setCurrentWidget(monitor);
        monitor->chooseConfig();
    });
    file->addAction("Open captured data…", this, [this] {
        tabs->setCurrentWidget(monitor);
        monitor->chooseCapture();
    });
    file->addAction("Replay acquisition from session log…", this, [this] {
        tabs->setCurrentWidget(monitor);
        monitor->chooseReplay();
    });
    file->addAction("Save session log…", this, [this] {
        auto p = QFileDialog::getSaveFileName(this, "Save session log", "OpenDiag.log");
        if (!p.isEmpty())
            try {
                acq::writeFile(p, (log->toPlainText() + '\n').toUtf8());
            } catch (const std::exception &e) {
                note(e.what());
            }
    });
    file->addAction("Exit", this, &QWidget::close);
    setup->addAction("Import PTDIAG.INI…", this, [this] {
        tabs->setCurrentWidget(monitor);
        monitor->chooseIni();
    });
    auto *general = setup->addAction("General configuration…", this, [this] {
        if (state != Offline)
            return;
        auto current = profiles->currentData().toStringList();
        QDialog dlg(this);
        dlg.setWindowTitle("Physical ISO 15765 channel");
        auto *form = new QFormLayout(&dlg);
        QLineEdit tx(current[0]), rx(current[1]);
        QComboBox rate;
        rate.addItems({"125000", "250000", "500000", "1000000"});
        rate.setCurrentText(current[2]);
        form->addRow("Transmit ID (hex)", &tx);
        form->addRow("Receive ID (hex)", &rx);
        form->addRow("Bitrate", &rate);
        QDialogButtonBox bb(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        form->addRow(&bb);
        connect(&bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        connect(&bb, &QDialogButtonBox::accepted, &dlg, [&] {
            try {
                auto l = diag::Link::parse(tx.text().trimmed().toStdString(), rx.text().trimmed().toStdString(),
                                           rate.currentText().toStdString());
                auto pair = QString::fromStdString(l.addresses());
                auto ids = pair.split('/');
                ids << rate.currentText();
                int i = profiles->findData(ids);
                if (i < 0) {
                    profiles->addItem(pair + " · " + rate.currentText(), ids);
                    i = profiles->count() - 1;
                }
                profiles->setCurrentIndex(i);
                dlg.accept();
            } catch (const std::exception &e) {
                QMessageBox::warning(&dlg, "Channel", e.what());
            }
        });
        dlg.exec();
    });
    auto *virtualTarget = setup->addAction("Select virtual ECU workspace…", this, [this] {
        auto p = QFileDialog::getExistingDirectory(this, "Select a programmed virtual ECU");
        if (!p.isEmpty())
            selectVirtualTarget(p);
    });
    connect(setup, &QMenu::aboutToShow, this, [this, general, virtualTarget] {
        general->setEnabled(state == Offline && !providers->currentData(Qt::UserRole + 1).isValid());
        virtualTarget->setEnabled(state == Offline);
    });
    diagMenu->addAction("Data acquisition", this, [this] { tabs->setCurrentWidget(monitor); });
    diagMenu->addAction("Get DTCs", this, [this, quick] {
        tabs->setCurrentWidget(quick);
        op->setCurrentIndex(op->findData("ford-dtc"));
    });
    diagMenu->addAction("Read DTCs from module list…", this, [this] {
        if (providers->currentData(Qt::UserRole + 1).isValid()) {
            note("A file-backed virtual ECU exposes identification and supply voltage only");
            return;
        }
        if (state != Offline) {
            note("Disconnect before starting a multi-module scan");
            return;
        }
        auto p = QFileDialog::getOpenFileName(this, "PTDIAG multi-module configuration");
        if (p.isEmpty())
            return;
        bool ok;
        auto mask = QInputDialog::getText(this, "ISO 14229 DTCs", "Status mask (01–FF hex)", QLineEdit::Normal,
                                          "FF", &ok)
                        .toUpper();
        if (!ok)
            return;
        try {
            diag::operation("ford-dtc:" + mask.toStdString());
        } catch (const std::exception &e) {
            note(e.what());
            return;
        }
        auto provider = providers->currentData().toString();
        note("Reading DTCs: " + p);
        setState(Scanning);
        watchdog->start(10000);
        worker->start(workerFor(provider), {"--dtc-scan", provider, p, mask});
    });
    diagMenu->addAction("OBD-II functions", this, [this, quick] {
        tabs->setCurrentWidget(quick);
        op->setCurrentIndex(op->findData("dtc"));
    });
    tools->addAction("Monitor CAN data…", this, [this] {
        if (providers->currentData(Qt::UserRole + 1).isValid()) {
            note("A file-backed virtual ECU has no broadcast CAN traffic");
            return;
        }
        if (state != Offline) {
            note("Disconnect the diagnostic session to use the raw CAN channel");
            return;
        }
        auto path = providers->currentData().toString();
        auto ids = profiles->currentData().toStringList();
        monitorCan(this, workerFor(path), path, ids[0].toUInt(nullptr, 16) > 0x7ff, ids[2]);
    });
    tools->addAction("Read DMRs from image…", this, [this] {
        tabs->setCurrentWidget(monitor);
        monitor->chooseSnapshot();
    });
    tools->addAction("Read translated table from image…", this, [this] {
        auto path = QFileDialog::getOpenFileName(this, "Select memory image");
        if (!path.isEmpty())
            try {
                viewImage(this, path, true);
            } catch (const std::exception &e) {
                note(e.what());
            }
    });
    program->addAction("Program or recover virtual ECU…", this, [this] {
        QString selected;
        {
            ProgramDialog dialog(this);
            dialog.exec();
            selected = dialog.diagnosticTarget();
        }
        if (!selected.isEmpty())
            selectVirtualTarget(selected);
    });
    program->addAction("Compare calibration with backup…", this, [this] {
        auto backup = QFileDialog::getOpenFileName(this, "Full raw backup", {}, "Binary (*.bin *.BIN)");
        if (backup.isEmpty())
            return;
        auto candidate = QFileDialog::getOpenFileName(this, "Addressed calibration image", {},
                                                      "Calibration (*.hex *.HEX *.h32 *.H32 *.vbf *.VBF)");
        if (candidate.isEmpty())
            return;
        bool ok;
        auto text = QInputDialog::getText(this, "Backup mapping", "Base address of raw backup (hex)",
                                          QLineEdit::Normal, "0", &ok);
        if (!ok)
            return;
        auto base = text.toUInt(&ok, 16);
        if (!ok) {
            note("Invalid backup base address");
            return;
        }
        try {
            note(compareCalibration(backup, base, candidate));
        } catch (const std::exception &e) {
            note(e.what());
        }
    });
    view->addAction("Ford trouble code definitions…", this, [this] {
        if (definitions.uds.isEmpty()) {
            auto p = QFileDialog::getOpenFileName(this, "Select original PTDIAG 2293 to read its definitions",
                                                  {}, "Reference executable (*.exe *.EXE)");
            if (p.isEmpty())
                return;
            try {
                definitions = DtcDefinitions::read(p);
            } catch (const std::exception &e) {
                note(e.what());
                return;
            }
        }
        definitions.show(this);
    });
    for (bool generic : {false, true})
        program->addAction(generic ? "Generic lookup table…" : "PCM lookup table…", this, [=] {
            auto path = QFileDialog::getOpenFileName(this, "Open programming lookup table");
            if (path.isEmpty())
                return;
            bool ok = false;
            auto id = QInputDialog::getText(this, "Offline lookup", "Complete current module ID",
                                            QLineEdit::Normal, {}, &ok);
            if (!ok)
                return;
            try {
                auto m = lookupImage(path, id, generic);
                note("Matched " + id + " → " + m.primaryId + "; image: " + m.primaryPath +
                     (generic ? "; SBL: " + m.sblPath : QString()));
            } catch (const std::exception &e) {
                note(e.what());
            }
        });
    utility->addAction("View HEX, VBF or binary file…", this, [this] {
        auto p = QFileDialog::getOpenFileName(this, "Inspect firmware file", {},
                                              "Images (*.hex *.HEX *.h32 *.H32 *.vbf *.VBF *.bin *.BIN)");
        if (!p.isEmpty())
            try {
                viewImage(this, p);
            } catch (const std::exception &e) {
                note(e.what());
            }
    });
    utility->addAction("MX32 configuration block to VBF…", this, [this] {
        bool ok;
        auto profile = QInputDialog::getItem(this, "MX32 configuration block", "Profile", mxConfigProfiles(), 0,
                                             false, &ok);
        if (!ok)
            return;
        auto input = QFileDialog::getOpenFileName(this, "Open addressed configuration block", {},
                                                  "H32 or HEX (*.h32 *.H32 *.hex *.HEX)");
        if (input.isEmpty())
            return;
        auto output = QFileDialog::getSaveFileName(this, "New VBF file",
                                                   QFileInfo(input).completeBaseName() + ".vbf", "VBF (*.vbf)");
        if (output.isEmpty())
            return;
        try {
            convertMxConfig(profile, input, output);
            note("Created " + output + " (" + profile + ")");
        } catch (const std::exception &e) {
            note(e.what());
        }
    });
    help->addAction("Usage notes", this, [this] {
        QMessageBox::information(this, "OpenDiag",
                                 "Select an adapter and module, then Connect. In Data acquisition, open a "
                                 "PTDIAG configuration or use the RPM/vbat example. Start the monitor, stop or "
                                 "trigger it, then view or save the capture. General configuration selects "
                                 "physical addresses and bitrate; pin switching is provider-specific.");
    });
    connect(browse, &QPushButton::clicked, this, [this] {
        auto p = QFileDialog::getOpenFileName(this, "Choose a native J2534 04.04 provider");
        if (!p.isEmpty()) {
            providers->addItem(p, p);
            providers->setCurrentIndex(providers->count() - 1);
        }
    });
    worker = new QProcess(this);
    watchdog = new QTimer(this);
    watchdog->setSingleShot(true);
    clock = new QTimer(this);
    clock->setInterval(1000);
    connect(clock, &QTimer::timeout, this, [this] {
        if (run->isEnabled())
            run->click();
    });
    connect(sample, &QCheckBox::toggled, this, [this](bool on) {
        if (on) {
            op->setCurrentIndex(op->findData("rpm"));
            clock->start();
        } else
            clock->stop();
        setState(state);
    });
    connect(worker, &QProcess::readyReadStandardOutput, this, &Window::readLines);
    connect(worker, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        readLines();
        watchdog->stop();
        monitor->stop("Disconnected");
        sample->setChecked(false);
        setState(Offline);
        note(code || status == QProcess::CrashExit ? "Provider session ended with an error" : "Disconnected");
    });
    connect(worker, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        note(worker->errorString());
        if (error == QProcess::FailedToStart) {
            watchdog->stop();
            setState(Offline);
        }
    });
    connect(watchdog, &QTimer::timeout, this, [this] {
        note("Provider timed out; closing session");
        monitor->stop("Provider timed out");
        setState(Closing);
        worker->kill();
    });
    connect(link, &QPushButton::clicked, this, [this] {
        if (state == Scanning) {
            setState(Closing);
            worker->kill();
            return;
        }
        if (state != Offline) {
            monitor->stop("Disconnected");
            sample->setChecked(false);
            setState(Closing);
            worker->write("quit\n");
            worker->closeWriteChannel();
            watchdog->start(2000);
            return;
        }
        auto path = providers->currentData().toString();
        setState(Opening);
        note("Opening " + providers->currentText());
        watchdog->start(10000);
        auto directory = providers->currentData(Qt::UserRole + 1).toString();
        worker->start(workerFor(path), directory.isEmpty()
                                           ? QStringList{path} + profiles->currentData().toStringList()
                                           : QStringList{path, "--virtual", directory});
    });
    connect(run, &QPushButton::clicked, this, [this] {
        note(op->currentText());
        send(op->currentData().toString(), QuickRead);
    });
    monitor->request = [this](const QString &cmd) { send(cmd, Acquisition); };
    monitor->event = [this](QString marker) {
        if (marker.startsWith("ACQ BEGIN ")) {
            auto ids = profiles->currentData().toStringList();
            marker += ' ' + ids[0] + '/' + ids[1];
        }
        note(marker);
    };
    monitor->stateChanged = [this] { setState(state); };
    connect(providers, &QComboBox::currentIndexChanged, this, [this] {
        if (providers->currentData(Qt::UserRole + 1).isValid())
            profiles->setCurrentIndex(0);
        setState(state);
    });
    setState(Offline);
}
void Window::selectVirtualTarget(const QString &directory) {
    if (state != Offline) {
        note("Disconnect the active session before selecting a virtual ECU");
        return;
    }
    auto path = QFileInfo(directory).absoluteFilePath();
    int row = providers->findData(path, Qt::UserRole + 1);
    if (row < 0) {
        providers->addItem("Virtual ECU: " + QFileInfo(path).fileName(), providers->itemData(0));
        row = providers->count() - 1;
        providers->setItemData(row, path, Qt::UserRole + 1);
    }
    providers->setCurrentIndex(row);
    op->setCurrentIndex(op->findData("module"));
    tabs->setCurrentIndex(0);
}
void Window::setState(State next) {
    state = next;
    bool ready = state == Ready, online = ready || state == QuickRead || state == Acquisition;
    providers->setEnabled(state == Offline);
    browse->setEnabled(state == Offline);
    profiles->setEnabled(state == Offline && !providers->currentData(Qt::UserRole + 1).isValid());
    link->setText(state == Offline ? "&Connect" : state == Scanning ? "Stop scan" : "&Disconnect");
    link->setEnabled(state == Offline || state == Scanning || online);
    run->setEnabled(ready && !monitor->running());
    op->setEnabled(state != QuickRead && !sample->isChecked() && !monitor->running());
    sample->setEnabled((ready || sample->isChecked()) && !monitor->running());
    monitor->setAvailable(ready && !sample->isChecked());
}
void Window::note(const QString &s) {
    log->appendPlainText(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs) + "  " + s);
}
void Window::send(const QString &cmd, State owner) {
    if (state != Ready) {
        if (owner == Acquisition)
            monitor->stop("Provider is busy");
        return;
    }
    setState(owner);
    watchdog->start(8000);
    worker->write(cmd.toLatin1() + "\n");
}
void Window::readLines() {
    while (worker->canReadLine()) {
        auto line = QString::fromUtf8(worker->readLine()).trimmed();
        note(line);
        if (state == Scanning) {
            watchdog->start(10000);
            continue;
        }
        if (state == QuickRead && line.startsWith("REQUEST "))
            watchdog->start(8000);
        bool ok = line.startsWith("OK ");
        if (!ok && !line.startsWith("ERR "))
            continue;
        if (state == Closing || state == Offline)
            continue;
        watchdog->stop();
        auto old = state;
        if (old == QuickRead && ok && !definitions.uds.empty()) {
            static const QRegularExpression code(
                R"(\b([PCBU][0-3][0-9A-F]{3})\b|\b([0-9A-F]{4})[0-9A-F]{2} status 0x)");
            if (op->currentData().toString().contains("dtc")) {
                auto it = code.globalMatch(line);
                while (it.hasNext()) {
                    auto m = it.next();
                    auto dtc = m.captured(1);
                    if (dtc.isEmpty())
                        dtc = DtcDefinitions::code(m.captured(2).toUShort(nullptr, 16));
                    if (definitions.uds.contains(dtc))
                        note(dtc + ": " + definitions.uds.value(dtc));
                }
            }
        }
        if (old == Opening && !ok) {
            setState(Closing);
            watchdog->start(2000);
            continue;
        }
        setState(Ready);
        if (old == Acquisition)
            monitor->result(line);
        if (!ok)
            sample->setChecked(false);
    }
}
void Window::closeEvent(QCloseEvent *event) {
    monitor->stop("Stopped");
    if (!monitor->confirmDiscard()) {
        event->ignore();
        return;
    }
    shutdownSession();
    event->accept();
}
void Window::shutdownSession() {
    if (worker->state() != QProcess::NotRunning) {
        bool alreadyClosing = state == Closing;
        setState(Closing);
        if (!alreadyClosing) {
            worker->write("quit\n");
            worker->closeWriteChannel();
        }
        if (!worker->waitForFinished(1500)) {
            worker->kill();
            worker->waitForFinished(1000);
        }
    }
}
Window::~Window() {
    // A QProcess destructor can emit finished after the earlier widget children are gone.
    disconnect(worker, nullptr, this, nullptr);
    clock->stop();
    watchdog->stop();
    shutdownSession();
}
