#include "../native/monitor.h"
#include "../native/window.h"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QMenu>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>

int main(int argc,char **argv){
    QApplication app(argc,argv);
    Window window;
    auto *providers=window.findChild<QComboBox *>("providerList");
    auto *status=window.findChild<QLabel *>("connectionStatus");
    QPushButton *connect=nullptr,*read=nullptr;
    for(auto *button:window.findChildren<QPushButton *>()){
        if(button->text()=="&Connect")connect=button;
        if(button->text()=="&Read")read=button;
    }
    if(!providers || providers->currentIndex()!=-1 || !connect || connect->isEnabled() || !read || read->isEnabled() ||
       !status || !status->text().contains("No J2534 device selected"))return 1;
    for(int i=0;i<providers->count();++i){
        auto text=providers->itemText(i).toLower();
        if(text.contains("simulator") || text.contains("virtual") || text.contains("mock"))return 2;
    }
    bool programming=false;
    for(auto *menu:window.findChildren<QMenu *>()){
        if(menu->title().contains("program",Qt::CaseInsensitive))programming=true;
        for(auto *action:menu->actions())
            if(action->text().contains("virtual",Qt::CaseInsensitive) || action->text().contains("simulator",Qt::CaseInsensitive))return 4;
    }
    if(!programming)return 3;
    auto *monitor=static_cast<Monitor *>(window.findChild<QWidget *>("acquisition"));
    auto *table=window.findChild<QTableWidget *>();
    if(!monitor || !table)return 5;
    monitor->loadIni(app.arguments().at(1));
    if(table->rowCount()!=2 || monitor->running() || connect->isEnabled() || read->isEnabled())return 6;
    window.show();
    return 0;
}
