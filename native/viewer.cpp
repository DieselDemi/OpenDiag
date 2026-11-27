#include "viewer.h"
#include "image.h"
#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

void viewImage(QWidget *parent, const QString &path) {
    const auto img = readImage(path);
    QDialog dlg(parent);
    dlg.setWindowTitle(QFileInfo(path).fileName());
    dlg.resize(780, 480);
    auto *layout = new QVBoxLayout(&dlg);
    layout->addWidget(new QLabel(img.summary()));
    auto *bar = new QHBoxLayout;
    layout->addLayout(bar);
    auto *regions = new QComboBox;
    bar->addWidget(regions);
    for (auto &[addr, bytes] : img.segments)
        regions->addItem(QString("0x%1 — %2 bytes").arg(addr, 8, 16, QChar('0')).arg(bytes.size()), addr);
    auto *page = new QSpinBox;
    page->setPrefix("Page ");
    bar->addWidget(page);
    auto *tabs = new QTabWidget;
    layout->addWidget(tabs);
    auto *text = new QPlainTextEdit;
    text->setReadOnly(true);
    text->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    tabs->addTab(text, "Data");
    if (!img.header.isEmpty()) {
        auto *h = new QPlainTextEdit(img.header);
        h->setReadOnly(true);
        tabs->addTab(h, "VBF header");
    }
    auto update = [&] {
        auto base = regions->currentData().toUInt();
        text->setPlainText(img.page(base, page->value() - 1));
    };
    auto range = [&] {
        page->setRange(1, int((img.segments.at(regions->currentData().toUInt()).size() + 255) / 256));
        page->setValue(1);
        update();
    };
    QObject::connect(page, &QSpinBox::valueChanged, &dlg, update);
    QObject::connect(regions, &QComboBox::currentIndexChanged, &dlg, range);
    range();
    auto *save = new QPushButton("Export as Intel HEX…");
    bar->addWidget(save);
    QObject::connect(save, &QPushButton::clicked, &dlg, [&] {
        auto p = QFileDialog::getSaveFileName(&dlg, "Export to new file", {}, "Intel HEX (*.hex)");
        if (p.isEmpty())
            return;
        try {
            img.saveHex(p);
        } catch (const std::exception &e) {
            QMessageBox::warning(&dlg, "HEX export", e.what());
        }
    });
    dlg.exec();
}
