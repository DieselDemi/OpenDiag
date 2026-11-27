#include "definitions.h"
#include "acquisition.h"
#include <QComboBox>
#include <QCryptographicHash>
#include <QDialog>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>
#include <QtEndian>
#include <stdexcept>

QString DtcDefinitions::code(quint16 n) {
    return QString("PCBU"[n >> 14]) + QString("%1").arg(n & 0x3fff, 4, 16, QChar('0')).toUpper();
}
DtcDefinitions DtcDefinitions::read(const QString &path) {
    auto file = acq::readFile(path, 16 << 20);
    if (QCryptographicHash::hash(file, QCryptographicHash::Sha256).toHex() !=
        "54023fd134365aa94d2327ffb5c45e6d7f4989a356ffe408647dc8b17de001de")
        throw std::runtime_error("Expected the original PTDIAG 2293 MyCANIC FD executable");
    const char *b = file.constData();
    auto u32 = [b](qsizetype p) { return qFromLittleEndian<quint32>(b + p); };
    auto u16 = [b](qsizetype p) { return qFromLittleEndian<quint16>(b + p); };
    auto pe = u32(60), optional = pe + 24, base = u32(optional + 28), sections = optional + u16(pe + 20);
    auto offset = [&](quint32 va) -> qsizetype {
        auto rva = va - base;
        for (int i = 0; i < u16(pe + 6); ++i) {
            auto s = sections + 40 * i, start = u32(s + 12);
            if (rva >= start && rva - start < u32(s + 16))
                return u32(s + 20) + (rva - start);
        }
        throw std::runtime_error("Invalid reference data address");
    };
    DtcDefinitions defs;
    // GetDTCDescription's two list views in this SHA-pinned build: 12-byte rows with different text columns.
    auto load = [&](quint32 va, int count, int column, auto &dest) {
        auto start = offset(va);
        for (int i = 0; i < count; ++i) {
            auto row = start + 12 * i;
            auto p = offset(u32(row + column));
            auto end = file.indexOf('\0', p);
            if (end < p || end - p > 1024)
                throw std::runtime_error("Invalid reference description");
            if (end > p)
                dest.insert(code(u16(row)), QString::fromLatin1(b + p, end - p));
        }
    };
    load(0x81fa20, 0x27ba, 4, defs.kwp);
    load(0x8a8380, 0x33fd, 8, defs.uds);
    return defs;
}
void DtcDefinitions::show(QWidget *parent) const {
    QDialog dlg(parent);
    dlg.setWindowTitle("PTDIAG trouble code definitions");
    dlg.resize(820, 490);
    auto *v = new QVBoxLayout(&dlg);
    QComboBox protocol;
    protocol.addItems({"ISO 14229", "KWP2000"});
    v->addWidget(&protocol);
    QLineEdit search;
    search.setPlaceholderText("Filter by code or description");
    v->addWidget(&search);
    QStandardItemModel data;
    QSortFilterProxyModel filtered;
    filtered.setSourceModel(&data);
    filtered.setFilterKeyColumn(-1);
    filtered.setFilterCaseSensitivity(Qt::CaseInsensitive);
    QTableView table;
    table.setEditTriggers(QTableView::NoEditTriggers);
    table.setModel(&filtered);
    table.horizontalHeader()->setStretchLastSection(true);
    v->addWidget(&table);
    auto fill = [&] {
        data.clear();
        data.setHorizontalHeaderLabels({"DTC", "PTDIAG description"});
        auto &dict = protocol.currentIndex() ? kwp : uds;
        for (auto i = dict.cbegin(); i != dict.cend(); ++i)
            data.appendRow({new QStandardItem(i.key()), new QStandardItem(i.value())});
    };
    QObject::connect(&protocol, &QComboBox::currentIndexChanged, &dlg, fill);
    QObject::connect(&search, &QLineEdit::textChanged, &filtered, &QSortFilterProxyModel::setFilterFixedString);
    fill();
    dlg.exec();
}
