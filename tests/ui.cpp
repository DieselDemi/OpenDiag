#include "../native/window.h"
#include <QApplication>
#include <QComboBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTest>
#include <QElapsedTimer>
#include <iostream>

int main(int argc, char **argv) {
    QApplication app(argc,argv);
    Window window; window.show();
    auto *log = window.findChild<QPlainTextEdit *>();
    QPushButton *connect = nullptr, *read = nullptr;
    for (auto *btn : window.findChildren<QPushButton *>()) {
        if (btn->text() == "&Connect") connect = btn;
        if (btn->text() == "&Read") read = btn;
    }
    auto waitLog = [&](const char *s) {
        QElapsedTimer t; t.start();
        while (!log->toPlainText().contains(s) && t.elapsed() < 4000) QTest::qWait(10);
        if (!log->toPlainText().contains(s)) { std::cerr << log->toPlainText().toStdString(); return false; }
        return true;
    };
    if (!log || !connect || !read || read->isEnabled()) return 1;
    connect->click();
    if (!waitLog("OK Connected") || !read->isEnabled()) return 2;
    read->click();
    if (!waitLog("OK GGDS")) return 3;
    for (auto *combo : window.findChildren<QComboBox *>()) {
        auto index = combo->findData("module"); if (index >= 0) combo->setCurrentIndex(index);
    }
    read->click();
    if (!waitLog("OK SIM-PCM-001")) return 4;
    connect->click();
    return waitLog("Disconnected") && !read->isEnabled() ? 0 : 5;
}
