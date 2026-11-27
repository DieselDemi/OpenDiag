#include "../native/window.h"
#include "../native/monitor.h"
#include "../native/program_dialog.h"
#include <QApplication>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTest>
#include <QElapsedTimer>
#include <QTabWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QFileInfo>
#include <iostream>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    Window window;
    window.show();
    auto *log = window.findChild<QPlainTextEdit *>();
    QPushButton *connect = nullptr, *read = nullptr;
    for (auto *btn : window.findChildren<QPushButton *>()) {
        if (btn->text() == "&Connect")
            connect = btn;
        if (btn->text() == "&Read")
            read = btn;
    }
    auto waitLog = [&](const char *s) {
        QElapsedTimer t;
        t.start();
        while (!log->toPlainText().contains(s) && t.elapsed() < 4000)
            QTest::qWait(10);
        if (!log->toPlainText().contains(s)) {
            std::cerr << log->toPlainText().toStdString();
            return false;
        }
        return true;
    };
    if (!log || !connect || !read || read->isEnabled())
        return 1;
    auto provider = qEnvironmentVariable("OPENDIAG_TEST_PROVIDER");
    if (!provider.isEmpty()) {
        auto *c = window.findChild<QComboBox *>("providerList");
        c->addItem("Qualification provider", provider);
        c->setCurrentIndex(c->count() - 1);
    }
    connect->click();
    if (!waitLog("OK Connected") || !read->isEnabled())
        return 2;
    read->click();
    if (!waitLog("OK GGDS"))
        return 3;
    for (auto *combo : window.findChildren<QComboBox *>()) {
        auto index = combo->findData("module");
        if (index >= 0)
            combo->setCurrentIndex(index);
    }
    read->click();
    if (!waitLog("OK SIM-PCM-001"))
        return 4;
    auto *monitor = static_cast<Monitor *>(window.findChild<QWidget *>("acquisition"));
    auto *tabs = window.findChild<QTabWidget *>();
    tabs->setCurrentWidget(monitor);
    monitor->loadIni(app.arguments().at(1));
    auto *table = monitor->findChild<QTableWidget *>();
    table->item(0, 5)->setText("820");
    table->item(0, 6)->setText("2");
    monitor->findChild<QPushButton *>("startMonitor")->click();
    QElapsedTimer t;
    t.start();
    while (monitor->running() && t.elapsed() < 5000)
        QTest::qWait(10);
    if (monitor->running() || monitor->captured().rows.size() != 4 || table->item(0, 4)->text() != "860" ||
        table->item(1, 2)->text() != "14")
        return 6;
    QTemporaryDir tmp;
    auto capture = tmp.filePath("capture.csv");
    monitor->saveCapture(capture);
    monitor->openCapture(capture);
    auto trace = tmp.filePath("session.log");
    acq::writeFile(trace, log->toPlainText().toUtf8());
    monitor->replaySession(trace);
    if (monitor->captured().rows.front().values.at(0) != 800 ||
        monitor->captured().rows.back().values.at(0) != 860)
        return 7;
    connect->click();
    if (!waitLog("Disconnected") || read->isEnabled())
        return 5;
    auto folder = QFileInfo(app.arguments().at(1)).dir().filePath("programming/");
    auto workspace = tmp.filePath("virtual-module");
    VirtualEcu::create(workspace, folder + "device.json");
    {
        ProgramDialog dialog;
        dialog.openTarget(workspace);
        dialog.prepareJob(folder + "generic.json");
        dialog.show();
        dialog.findChild<QPushButton *>("programStep")
            ->click(); // Close while the virtual ECU is in programming mode.
    }
    if (VirtualEcu(workspace).phase() != "erasing")
        return 8;
    {
        ProgramDialog dialog;
        dialog.openTarget(workspace);
        dialog.show();
        auto *runJob = dialog.findChild<QPushButton *>("programRun");
        runJob->click();
        QElapsedTimer clock;
        clock.start();
        while (runJob->isEnabled() && clock.elapsed() < 5000)
            QTest::qWait(5);
        if (runJob->isEnabled())
            return 9;
    }
    window.selectVirtualTarget(workspace);
    connect->click();
    if (!waitLog("file-backed virtual ECU"))
        return 10;
    t.restart();
    while (!read->isEnabled() && t.elapsed() < 4000)
        QTest::qWait(5);
    read->click();
    if (!waitLog("OK SIM-B"))
        return 11;
    connect->click();
    t.restart();
    while (connect->text() != "&Connect" && t.elapsed() < 4000)
        QTest::qWait(5);
    return connect->text() == "&Connect" ? 0 : 12;
}
