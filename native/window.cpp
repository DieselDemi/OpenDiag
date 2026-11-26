#include "window.h"
#include "image.h"
#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>
#include <QtEndian>

namespace {
QString workerFor(const QString &provider) {
    auto dir = QCoreApplication::applicationDirPath() + '/';
#ifdef Q_OS_WIN
    QFile dll(provider);
    if (dll.open(QIODevice::ReadOnly)) {
        auto header = dll.read(64);
        if (header.size() == 64 && header.startsWith("MZ") && dll.seek(qFromLittleEndian<quint32>(header.constData()+60))) {
            auto pe = dll.read(6);
            if (pe.size() == 6 && pe.startsWith(QByteArray("PE\0\0",4)) && qFromLittleEndian<quint16>(pe.constData()+4) == 0x14c)
                return dir + "opendiag-driver-x86.exe";
        }
    }
    return dir + "opendiag-driver.exe";
#else
    Q_UNUSED(provider);
    return dir + "opendiag-driver";
#endif
}
}

Window::Window() {
    setWindowTitle("OpenDiag"); resize(750,470);
    auto *layout = new QVBoxLayout(this);
    auto *providers = new QComboBox;
    auto *label = new QLabel("&J2534 provider"); label->setBuddy(providers);
    layout->addWidget(label); layout->addWidget(providers);
    providers->addItem("Simulator (no vehicle)", QApplication::applicationDirPath() + "/" OPENDIAG_SIM_FILE);
#ifdef Q_OS_WIN
    for (auto format : {QSettings::Registry64Format, QSettings::Registry32Format}) {
        QSettings reg("HKEY_LOCAL_MACHINE\\SOFTWARE\\PassThruSupport.04.04", format);
        for (const auto &key : reg.childGroups()) {
            const auto path = reg.value(key+"/FunctionLibrary").toString();
            if (QFileInfo(path).isAbsolute() && providers->findData(path) < 0)
                providers->addItem(reg.value(key+"/Name", key).toString() + (format == QSettings::Registry32Format ? " (x86)" : " (x64)"), path);
        }
    }
#endif
    auto *row = new QHBoxLayout; layout->addLayout(row);
    auto *browse = new QPushButton("Browse for &driver…"); row->addWidget(browse);
    auto *link = new QPushButton("&Connect"); row->addWidget(link);
    auto *op = new QComboBox;
    op->addItem("Ping module (D100)", "ping"); op->addItem("Read module software ID (F188)", "module");
    op->addItem("Read VIN (F190)", "vin"); op->addItem("Read stored OBD-II DTCs", "dtc"); op->addItem("Read engine RPM", "rpm");
    auto *opLabel = new QLabel("&PCM operation — 11-bit ISO 15765, 500 kbit/s"); opLabel->setBuddy(op);
    layout->addWidget(opLabel); layout->addWidget(op);
    auto *run = new QPushButton("&Read"); layout->addWidget(run); run->setEnabled(false);
    auto *sample = new QCheckBox("Sample engine RPM once per second"); layout->addWidget(sample);
    sample->setEnabled(false);
    auto *log = new QPlainTextEdit; log->setReadOnly(true); log->setAccessibleName("Session log");
    log->setMaximumBlockCount(10000); layout->addWidget(log);
    auto *files = new QHBoxLayout; layout->addLayout(files);
    auto *inspect = new QPushButton("&Inspect image…"); files->addWidget(inspect);
    auto *save = new QPushButton("&Save session log…"); files->addWidget(save);
    auto note = [log](const QString &s) { log->appendPlainText(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs) + "  " + s); };
    connect(inspect,&QPushButton::clicked,this,[=] {
        auto path = QFileDialog::getOpenFileName(this,"Inspect firmware file",{},"Images (*.hex *.HEX *.h32 *.H32 *.vbf *.VBF *.bin *.BIN)");
        if (path.isEmpty()) return;
        try { note(QFileInfo(path).fileName()+": "+readImage(path).summary()); }
        catch (const std::exception &e) { note(e.what()); }
    });
    connect(save,&QPushButton::clicked,this,[=] {
        auto path = QFileDialog::getSaveFileName(this,"Save session log", "OpenDiag.log");
        if (path.isEmpty()) return;
        QSaveFile out(path); auto data = (log->toPlainText()+'\n').toUtf8();
        if (!out.open(QIODevice::WriteOnly) || out.write(data) != data.size() || !out.commit()) note(out.errorString());
    });
    connect(browse,&QPushButton::clicked,this,[=] {
        auto path = QFileDialog::getOpenFileName(this,"Choose a native J2534 04.04 provider");
        if (!path.isEmpty()) { providers->addItem(path,path); providers->setCurrentIndex(providers->count()-1); }
    });
    auto *worker = new QProcess(this);
    auto *timer = new QTimer(this); timer->setSingleShot(true);
    auto *clock = new QTimer(this); clock->setInterval(1000);
    connect(sample,&QCheckBox::toggled,this,[=](bool on) {
        op->setEnabled(!on);
        if (on) { op->setCurrentIndex(op->findData("rpm")); clock->start(); } else clock->stop();
    });
    connect(clock,&QTimer::timeout,this,[=] { if (run->isEnabled()) run->click(); });
    auto idle = [=] {
        timer->stop(); run->setEnabled(false); link->setText("&Connect"); link->setEnabled(true);
        browse->setEnabled(true); providers->setEnabled(true);
        sample->setChecked(false); sample->setEnabled(false);
    };
    connect(worker,&QProcess::finished,this,[=](int code,QProcess::ExitStatus status) {
        note(code || status == QProcess::CrashExit ? "Provider session ended with an error" : "Disconnected"); idle();
    });
    connect(worker,&QProcess::errorOccurred,this,[=](QProcess::ProcessError error) {
        note(worker->errorString()); if (error == QProcess::FailedToStart) idle();
    });
    connect(timer,&QTimer::timeout,this,[=] { note("Provider timed out; closing session"); worker->kill(); });
    connect(worker,&QProcess::readyReadStandardOutput,this,[=] {
        while (worker->canReadLine()) {
            auto line = QString::fromUtf8(worker->readLine()).trimmed();
            note(line);
            if (!line.startsWith("OK ") && !line.startsWith("ERR ")) continue;
            if (line.startsWith("ERR ")) sample->setChecked(false);
            timer->stop(); run->setEnabled(true); link->setEnabled(true); sample->setEnabled(true);
        }
    });
    connect(link,&QPushButton::clicked,this,[=] {
        run->setEnabled(false); link->setEnabled(false);
        if (worker->state() != QProcess::NotRunning) {
            sample->setChecked(false); sample->setEnabled(false);
            worker->write("quit\n"); worker->closeWriteChannel(); timer->start(2000); return;
        }
        auto path = providers->currentData().toString();
        providers->setEnabled(false); browse->setEnabled(false); link->setText("&Disconnect");
        note("Opening " + providers->currentText()); timer->start(10000);
        worker->start(workerFor(path),{path});
    });
    connect(run,&QPushButton::clicked,this,[=] {
        run->setEnabled(false); link->setEnabled(false); note(op->currentText()); timer->start(8000);
        worker->write(op->currentData().toByteArray()+"\n");
    });
    connect(qApp,&QCoreApplication::aboutToQuit,this,[worker] {
        if (worker->state() == QProcess::Running) {
            worker->write("quit\n"); worker->closeWriteChannel();
            if (!worker->waitForFinished(1500)) { worker->kill(); worker->waitForFinished(1000); }
        }
    });
}
