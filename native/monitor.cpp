#include "monitor.h"
#include <QAbstractTableModel>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <cmath>
#include <stdexcept>

namespace {
QString display(double n) { return QString::number(n, 'g', 12); }
class CaptureModel : public QAbstractTableModel {
    const acq::Capture &cap;

  public:
    explicit CaptureModel(const acq::Capture &c) : cap(c) {}
    int rowCount(const QModelIndex &p = {}) const override { return p.isValid() ? 0 : int(cap.rows.size()); }
    int columnCount(const QModelIndex &p = {}) const override {
        return p.isValid() ? 0 : int(cap.names.size() + 1);
    }
    QVariant headerData(int i, Qt::Orientation o, int role) const override {
        if (o == Qt::Horizontal && role == Qt::DisplayRole)
            return i == 0 ? "Milliseconds" : cap.names[i - 1];
        return QAbstractTableModel::headerData(i, o, role);
    }
    QVariant data(const QModelIndex &index, int role) const override {
        if (!index.isValid() || role != Qt::DisplayRole)
            return {};
        auto &s = cap.rows[size_t(index.row())];
        return index.column() ? display(s.values[index.column() - 1]) : QString::number(s.ms);
    }
};
} // namespace
Monitor::Monitor() {
    auto *layout = new QVBoxLayout(this);
    settings = new QWidget;
    auto *controls = new QVBoxLayout(settings);
    controls->setContentsMargins(0, 0, 0, 0);
    auto *files = new QHBoxLayout;
    controls->addLayout(files);
    auto button = [this](QHBoxLayout *row, const QString &name, std::function<void()> action) {
        auto *b = new QPushButton(name);
        row->addWidget(b);
        connect(b, &QPushButton::clicked, this, [this, action] { report(action); });
        return b;
    };
    button(files, "Open configuration…", [this] { chooseConfig(); });
    button(files, "Save configuration…", [this] {
        auto p = QFileDialog::getSaveFileName(this, "Save PTDIAG configuration", {}, "Configuration (*.cfg)");
        if (!p.isEmpty())
            saveConfig(p);
    });
    button(files, "PID list…", [this] { chooseChannels(); });
    button(files, "Add…", [this] { editChannel(true); });
    button(files, "Edit…", [this] { editChannel(false); });
    button(files, "Remove", [this] {
        int r = table->currentRow();
        if (r >= 0) {
            auto c = cfg;
            c.channels.removeAt(r);
            setConfig(c);
        }
    });
    auto *timing = new QHBoxLayout;
    controls->addLayout(timing);
    auto spin = [&](const QString &name, int max, int initial) {
        auto *l = new QLabel(name);
        auto *s = new QSpinBox;
        s->setRange(0, max);
        s->setValue(initial);
        l->setBuddy(s);
        timing->addWidget(l);
        timing->addWidget(s);
        return s;
    };
    interval = spin("Sample interval (ms)", 60000, 0);
    interval->setObjectName("sampleInterval");
    capacity = spin("FIFO samples", 200000, 200000);
    capacity->setMinimum(1);
    capacity->setObjectName("fifoCapacity");
    stopFull = new QCheckBox("Stop when full");
    timing->addWidget(stopFull);
    layout->addWidget(settings);
    table = new QTableWidget(0, 7);
    table->setAccessibleName("Acquisition channels");
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setHorizontalHeaderLabels(
        {"Channel", "Read", "Value", "Min", "Max", "High trigger", "Post samples"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(table);
    auto *actions = new QHBoxLayout;
    layout->addLayout(actions);
    monitor = button(actions, "Start monitor", [this] {
        if (active)
            stop("Stopped");
        else
            start();
    });
    monitor->setObjectName("startMonitor");
    view = button(actions, "View capture…", [this] { showCapture(); });
    save = button(actions, "Save capture…", [this] {
        auto p = QFileDialog::getSaveFileName(this, "Save captured data", "capture.csv");
        if (!p.isEmpty())
            saveCapture(p);
    });
    button(actions, "Open capture…", [this] { chooseCapture(); });
    status = new QLabel;
    status->setWordWrap(true);
    layout->addWidget(status);
    tick = new QTimer(this);
    tick->setSingleShot(true);
    connect(tick, &QTimer::timeout, this, [this] {
        if (active)
            next();
    });
    setConfig(acq::Config::parse(": rpm 1165 p 2 2 u 1.0000\n: vbat 1172 p 1 4 u 1.0000\n"));
    status->setText("Loaded the RPM and battery example from PTDIAG 2293. Set a high trigger and post-sample "
                    "count to stop automatically.");
}
void Monitor::report(const std::function<void()> &action) {
    try {
        action();
    } catch (const std::exception &e) {
        status->setText(QString::fromUtf8(e.what()));
    }
}
void Monitor::refresh() {
    settings->setEnabled(!active);
    monitor->setText(active ? "Stop monitor" : "Start monitor");
    monitor->setEnabled(active || (available && !cfg.channels.isEmpty()));
    view->setEnabled(!active && !capture.rows.empty());
    save->setEnabled(view->isEnabled());
}
void Monitor::setAvailable(bool ready) {
    available = ready;
    refresh();
}
void Monitor::setConfig(acq::Config c) {
    if (active || c.channels.size() > 64)
        throw std::runtime_error("Stop monitoring; select at most 64 channels");
    cfg = std::move(c);
    table->setRowCount(cfg.channels.size());
    for (int r = 0; r < cfg.channels.size(); ++r) {
        auto &c = cfg.channels[r];
        QStringList cells = {c.name + (c.units.isEmpty() ? QString() : " [" + c.units + "]"),
                             c.dmr ? QString("DMR %1").arg(c.address, 8, 16, QChar('0')) : c.command,
                             "—",
                             "—",
                             "—",
                             "",
                             "0"};
        for (int col = 0; col < cells.size(); ++col) {
            auto *item = new QTableWidgetItem(cells[col]);
            if (col < 5)
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            table->setItem(r, col, item);
        }
    }
    refresh();
}
void Monitor::loadConfig(const QString &path) {
    setConfig(acq::Config::parse(acq::readFile(path, 4 << 20)));
    configPath = path;
    status->setText("Loaded " + path);
}
void Monitor::saveConfig(const QString &path) const { acq::writeFile(path, cfg.serialize()); }
void Monitor::loadPidList(const QString &path) {
    catalog = acq::Config::parse(acq::readFile(path, 4 << 20));
    pidPath = path;
}
void Monitor::loadIni(const QString &path) {
    if (active)
        throw std::runtime_error("Stop monitoring before importing settings");
    auto ini = acq::Init::load(path);
    std::optional<acq::Config> incoming, list;
    if (ini.autoConfig)
        incoming = acq::Config::parse(acq::readFile(ini.configPath, 4 << 20));
    if (ini.autoPid)
        list = acq::Config::parse(acq::readFile(ini.pidPath, 4 << 20));
    if (incoming)
        setConfig(std::move(*incoming));
    if (list)
        catalog = std::move(*list);
    configPath = ini.configPath;
    pidPath = ini.pidPath;
    if (ini.interval)
        interval->setValue(*ini.interval);
    status->setText("Imported acquisition settings from " + path +
                    (ini.unhandled.isEmpty() ? QString() : ". Unapplied keys: " + ini.unhandled.join(", ")));
}
void Monitor::chooseIni() {
    report([this] {
        auto p = QFileDialog::getOpenFileName(this, "Import PTDIAG.INI", {},
                                              "INI files (*.ini *.INI);;All files (*)");
        if (!p.isEmpty())
            loadIni(p);
    });
}
void Monitor::chooseConfig() {
    report([this] {
        auto p = QFileDialog::getOpenFileName(this, "Open PTDIAG acquisition configuration", configPath);
        if (!p.isEmpty())
            loadConfig(p);
    });
}
void Monitor::chooseChannels() {
    QDialog dlg(this);
    dlg.setWindowTitle("PID selection");
    auto *v = new QVBoxLayout(&dlg);
    auto *items = new QListWidget;
    items->setSelectionMode(QAbstractItemView::ExtendedSelection);
    v->addWidget(items);
    auto fill = [&] {
        items->clear();
        for (auto &c : catalog.channels)
            items->addItem(c.record);
    };
    fill();
    auto *open = new QPushButton("Load list file…");
    v->addWidget(open);
    connect(open, &QPushButton::clicked, &dlg, [&] {
        auto p = QFileDialog::getOpenFileName(&dlg, "PID list", pidPath);
        if (!p.isEmpty())
            try {
                loadPidList(p);
                fill();
            } catch (const std::exception &e) {
                QMessageBox::warning(&dlg, "PID list", e.what());
            }
    });
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    v->addWidget(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted)
        return;
    auto c = cfg;
    for (auto *i : items->selectedItems())
        c.channels << catalog.channels[items->row(i)];
    if (!c.channels.isEmpty())
        setConfig(acq::Config::parse(c.serialize()));
}
void Monitor::editChannel(bool add) {
    int r = table->currentRow();
    if (!add && r < 0)
        return;
    bool ok = false;
    auto text = QInputDialog::getText(
        this, add ? "Add data item" : "Edit data item",
        "Name  Hex ID  Type  Size  Binary point  Translation  [Scalar Offset PI Formula Units]",
        QLineEdit::Normal, add ? QString() : cfg.channels[r].record, &ok);
    if (!ok)
        return;
    auto c = cfg;
    auto channel = acq::Channel::parse(text);
    if (add)
        c.channels << channel;
    else
        c.channels[r] = channel;
    setConfig(acq::Config::parse(c.serialize()));
}
bool Monitor::confirmDiscard() {
    if (!dirty)
        return true;
    auto choice = QMessageBox::question(this, "Captured data", "Save the current capture before replacing it?",
                                        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                        QMessageBox::Save);
    if (choice == QMessageBox::Discard)
        return true;
    if (choice == QMessageBox::Save) {
        auto p = QFileDialog::getSaveFileName(this, "Save captured data", "capture.csv");
        if (!p.isEmpty())
            report([&] { saveCapture(p); });
        return !dirty;
    }
    return false;
}
void Monitor::start() {
    if (!available || !request || cfg.channels.isEmpty())
        return;
    if (cfg.rapid)
        throw std::runtime_error(
            "Rapid packet mode requires a qualified provider; select a single-rate configuration");
    QList<Trigger> validated;
    QStringList names, reads;
    for (int r = 0; r < cfg.channels.size(); ++r) {
        auto &c = cfg.channels[r];
        if (c.dmr)
            throw std::runtime_error("DMR reads require a memory snapshot. Use Tools → Read DMRs from image");
        names << c.name;
        if (!reads.contains(c.command))
            reads << c.command;
        Trigger t{{}, 0};
        auto text = table->item(r, 5)->text().trimmed();
        if (!text.isEmpty()) {
            bool ok;
            double v = text.toDouble(&ok);
            if (!ok || !std::isfinite(v) || c.encoding == 'b')
                throw std::runtime_error("Invalid high trigger for channel");
            t.high = v;
            t.post = table->item(r, 6)->text().toInt(&ok);
            if (!ok || t.post < 0 || t.post >= capacity->value())
                throw std::runtime_error("Post-trigger count must fit the FIFO");
        }
        validated << t;
    }
    if (!confirmDiscard())
        return;
    triggers = std::move(validated);
    commands = reads;
    remaining = -1;
    capture = {names, {}};
    dirty = false;
    minimum.clear();
    maximum.clear();
    cursor = 0;
    replies.clear();
    active = true;
    elapsed.start();
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    refresh();
    if (stateChanged)
        stateChanged();
    if (event)
        event("ACQ BEGIN " + QString::fromLatin1(cfg.fingerprint()) + ' ' + QString::number(capacity->value()));
    next();
}
void Monitor::next() {
    if (!cursor)
        scan.start();
    request(commands[cursor]);
}
void Monitor::stop(const QString &message) {
    if (!active)
        return;
    tick->stop();
    active = false;
    table->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    status->setText(message + QString(" — %1 complete samples").arg(capture.rows.size()));
    if (event)
        event("ACQ END " + message);
    refresh();
    if (stateChanged)
        stateChanged();
}
void Monitor::result(const QString &line) {
    if (!active)
        return;
    try {
        if (!line.startsWith("OK raw=")) {
            stop(commands[cursor] + ": " + line);
            return;
        }
        auto hex = line.mid(7).toLatin1();
        auto raw = QByteArray::fromHex(hex);
        if (raw.isEmpty() || raw.toHex().toUpper() != hex.toUpper())
            throw std::runtime_error("Malformed acquisition reply");
        replies[commands[cursor]] = raw;
        if (++cursor < commands.size()) {
            tick->start(0);
            return;
        }
        cursor = 0;
        finishScan();
    } catch (const std::exception &e) {
        stop(e.what());
    }
}
void Monitor::finishScan() {
    acq::Sample s{elapsed.elapsed(), {}};
    for (auto &c : cfg.channels)
        s.values << c.translate(replies.value(c.command));
    if (minimum.isEmpty())
        minimum = maximum = s.values;
    int post = -1;
    for (int r = 0; r < s.values.size(); ++r) {
        auto v = s.values[r];
        minimum[r] = std::min(minimum[r], v);
        maximum[r] = std::max(maximum[r], v);
        table->item(r, 2)->setText(display(v));
        table->item(r, 3)->setText(display(minimum[r]));
        table->item(r, 4)->setText(display(maximum[r]));
        if (triggers[r].high && v >= *triggers[r].high && (post < 0 || triggers[r].post < post))
            post = triggers[r].post;
    }
    capture.append(std::move(s), size_t(capacity->value()));
    dirty = true;
    if (event)
        event("ACQ SAMPLE " + QString::number(capture.rows.back().ms));
    if (remaining >= 0)
        --remaining;
    else
        remaining = post;
    if (remaining == 0) {
        stop("Triggered");
        return;
    }
    if (stopFull->isChecked() && capture.rows.size() == size_t(capacity->value())) {
        stop("FIFO full");
        return;
    }
    status->setText(
        QString("%1 samples in FIFO").arg(capture.rows.size()) +
        (remaining > 0 ? QString("; %1 post-trigger samples remaining").arg(remaining) : QString()));
    tick->start(int(std::max<qint64>(0, interval->value() - scan.elapsed())));
}
void Monitor::saveCapture(const QString &path) {
    capture.save(path);
    dirty = false;
    status->setText("Saved capture: " + path);
}
void Monitor::openCapture(const QString &path) {
    if (active)
        throw std::runtime_error("Stop monitoring before opening a capture");
    auto incoming = acq::Capture::load(path);
    if (!confirmDiscard())
        return;
    capture = std::move(incoming);
    dirty = false;
    refresh();
    status->setText("Opened capture: " + path);
}
void Monitor::chooseCapture() {
    report([this] {
        if (active)
            throw std::runtime_error("Stop monitoring before opening a capture");
        auto p = QFileDialog::getOpenFileName(this, "Open captured data");
        if (!p.isEmpty()) {
            openCapture(p);
            if (!capture.rows.empty())
                showCapture();
        }
    });
}
void Monitor::replaySession(const QString &path) {
    if (active)
        throw std::runtime_error("Stop monitoring before replay");
    auto result = acq::Capture::replay(cfg, path);
    if (!confirmDiscard())
        return;
    capture = std::move(result);
    dirty = false;
    refresh();
    status->setText("Replayed " + QString::number(capture.rows.size()) + " samples");
}
void Monitor::chooseReplay() {
    report([this] {
        auto p = QFileDialog::getOpenFileName(this, "Replay OpenDiag session for the loaded configuration");
        if (!p.isEmpty()) {
            replaySession(p);
            if (!capture.rows.empty())
                showCapture();
        }
    });
}
void Monitor::chooseSnapshot() {
    report([this] {
        if (active)
            throw std::runtime_error("Stop the monitor before reading a snapshot");
        auto p = QFileDialog::getOpenFileName(this, "Memory image for loaded DMR configuration");
        if (p.isEmpty())
            return;
        quint32 base = 0;
        if (p.endsWith(".bin", Qt::CaseInsensitive)) {
            bool ok;
            auto s =
                QInputDialog::getText(this, "Binary image", "Base address (hex)", QLineEdit::Normal, "0", &ok);
            if (!ok)
                return;
            base = s.toUInt(&ok, 16);
            if (!ok)
                throw std::runtime_error("Invalid binary base address");
        }
        auto data = acq::Capture::snapshot(cfg, p, base);
        if (!confirmDiscard())
            return;
        capture = std::move(data);
        dirty = true;
        refresh();
        for (int r = 0; r < cfg.channels.size(); ++r)
            for (int col = 2; col <= 4; ++col)
                table->item(r, col)->setText(display(capture.rows[0].values[r]));
        status->setText("Decoded DMRs from " + p);
    });
}
void Monitor::showCapture() {
    QDialog dlg(this);
    dlg.setWindowTitle("Captured data");
    dlg.resize(760, 440);
    auto *layout = new QVBoxLayout(&dlg);
    CaptureModel model(capture);
    auto *grid = new QTableView;
    grid->setModel(&model);
    layout->addWidget(grid);
    dlg.exec();
}
