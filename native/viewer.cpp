#include "viewer.h"
#include "image.h"
#include "files.h"
#include <QComboBox>
#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QLabel>
#include <QLineEdit>
#include <QInputDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>

void viewImage(QWidget *parent, const QString &path, bool translated) {
    quint32 base = 0;
    if (translated && path.endsWith(".bin", Qt::CaseInsensitive)) {
        bool ok;
        auto s =
            QInputDialog::getText(parent, "Binary mapping", "Base address (hex)", QLineEdit::Normal, "0", &ok);
        if (!ok)
            return;
        base = s.toUInt(&ok, 16);
        if (!ok)
            throw std::runtime_error("Invalid image base address");
    }
    const auto img = readImage(path, base);
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
        tabs->addTab(h, "Header");
    }
    auto *tab = new QWidget;
    auto *tableLayout = new QVBoxLayout(tab);
    auto *parameters = new QHBoxLayout;
    tableLayout->addLayout(parameters);
    auto *start = new QLineEdit(QString::number(img.segments.begin()->first, 16));
    parameters->addWidget(new QLabel("Start (hex)"));
    parameters->addWidget(start);
    auto spin = [&](const QString &label, int max) {
        auto *w = new QSpinBox;
        w->setRange(1, max);
        parameters->addWidget(new QLabel(label));
        parameters->addWidget(w);
        return w;
    };
    auto *rows = spin("Rows", 65535), *columns = spin("Columns", 40);
    auto choice = [&](const QStringList &items) {
        auto *w = new QComboBox;
        w->addItems(items);
        parameters->addWidget(w);
        return w;
    };
    auto *cell = choice({"u8", "s8", "u16", "s16", "u32", "s32", "f32"}),
         *order = choice({"motorola", "intel"}), *delimiter = choice({"csv", "fixed"});
    auto *tableText = new QPlainTextEdit;
    tableText->setReadOnly(true);
    tableText->setFont(text->font());
    tableLayout->addWidget(tableText);
    auto *actions = new QHBoxLayout;
    tableLayout->addLayout(actions);
    auto *read = new QPushButton("Read specified table"),
         *exportTable = new QPushButton("Export displayed table…");
    actions->addWidget(read);
    actions->addWidget(exportTable);
    exportTable->setEnabled(false);
    QObject::connect(read, &QPushButton::clicked, &dlg, [&] {
        try {
            bool ok;
            auto addr = start->text().toUInt(&ok, 16);
            if (!ok)
                throw std::runtime_error("Invalid start address");
            tableText->setPlainText(img.table(addr, rows->value(), columns->value(), cell->currentText(),
                                              order->currentText(), delimiter->currentText()));
            exportTable->setEnabled(true);
        } catch (const std::exception &e) {
            tableText->clear();
            exportTable->setEnabled(false);
            QMessageBox::warning(&dlg, "Translated table", e.what());
        }
    });
    QObject::connect(exportTable, &QPushButton::clicked, &dlg, [&] {
        auto p = QFileDialog::getSaveFileName(&dlg, "Export displayed table to new file");
        if (!p.isEmpty())
            try {
                fileio::writeNew(p, tableText->toPlainText().toUtf8());
            } catch (const std::exception &e) {
                QMessageBox::warning(&dlg, "Table export", e.what());
            }
    });
    tabs->addTab(tab, "Translated table");
    if (translated)
        tabs->setCurrentWidget(tab);
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
