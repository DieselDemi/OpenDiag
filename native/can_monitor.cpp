#include "can_monitor.h"
#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSaveFile>
#include <QTimer>
#include <QVBoxLayout>
#include <deque>
#include <algorithm>

namespace {
struct Frame {
    quint32 us, id;
    QByteArray idText, data;
};
bool parse(const QByteArray &timestamp, const QByteArray &ident, const QByteArray &data, Frame &f) {
    bool t, i;
    f.us = timestamp.toUInt(&t);
    f.id = ident.toUInt(&i, 16);
    if (!t || !i || (ident.size() != 3 && ident.size() != 8) ||
        f.id > (ident.size() == 3 ? 0x7ffu : 0x1fffffffu) || data.size() > 16 ||
        QByteArray::fromHex(data).toHex().toUpper() != data.toUpper())
        return false;
    f.idText = ident.toUpper();
    f.data = data.toUpper();
    return true;
}
} // namespace
void monitorCan(QWidget *parent, const QString &program, const QString &provider, bool extended,
                const QString &baud) {
    QDialog dlg(parent);
    dlg.setWindowTitle("Monitor CAN data");
    dlg.resize(760, 510);
    auto *layout = new QVBoxLayout(&dlg);
    auto *bar = new QHBoxLayout;
    layout->addLayout(bar);
    QPushButton run("Start"), save("Save all…"), load("Open capture…");
    bar->addWidget(&run);
    bar->addWidget(&save);
    bar->addWidget(&load);
    QComboBox filter;
    filter.addItems({"All IDs", "7xx only", "6xx / 7xx", "Exact ID"});
    bar->addWidget(&filter);
    QLineEdit id;
    id.setPlaceholderText("Hex ID");
    id.setEnabled(false);
    bar->addWidget(&id);
    QLabel hint("Timestamp (adapter µs)       CAN ID       Data (hex) — latest 1000 matching frames");
    layout->addWidget(&hint);
    QPlainTextEdit display;
    display.setReadOnly(true);
    display.setMaximumBlockCount(1000);
    layout->addWidget(&display);
    QLabel status;
    layout->addWidget(&status);
    std::deque<Frame> frames;
    quint64 total = 0;
    bool stopping = false, dirty = false;
    QProcess worker;
    QTimer poll, guard;
    poll.setSingleShot(true);
    guard.setSingleShot(true);
    auto match = [&](const Frame &f) {
        switch (filter.currentIndex()) {
        case 1:
            return f.id >= 0x700 && f.id <= 0x7ff;
        case 2:
            return f.id >= 0x600 && f.id <= 0x7ff;
        case 3: {
            bool ok;
            return id.text().toUInt(&ok, 16) == f.id && ok;
        }
        default:
            return true;
        }
    };
    auto line = [](const Frame &f) {
        return QString("%1   %2   %3")
            .arg(f.us, 10)
            .arg(QString::fromLatin1(f.idText), 8)
            .arg(QString::fromLatin1(QByteArray::fromHex(f.data).toHex(' ').toUpper()));
    };
    auto refresh = [&] {
        id.setEnabled(filter.currentIndex() == 3);
        QStringList lines;
        for (auto it = frames.rbegin(); it != frames.rend() && lines.size() < 1000; ++it)
            if (match(*it))
                lines << line(*it);
        std::reverse(lines.begin(), lines.end());
        display.setPlainText(lines.join('\n'));
    };
    QObject::connect(&filter, &QComboBox::currentIndexChanged, &dlg, refresh);
    QObject::connect(&id, &QLineEdit::textChanged, &dlg, refresh);
    auto persist = [&]() {
        auto path = QFileDialog::getSaveFileName(&dlg, "Save all retained CAN frames", "can.csv");
        if (path.isEmpty())
            return false;
        QSaveFile out(path);
        if (!out.open(QIODevice::WriteOnly)) {
            status.setText(out.errorString());
            return false;
        }
        bool ok = out.write("timestamp_us,id,data\n") == 21;
        for (auto &f : frames) {
            auto row = QByteArray::number(f.us) + ',' + f.idText + ',' + f.data + '\n';
            if (out.write(row) != row.size()) {
                ok = false;
                break;
            }
        }
        if (!ok || !out.commit()) {
            status.setText(out.errorString());
            return false;
        }
        dirty = false;
        return true;
    };
    auto discard = [&] {
        if (!dirty)
            return true;
        auto c = QMessageBox::question(&dlg, "CAN capture", "Save captured CAN frames?",
                                       QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        return c == QMessageBox::Discard || (c == QMessageBox::Save && persist());
    };
    QObject::connect(&save, &QPushButton::clicked, &dlg, persist);
    QObject::connect(&load, &QPushButton::clicked, &dlg, [&] {
        auto path = QFileDialog::getOpenFileName(&dlg, "Open CAN capture");
        if (path.isEmpty() || !discard())
            return;
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly) || f.size() > 8 * 1024 * 1024 ||
            f.readLine(64).trimmed() != "timestamp_us,id,data") {
            status.setText("Expected OpenDiag CAN CSV");
            return;
        }
        std::deque<Frame> input;
        while (!f.atEnd()) {
            auto fields = f.readLine(128).trimmed().split(',');
            Frame item;
            if (f.error() || fields.size() != 3 || !parse(fields[0], fields[1], fields[2], item) ||
                input.size() >= 100000) {
                status.setText("Invalid CAN capture row");
                return;
            }
            input.push_back(item);
        }
        frames.swap(input);
        total = frames.size();
        dirty = false;
        save.setEnabled(!frames.empty());
        refresh();
        status.setText(QString("Loaded %1 frames").arg(total));
    });
    auto idle = [&] {
        poll.stop();
        guard.stop();
        run.setText("Start");
        run.setEnabled(true);
        load.setEnabled(true);
        save.setEnabled(!frames.empty());
    };
    idle();
    QObject::connect(&worker, &QProcess::finished, &dlg, [&] { idle(); });
    QObject::connect(&worker, &QProcess::errorOccurred, &dlg, [&] {
        status.setText(worker.errorString());
        if (worker.state() == QProcess::NotRunning)
            idle();
    });
    QObject::connect(&guard, &QTimer::timeout, &dlg, [&] {
        status.setText("CAN provider timed out");
        worker.kill();
    });
    QObject::connect(&poll, &QTimer::timeout, &dlg, [&] {
        if (!stopping) {
            worker.write("poll\n");
            guard.start(3000);
        }
    });
    QObject::connect(&worker, &QProcess::readyReadStandardOutput, &dlg, [&] {
        QStringList lines;
        while (worker.canReadLine()) {
            auto raw = worker.readLine();
            while (raw.endsWith('\r') || raw.endsWith('\n'))
                raw.chop(1);
            if (raw.startsWith("FRAME ")) {
                auto f = raw.split(' ');
                Frame item;
                if (f.size() != 4 || !parse(f[1], f[2], f[3], item)) {
                    status.setText("Malformed CAN frame");
                    worker.kill();
                    return;
                }
                if (frames.size() == 100000)
                    frames.pop_front();
                frames.push_back(item);
                ++total;
                dirty = true;
                if (match(item))
                    lines << line(item);
            } else if (raw.startsWith("OK ") && !stopping) {
                guard.stop();
                run.setEnabled(true);
                poll.start(10);
                status.setText(QString("%1 received; %2 retained").arg(total).arg(frames.size()));
            } else if (raw.startsWith("ERR "))
                status.setText(QString::fromUtf8(raw));
        }
        if (!lines.isEmpty())
            display.appendPlainText(lines.join('\n'));
    });
    QObject::connect(&run, &QPushButton::clicked, &dlg, [&] {
        if (worker.state() == QProcess::NotRunning) {
            if (!discard())
                return;
            frames.clear();
            total = 0;
            dirty = false;
            stopping = false;
            refresh();
            run.setText("Stop");
            run.setEnabled(false);
            load.setEnabled(false);
            save.setEnabled(false);
            guard.start(10000);
            worker.start(program, {provider, "--can", extended ? "29" : "11", baud});
        } else {
            stopping = true;
            poll.stop();
            run.setEnabled(false);
            worker.write("quit\n");
            worker.closeWriteChannel();
            guard.start(2000);
        }
    });
    // A rejected save returns to the same capture, with the hardware channel closed.
    do {
        dlg.exec();
        stopping = true;
        poll.stop();
        guard.stop();
        if (worker.state() != QProcess::NotRunning) {
            worker.write("quit\n");
            worker.closeWriteChannel();
            if (!worker.waitForFinished(1000)) {
                worker.kill();
                worker.waitForFinished(500);
            }
        }
    } while (!discard());
}
